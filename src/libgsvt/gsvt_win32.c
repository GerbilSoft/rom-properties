/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt_win32.c: Virtual Terminal wrapper functions. (Win32 version)       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gsvt.h"
#include "common.h"

// C includes
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctypex.h"

// Windows-specific stuff
#include "libwin32common/RpWin32_sdk.h"
#include <winternl.h>
#include <tchar.h>

#ifdef _MSC_VER
typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name;
	WCHAR NameBuffer[1];	// flex array
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

// NOTE: ObjectNameInformation isn't defined in the Windows 7 SDK.
#  define ObjectNameInformation ((OBJECT_INFORMATION_CLASS)1)
#endif /* _MSC_VER */

typedef NTSTATUS (WINAPI *pfnNtQueryObject_t)(
	_In_opt_ HANDLE Handle,
	_In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
	_Out_opt_ PVOID ObjectInformation,	// FIXME: Missing bcount annotation.
	_In_ ULONG ObjectInformationLength,
	_Out_opt_ PULONG returnLength
	);

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#  define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif /* !ENABLE_VIRTUAL_TERMINAL_PROCESSING */

struct _gsvt_console {
	FILE *stream;		// File handle, e.g. stdout or stderr
	bool is_console;	// True if this is a real console and not redirected to a file
	bool supports_ansi;	// True if the console supports ANSI escape sequences

	// Windows-specific stuff here
	bool is_real_console;	// True if this is a real Windows console and not e.g. MinTTY

	// Windows 10 1607 ("Anniversary Update") adds support for ANSI escape sequences.
	// For older Windows, we'll need to parse the sequences manually and
	// call SetConsoleTextAttribute().
	bool bold;
	bool bright;
	uint16_t wAttributesOrig;	// Original attributes
	uint16_t wAttributesCur;	// Current attributes, when using ConsoleSetTextColor()

	HANDLE hConsole;	// Console handle, or NULL if not a real console.
};

static gsvt_console __gsvt_stdout;
static gsvt_console __gsvt_stderr;

gsvt_console *gsvt_stdout = &__gsvt_stdout;
gsvt_console *gsvt_stderr = &__gsvt_stderr;

// Map ANSI colors (red=1) to Windows colors (blue=1).
static const uint8_t gsvt_win32_color_map[8] = {
	0, 4, 2, 6, 1, 5, 3, 7
};

/** Basic functions **/

/**
 * Check if we're using MinTTY.
 * @param hConsole Console handle
 * @return True if this is MinTTY; false if not.
 */
static bool check_mintty(HANDLE hConsole)
{
	// References:
	// - https://github.com/git/git/commit/58fcd54853023b28a44016c06bd84fc91d2556ed
	// - https://github.com/git/git/blob/master/compat/winansi.c
	ULONG result;
	BYTE buffer[1024];
	POBJECT_NAME_INFORMATION nameinfo = (POBJECT_NAME_INFORMATION)buffer;

	// Check if the handle is a pipe.
	if (GetFileType(hConsole) != FILE_TYPE_PIPE) {
		// Not a pipe.
		return false;
	}

	// Get the pipe name.
	HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
	assert(hNtDll != NULL);
	if (!hNtDll) {
		// Can't check without NTDLL.dll.
		return false;
	}
	pfnNtQueryObject_t pfnNtQueryObject = (pfnNtQueryObject_t)GetProcAddress(hNtDll, "NtQueryObject");
	if (!pfnNtQueryObject) {
		// Can't check without NtQueryObject().
		return false;
	}

	if (!NT_SUCCESS(pfnNtQueryObject(hConsole, ObjectNameInformation,
	                buffer, sizeof(buffer) - 2, &result)))
	{
		// Unable to get the pipe name.
		return false;
	}

	PWSTR name = nameinfo->Name.Buffer;
	name[nameinfo->Name.Length / sizeof(*name)] = 0;

	// Check if this could be a MSYS2 pty pipe ('msys-XXXX-ptyN-XX')
	// or a cygwin pty pipe ('cygwin-XXXX-ptyN-XX')
	if ((!wcsstr(name, L"msys-") && !wcsstr(name, L"cygwin-")) ||
	     !wcsstr(name, L"-pty"))
	{
		// Not a matching pipe.
		return false;
	}

	return true;
}

/**
 * Initialize console information for the specified file descriptor.
 * @param vt	[out] gsvt_console
 * @param f	[in] FILE*
 * @param fd	[in] Standard handle, e.g. STD_OUTPUT_HANDLE or STD_ERROR_HANDLE
 */
static void gsvt_init_win32(gsvt_console *vt, FILE *f, DWORD fd)
{
	vt->stream = f;

	// Default attributes (white on black)
	vt->bold = false;
	vt->bright = false;
	vt->wAttributesOrig = 0x07;
	vt->wAttributesCur = 0x07;

	HANDLE hStd = GetStdHandle(fd);
	if (!hStd || hStd == INVALID_HANDLE_VALUE) {
		// Not a valid console handle...
		vt->is_console = false;
		vt->supports_ansi = false;
		vt->is_real_console = false;
		vt->hConsole = NULL;
		return;
	}
	vt->hConsole = hStd;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hStd, &dwMode)) {
		// Not a real console.
		vt->is_real_console = false;

		// NOTE: Might be a MinTTY fake console.
		// NOTE 2: On Windows 10, MinTTY (git bash, cygwin) acts like a real console.
		if (check_mintty(hStd)) {
			// This is MinTTY.
			vt->is_console = true;
			vt->supports_ansi = true;
			return;
		} else {
			// Not MinTTY.
			vt->is_console = false;
			vt->supports_ansi = false;
		}
		return;
	}

	// We have a real console.
	vt->is_console = true;
	vt->is_real_console = true;

	// Does it support ANSI escape sequences?
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (SetConsoleMode(hStd, dwMode)) {
		// ANSI escape sequences enabled.
		vt->supports_ansi = true;
		return;
	}

	// Failed to enable ANSI escape sequences.
	vt->supports_ansi = false;

	// Save the original console text attributes.
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hStd, &csbi)) {
		vt->wAttributesOrig = csbi.wAttributes;
		vt->wAttributesCur = csbi.wAttributes;
	}
}

/**
 * Initialize VT detection.
 */
void gsvt_init(void)
{
	// Initialize the gsvt_console variables using stdout/stderr.
	gsvt_init_win32(&__gsvt_stdout, stdout, STD_OUTPUT_HANDLE);
	gsvt_init_win32(&__gsvt_stderr, stderr, STD_ERROR_HANDLE);
}

/**
 * Force-enable color for the specified gsvt_console.
 * @param vt
 */
void gsvt_force_color_on(gsvt_console *vt)
{
	vt->is_console = true;
	vt->supports_ansi = true;
}

/**
 * Force-disable color for the specified gsvt_console.
 * @param vt
 */
void gsvt_force_color_off(gsvt_console *vt)
{
	vt->is_console = false;
	vt->supports_ansi = false;
}

/**
 * Is the specified gsvt_console an actual console?
 * @param vt
 * @return True if it is; false if it isn't.
 */
bool gsvt_is_console(const gsvt_console *vt)
{
	return vt->is_console;
}

/**
 * Does the specified gsvt_console support ANSI escape sequences?
 * @param vt
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_ansi(const gsvt_console *vt)
{
	return vt->supports_ansi;
}

/** stdio wrapper functions **/

/**
 * Write UTF-8 text to the Windows console.
 * Direct write using WriteConsole(); no ANSI escape interpretation.
 * @param vt
 * @param str UTF-8 text string
 * @param len Length of UTF-8 text string (if -1, use strlen)
 * @return 0 on success; negative POSIX error code on error.
 */
static int gsvt_win32_console_print_raw(const gsvt_console *vt, const char *str, int len)
{
	HANDLE hConsole = vt->hConsole;
	assert(hConsole != NULL);
	if (!hConsole) {
		// No console handle...
		return -ENOTTY;
	}

	// Write in 4096-character chunks.
	// WriteConsole() seems to fail if the input buffer is > 64 KiB.
	#define CHUNK_SIZE 4096
	if (len < 0) {
		len = (int)strlen(str);
	}

#ifdef UNICODE
	// If using a real Windows console with ANSI escape sequences, use WriteConsoleA().
	if (vt->supports_ansi) {
		const char *p = str;
		for (int size = len; size > 0; size -= CHUNK_SIZE) {
			const DWORD chunk_len = (DWORD)((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
			WriteConsoleA(hConsole, p, chunk_len, NULL, NULL);
			p += chunk_len;
		}
	} else {
		// ANSI escape sequences are not supported.
		// This means it's likely older than Win10 1607, so no UTF-8 support.
		// Convert to UTF-16 first.

		// NOTE: Using MultiByteToWideChar() directly so we don't have a
		// libromdata (librptext) dependency, since this function is used if
		// libromdata can't be loaded for some reason.
		const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
		if (cchWcs <= 0) {
			// Unable to convert the text...
			return -EIO;
		}
		wchar_t *const wcs = (wchar_t*)malloc(cchWcs * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str, len, wcs, cchWcs);

		const wchar_t *p = wcs;
		for (int size = cchWcs; size > 0; size -= CHUNK_SIZE) {
			const DWORD chunk_len = (DWORD)((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
			WriteConsoleW(hConsole, p, chunk_len, NULL, NULL);
			p += chunk_len;
		}

		free(wcs);
	}
#else /* !UNICODE */
	// FIXME: Convert from UTF-8 to ANSI?
	const char *p = str;
	for (int size = len; size > 0; size -= CHUNK_SIZE) {
		const DWORD chunk_len = (DWORD)((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
		WriteConsoleA(hConsole, p, chunk_len, NULL, NULL);
		p += chunk_len;
	}
#endif /* UNICODE */

	return 0;
}

/**
 * Write text with ANSI escape sequences to the Windows console. (stdout)
 * Color escapes will be handled using SetConsoleTextAttribute().
 *
 * @param vt
 * @param str Source text
 * @param len Length (if -1, assume a NULL-terminated string)
 * @return 0 on success; negative POSIX error code on error.
 */
static int gsvt_win32_console_print_ANSI_emulate(gsvt_console *vt, const char *str, int len)
{
	HANDLE hConsole = vt->hConsole;
	assert(hConsole != NULL);
	if (!hConsole) {
		// No console handle...
		return -ENOTTY;
	}

	const char *const pStrEnd = (len >= 0) ? (str + len) : NULL;

	WORD wAttributes = vt->wAttributesCur;
	// Bold/bright tracking
	// FIXME: Set one of these if wAttributesOrig has FOREGROUND_INTENSITY?
	bool bold = vt->bold, bright = vt->bright;

	#define MAX_PARAMS 16
	int params_idx = 0;
	int params[MAX_PARAMS];	// "CSI n X" parameters (semicolon-separated)
	while (*str != '\0') {
		if (pStrEnd && str >= pStrEnd) {
			break;
		}

		// Find an escape character.
		const char *pEsc = strchr(str, '\033');
		if (!pEsc) {
			// No more escape characters.
			// Send the rest of the buffer.
			gsvt_win32_console_print_raw(vt, str, -1);
			break;
		}
		if (pStrEnd && pEsc >= pStrEnd) {
			// Out of bounds?
			// Send the rest of the buffer.
			gsvt_win32_console_print_raw(vt, str, (int)(pStrEnd - str));
			break;
		}

		// Found an escape character.
		// Send everything up to the escape.
		if (str != pEsc) {
			gsvt_win32_console_print_raw(vt, str, (int)(pEsc - str));
			str = pEsc;
		}

		// Check what type of escape sequence this is.
		str++;
		bool ok = true;
		switch (*str) {
			case '[':
				// Control Sequence Introducer (CSI)
				// NOTE: Only "CSI n m" (attributes) is supported.
				str++;
				break;

			case ']':
				// Operating System Command (OSC)
				// May be used for hyperlinks, but we can't easily support this
				// with regular Windows cmd, so skip it entirely.
				// Search for the end sequence: "\033\\" (ST)
				for (str++; *str != '\0'; str++) {
					if (str[0] == '\033' && str[1] == '\\') {
						// Found the end sequence.
						str += 2;
						break;
					}
				}
				// Restart parsing.
				ok = false;
				break;

			default:
				// Not supported.
				ok = false;
				str++;
				break;
		}
		if (!ok) {
			continue;
		}

		// "CSI n m" processing
		params_idx = 0;

		// Process all numeric parameters.
		// Processing stops at:
		// - semicolon: Next parameter
		// - digit: Part of a parameter
		// - letter: End of parameter
		// - other: Invalid
		int num = 0;
		char cmd = '\0';
		for (; *str != '\0'; str++) {
			const char c = *str;
			if (c == ';') {
				// Found a separator.
				// Save the parameter.
				if (params_idx < MAX_PARAMS) {
					params[params_idx++] = num;
				}
				num = 0;
			} else if (isdigit_ascii(c)) {
				// Found a digit.
				// This is part of a parameter.
				num *= 10;
				num += (c - '0');
			} else if (isalpha_ascii(c)) {
				// Found a letter.
				// Finished processing this sequence.

				// Save the last parameter.
				// NOTE: If no parameters were specified, this will be handled as "ESC 0 m" (reset).
				if (params_idx < MAX_PARAMS) {
					params[params_idx++] = num;
				}
				cmd = c;
				str++;
				break;
			} else {
				// Invalid character.
				str++;
				break;
			}
		}

		// Only "CSI n m" (SGR) is supported right now.
		if (cmd != 'm') {
			// Not SGR.
			continue;
		}

		// Apply attributes based on the parameters.
		for (int i = 0; i < params_idx; i++) {
			const int param = params[i];
			switch (param) {
				case 0:
					// Reset
					wAttributes = vt->wAttributesOrig;
					bold = false;
					bright = false;
					break;
				case 1:
					// Bold
					wAttributes |= FOREGROUND_INTENSITY;
					bold = true;
					break;
				case 4:
					// Underline
					// NOTE: Works on Windows 10; does not work on Windows 7.
					wAttributes |= COMMON_LVB_UNDERSCORE;
					break;
				case 7:
					// Reverse video
					// NOTE: Works on Windows 10; does not work on Windows 7.
					wAttributes |= COMMON_LVB_REVERSE_VIDEO;
					break;
				case 22:
					// Normal intensity
					wAttributes &= ~FOREGROUND_INTENSITY;
					break;
				case 24:
					// Not underline
					wAttributes &= ~COMMON_LVB_UNDERSCORE;
					break;
				case 27:
					// Not-reverse video
					// NOTE: Works on Windows 10; does not work on Windows 7.
					wAttributes &= ~COMMON_LVB_REVERSE_VIDEO;
					break;

				case 30: case 31: case 32: case 33:
				case 34: case 35: case 36: case 37:
					// Foreground color
					wAttributes &= ~0x000F;
					wAttributes |= gsvt_win32_color_map[param - 30];
					// Brightness is disabled here, but if bold is set,
					// we need to keep FOREGROUND_INTENSITY.
					bright = false;
					if (bold) {
						wAttributes |= FOREGROUND_INTENSITY;
					}
					break;
				case 38: case 48:
					// 8-bit or 24-bit foreground or background color.
					// NOT SUPPORTED; this is implemented in order to skip parameters.
					if (i + 1 >= params_idx) {
						// Not enough parameters?
						break;
					}
					// Check next parameter to determine how many to skip.
					i++;
					switch (params[i]) {
						case 2:
							// RGB truecolor: skip 3 parameters
							i += 3;
							break;
						case 5:
							// 256-color: skip 1 parameter
							i++;
							break;
						default:
							// FIXME: Not supported?
							break;
					}
					break;

				case 39:
					// Default foreground color
					// NOTE: Does not affect bold/bright.
					wAttributes &= ~0x0007;
					wAttributes |= (vt->wAttributesOrig & 0x0007);
					break;

				case 40: case 41: case 42: case 43:
				case 44: case 45: case 46: case 47:
					// Background color
					wAttributes &= ~0x0070;
					wAttributes |= (gsvt_win32_color_map[param - 40] << 4);
					break;
				case 49:
					// Default background color
					wAttributes &= ~0x0070;
					wAttributes |= (vt->wAttributesOrig & 0x0070);
					break;

				case 90: case 91: case 92: case 93:
				case 94: case 95: case 96: case 97:
					// Foreground color (bright)
					wAttributes &= ~0x0007;
					wAttributes |= gsvt_win32_color_map[param - 90];
					wAttributes |= FOREGROUND_INTENSITY;
					bright = true;
					break;

				case 100: case 101: case 102: case 103:
				case 104: case 105: case 106: case 107:
					// Background color (bright)
					wAttributes &= ~0x0070;
					wAttributes |= (gsvt_win32_color_map[param - 100] << 4);
					wAttributes |= BACKGROUND_INTENSITY;
					break;

				default:
					// Not a valid number.
					// Ignore it and keep processing.
					break;
			}
		}

		// Apply the new attributes.
		SetConsoleTextAttribute(hConsole, wAttributes);
	}

	// Save the current console attributes.
	vt->wAttributesCur = wAttributes;
	vt->bold = bold;
	vt->bright = bright;
	return 0;
}

/**
 * fwrite() wrapper function for gsvt_console.
 *
 * On Windows, if using a standard Windows console and ANSI escape sequences
 * are not supported, color will be emulated using SetConsoleTextAttribute().
 *
 * @param ptr Characters to write
 * @param nmemb Number of characters to write
 * @param vt
 * @return Number of characters written
 */
size_t gsvt_fwrite(const char *ptr, size_t nmemb, gsvt_console *vt)
{
	if (vt->is_real_console) {
		// This is a real Windows console.
		int ret;
		if (!vt->supports_ansi) {
			// ANSI escape sequences are not supported.
			// Parse the ANSI escape sequences and use SetConsoleTextAttribute().
			ret = gsvt_win32_console_print_ANSI_emulate(vt, ptr, (int)nmemb);
		} else {
			// ANSI escape sequences are supported.
			// Print the text directly to the console.
			ret = gsvt_win32_console_print_raw(vt, ptr, (int)nmemb);
		}

		return (ret == 0) ? nmemb : 0;
	}

	// Not a real console. Use fwrite().
	return fwrite(ptr, 1, nmemb, vt->stream);
}

/**
 * fputs() wrapper function for gsvt_console.
 *
 * On Windows, if using a standard Windows console and ANSI escape sequences
 * are not supported, color will be emulated using SetConsoleTextAttribute().
 *
 * @param s NULL-terminated string to write
 * @param vt
 * @return Non-negative number on success, or EOF on error.
 */
int gsvt_fputs(const char *s, gsvt_console *vt)
{
	if (vt->is_real_console) {
		// This is a real Windows console.
		int ret;
		if (!vt->supports_ansi) {
			// ANSI escape sequences are not supported.
			// Parse the ANSI escape sequences and use SetConsoleTextAttribute().
			ret = gsvt_win32_console_print_ANSI_emulate(vt, s, -1);
		} else {
			// ANSI escape sequences are supported.
			// Print the text directly to the console.
			ret = gsvt_win32_console_print_raw(vt, s, -1);
		}

		return (ret == 0) ? 1 : 0;
	}

	// Not a real console. Use fputs().
	return fputs(s, vt->stream);
}

/** Convenience functions **/

/**
 * Print a newline to the specified gsvt_console.
 * @param vt
 */
void gsvt_newline(gsvt_console *vt)
{
	if (vt->is_real_console) {
		// This is a real Windows console.
		// TODO: Use WriteConsoleW() or WriteConsoleA() directly?
		gsvt_win32_console_print_raw(vt, "\n", 1);
	} else {
		// Not a real console. Use fputc().
		fputc('\n', vt->stream);
	}
}

/** Color functions (NOPs if the console doesn't support color) **/

/**
 * Set the text color.
 * @param vt
 * @param color ANSI text color (8 color options)
 * @param bold If true, enable bold rendering. (commonly rendered as "bright")
 */
void gsvt_text_color_set8(gsvt_console *vt, uint8_t color, bool bold)
{
	if (!vt->is_console) {
		// Not a console.
		return;
	}

	assert(color < 8);
	color &= 0x07;

	// If printing to a real console, and ANSI escape sequences
	// are not supported, set Win32 console attributes.
	if (vt->is_real_console && !vt->supports_ansi) {
		// Set Win32 console attributes.
		vt->bold = bold;
		vt->wAttributesCur &= ~0x0F;
		vt->wAttributesCur |= gsvt_win32_color_map[color];
		if (bold) {
			vt->wAttributesCur |= FOREGROUND_INTENSITY;
		}
		SetConsoleTextAttribute(vt->hConsole, vt->wAttributesCur);
		return;
	}

	// ANSI escape sequences are supported.
	char buf[16];
	int len = snprintf(buf, sizeof(buf), "\033[3%u%sm", color, (bold ? ";1" : ""));
	if (len >= (int)sizeof(buf)) {
		len = (int)sizeof(buf);
	}
	fwrite(buf, 1, len, vt->stream);
}

/**
 * Reset the text color to its original value.
 * @param vt
 */
void gsvt_text_color_reset(gsvt_console *vt)
{
	if (!vt->is_console) {
		// Not a console.
		return;
	}

	// If printing to a real console, and ANSI escape sequences
	// are not supported, set Win32 console attributes.
	if (vt->is_real_console && !vt->supports_ansi) {
		// Set Win32 console attributes.
		vt->bold = !!(vt->wAttributesOrig & FOREGROUND_INTENSITY);
		vt->bright = false;
		vt->wAttributesCur = vt->wAttributesOrig;
		SetConsoleTextAttribute(vt->hConsole, vt->wAttributesOrig);
		return;
	}

	// ANSI escape sequences are supported.
	static const char ansi_color_reset[] = "\033[0m";
	fwrite(ansi_color_reset, 1, sizeof(ansi_color_reset)-1, vt->stream);
}

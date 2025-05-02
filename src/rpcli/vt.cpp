/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * vt.cpp: ANSI Virtual Terminal handling.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rpcli.h"

#include "vt.hpp"

// C++ STL classes
using std::array;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include <winternl.h>
#  include <tchar.h>

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

#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#  endif /* !ENABLE_VIRTUAL_TERMINAL_PROCESSING */
#else /* !_WIN32 */
#  include <unistd.h>
#endif /* _WIN32 */

// Map ANSI colors (red=1) to Windows colors (blue=1).
static constexpr array<uint8_t, 8> win32_color_map = {
	0, 4, 2, 6, 1, 5, 3, 7
};

// Console information
// NOTE: stdout and stderr can both be real consoles,
// both be redirected, or one real and one redirected.
ConsoleInfo_t ci_stdout;
ConsoleInfo_t ci_stderr;

#ifdef _WIN32
static bool check_mintty(HANDLE hStdOut)
{
	// References:
	// - https://github.com/git/git/commit/58fcd54853023b28a44016c06bd84fc91d2556ed
	// - https://github.com/git/git/blob/master/compat/winansi.c
	ULONG result;
	BYTE buffer[1024];
	POBJECT_NAME_INFORMATION nameinfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(buffer);

	// Check if the handle is a pipe.
	if (GetFileType(hStdOut) != FILE_TYPE_PIPE) {
		// Not a pipe.
		return false;
	}

	// Get the pipe name.
	HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
	assert(hNtDll != nullptr);
	if (!hNtDll) {
		// Can't check without NTDLL.dll.
		return false;
	}
	pfnNtQueryObject_t pfnNtQueryObject = reinterpret_cast<pfnNtQueryObject_t>(
		GetProcAddress(hNtDll, "NtQueryObject"));
	if (!pfnNtQueryObject) {
		// Can't check without NtQueryObject().
		return false;
	}

	if (!NT_SUCCESS(pfnNtQueryObject(hStdOut, ObjectNameInformation,
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
 * Initialize console information for the specified standard handle.
 * @param ci	[out] ConsoleInfo_t
 * @param fd	[in] Standard handle, e.g. STD_OUTPUT_HANDLE or STD_ERROR_HANDLE
 */
static void init_win32_ConsoleInfo_t(ConsoleInfo_t *ci, DWORD fd)
{
	// Default attributes (white on black)
	ci->wAttributesOrig = 0x07;
	ci->wAttributesCur = 0x07;

	HANDLE hStd = GetStdHandle(fd);
	if (!hStd || hStd == INVALID_HANDLE_VALUE) {
		// Not a valid console handle...
		ci->is_console = false;
		ci->supports_ansi = false;
		ci->is_real_console = false;
		ci->hConsole = nullptr;
		return;
	}
	ci->hConsole = hStd;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hStd, &dwMode)) {
		// Not a real console.
		ci->is_real_console = false;

		// NOTE: Might be a MinTTY fake console.
		// NOTE 2: On Windows 10, MinTTY (git bash, cygwin) acts like a real console.
		if (check_mintty(hStd)) {
			// This is MinTTY.
			ci->is_console = true;
			ci->supports_ansi = true;
			return;
		} else {
			// Not MinTTY.
			ci->is_console = false;
			ci->supports_ansi = false;
		}
		return;
	}

	// We have a real console.
	ci->is_console = true;
	ci->is_real_console = true;

	// Does it support ANSI escape sequences?
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (SetConsoleMode(hStd, dwMode)) {
		// ANSI escape sequences enabled.
		ci->supports_ansi = true;
		return;
	}

	// Failed to enable ANSI escape sequences.
	ci->supports_ansi = false;

	// Save the original console text attributes.
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hStd, &csbi)) {
		ci->wAttributesOrig = csbi.wAttributes;
	}
}
#else /* !_WIN32 */
static bool is_color_TERM = false;

/**
 * Check the TERM variable to determine if the terminal supports ANSI color.
 * This will set the is_color_TERM variable.
 */
static void check_TERM_variable(void)
{
	const char *const TERM = getenv("TERM");
	if (!TERM || TERM[0] == '\0') {
		// No TERM variable, or it's empty...
		is_color_TERM = false;
		return;
	}

	// Reference: https://github.com/jwalton/go-supportscolor/blob/5d4fbba7ce3e2f0629f5885f89cd9a2d3e0d7a39/supportscolor.go#L271
	// (?i)^screen|^xterm|^vt100|^vt220|^rxvt|color|ansi|cygwin|linux

	// Convert to lowercase.
	string s_term(TERM);
	std::transform(s_term.begin(), s_term.end(), s_term.begin(),
		[](char c) noexcept -> char { return std::tolower(c); });

	/// Check for matching terminals.

	// Match the beginning of the string.
	struct match_begin_t {
		uint8_t len;
		char term[7];
	};
	static const array<match_begin_t, 5> match_begin = {{
		{6, "screen"},
		{5, "xterm"},
		{5, "vt100"},
		{5, "vt220"},
		{4, "rxvt"},
	}};
	for (const match_begin_t &match : match_begin) {
		if (!s_term.compare(0, match.len, match.term)) {
			// Found a match!
			is_color_TERM = true;
			return;
		}
	}

	// Match the entire string.
	struct match_whole_t {
		char term[8];
	};
	static const array<match_whole_t, 4> match_whole = {{
		{"color"},
		{"ansi"},
		{"cygwin"},
		{"linux"},
	}};
	for (const match_whole_t &match : match_whole) {
		if (!s_term.compare(match.term)) {
			// Found a match!
			is_color_TERM = true;
			return;
		}
	}

	// Terminal does not support ANSI color.
	is_color_TERM = false;
	return;
}

/**
 * Initialize console information for the specified file descriptor.
 * @param ci	[out] ConsoleInfo_t
 * @param fd	[in] File descriptor
 */
static void init_posix_ConsoleInfo_t(ConsoleInfo_t *ci, int fd)
{
	// On other systems, use isatty() to determine if
	// stdout is a tty or a file.
	if (isatty(fd)) {
		// Is a tty.
		ci->is_console = true;
		// If $TERM matches a valid ANSI color terminal, ANSI color is supported.
		ci->supports_ansi = is_color_TERM;
	} else {
		// Not a tty.
		ci->is_console = false;
		ci->supports_ansi = false;
	}
}
#endif /* _WIN32 */

/**
 * Initialize VT detection.
 */
void init_vt(void)
{
	ci_stdout.stream = stdout;
	ci_stderr.stream = stderr;

#ifdef _WIN32
	init_win32_ConsoleInfo_t(&ci_stdout, STD_OUTPUT_HANDLE);
	init_win32_ConsoleInfo_t(&ci_stderr, STD_ERROR_HANDLE);
#else /* !_WIN32 */
	check_TERM_variable();
	init_posix_ConsoleInfo_t(&ci_stdout, fileno(stdout));
	init_posix_ConsoleInfo_t(&ci_stderr, fileno(stderr));
#endif
}

#ifdef _WIN32
/**
 * Write UTF-8 text to the Windows console.
 * Direct write using WriteConsole(); no ANSI escape interpretation.
 * @param ci ConsoleInfo_t
 * @param str UTF-8 text string
 * @param len Length of UTF-8 text string (if -1, use strlen)
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_write_to_console(const ConsoleInfo_t *ci, const char *str, int len)
{
	HANDLE hConsole = ci->hConsole;
	assert(hConsole != nullptr);
	if (!hConsole) {
		// No console handle...
		return -ENOTTY;
	}

	// Write in 4096-character chunks.
	// WriteConsole() seems to fail if the input buffer is > 64 KiB.
	static constexpr int CHUNK_SIZE = 4096;
	if (len < 0) {
		len = static_cast<int>(strlen(str));
	}

#ifdef UNICODE
	// If using a real Windows console with ANSI escape sequences, use WriteConsoleA().
	if (ci->supports_ansi) {
		const char *p = str;
		for (int size = len; size > 0; size -= CHUNK_SIZE) {
			const DWORD chunk_len = static_cast<DWORD>((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
			WriteConsoleA(hConsole, p, chunk_len, nullptr, nullptr);
			p += chunk_len;
		}
	} else {
		// ANSI escape sequences are not supported.
		// This means it's likely older than Win10 1607, so no UTF-8 support.
		// Convert to UTF-16 first.

		// NOTE: Using MultiByteToWideChar() directly so we don't have a
		// libromdata (librptext) dependency, since this function is used if
		// libromdata can't be loaded for some reason.
		const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, str, len, nullptr, 0);
		if (cchWcs <= 0) {
			// Unable to convert the text...
			return -EIO;
		}
		unique_ptr<wchar_t[]> wcs(new wchar_t[cchWcs]);
		MultiByteToWideChar(CP_UTF8, 0, str, len, wcs.get(), cchWcs);

		const wchar_t *p = wcs.get();
		for (int size = cchWcs; size > 0; size -= CHUNK_SIZE) {
			const DWORD chunk_len = static_cast<DWORD>((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
			WriteConsoleW(hConsole, p, chunk_len, nullptr, nullptr);
			p += chunk_len;
		}
	}
#else /* !UNICODE */
	// FIXME: Convert from UTF-8 to ANSI?
	const char *p = str;
	for (int size = len; size > 0; size -= CHUNK_SIZE) {
		const DWORD chunk_len = static_cast<DWORD>((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
		WriteConsoleA(hConsole, p, chunk_len, nullptr, nullptr);
		p += chunk_len;
	}
#endif /* UNICODE */

	return 0;
}

/**
 * Non-localized isdigit() implementation for ANSI escape sequence processing.
 * @param c Character
 * @param True if c is a digit; false if not.
 */
static constexpr inline bool vt_isdigit(char c)
{
	return ((uint8_t)c >= '0' && (uint8_t)c <= '9');
}

/**
 * Non-localized isalpha() implementation for ANSI escape sequence processing.
 * @param c Character
 * @param True if c is a letter; false if not.
 */
static constexpr inline bool vt_isalpha(char c)
{
	return ((uint8_t)c >= 'A' && (uint8_t)c <= 'Z') ||
	       ((uint8_t)c >= 'a' && (uint8_t)c <= 'z');
}

/**
 * Write text with ANSI escape sequences to the Windows console. (stdout)
 * Color escapes will be handled using SetConsoleTextAttribute().
 *
 * @param str Source text
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_console_print_ansi_color(const char *str)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(hStdOut != nullptr && hStdOut != INVALID_HANDLE_VALUE);
	if (!hStdOut || hStdOut == INVALID_HANDLE_VALUE) {
		// Cannot access the console handle...
		return -ENOTTY;
	}

	WORD wAttributes = ci_stdout.wAttributesOrig;
	// Bold/bright tracking
	// FIXME: Set one of these if wAttributesOrig has FOREGROUND_INTENSITY?
	bool bold = false, bright = false;

	vector<int> params;	// "CSI n X" parameters (semicolon-separated)
	while (*str != '\0') {
		// Find an escape character.
		const char *pEsc = strchr(str, '\033');
		if (!pEsc) {
			// No more escape characters.
			// Send the rest of the buffer.
			win32_write_to_console(&ci_stdout, str);
			break;
		}

		// Found an escape character.
		// Send everything up to the escape.
		if (str != pEsc) {
			win32_write_to_console(&ci_stdout, str, static_cast<int>(pEsc - str));
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
		params.clear();

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
				params.push_back(num);
				num = 0;
			} else if (vt_isdigit(c)) {
				// Found a digit.
				// This is part of a parameter.
				num *= 10;
				num += (c - '0');
			} else if (vt_isalpha(c)) {
				// Found a letter.
				// Finished processing this sequence.

				// Save the last parameter.
				// NOTE: If no parameters were specified, this will be handled as "ESC 0 m" (reset).
				params.push_back(num);
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
		const size_t params_size = params.size();
		for (size_t i = 0; i < params_size; i++) {
			const int param = params[i];
			switch (param) {
				case 0:
					// Reset
					wAttributes = ci_stdout.wAttributesOrig;
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
					wAttributes |= win32_color_map[param - 30];
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
					if (i + 1 >= params_size) {
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
					wAttributes |= (ci_stdout.wAttributesOrig & 0x0007);
					break;

				case 40: case 41: case 42: case 43:
				case 44: case 45: case 46: case 47:
					// Background color
					wAttributes &= ~0x0070;
					wAttributes |= (win32_color_map[param - 40] << 4);
					break;
				case 49:
					// Default background color
					wAttributes &= ~0x0070;
					wAttributes |= (ci_stdout.wAttributesOrig & 0x0070);
					break;

				case 90: case 91: case 92: case 93:
				case 94: case 95: case 96: case 97:
					// Foreground color (bright)
					wAttributes &= ~0x0007;
					wAttributes |= win32_color_map[param - 90];
					wAttributes |= FOREGROUND_INTENSITY;
					bright = true;
					break;

				case 100: case 101: case 102: case 103:
				case 104: case 105: case 106: case 107:
					// Background color (bright)
					wAttributes &= ~0x0070;
					wAttributes |= (win32_color_map[param - 100] << 4);
					wAttributes |= BACKGROUND_INTENSITY;
					break;

				default:
					// Not a valid number.
					// Ignore it and keep processing.
					break;
			}
		}

		// Apply the new attributes.
		SetConsoleTextAttribute(hStdOut, wAttributes);
	}

	// Restore the original console attributes.
	SetConsoleTextAttribute(hStdOut, ci_stdout.wAttributesOrig);
	return 0;
}
#endif /* _WIN32 */

/**
 * Print text to the console.
 *
 * On Windows, if a real console is in use, use WriteConsole().
 *
 * On other systems, or if we're not using a real console on Windows,
 * use regular stdio functions.
 *
 * @param ci ConsoleInfo_t
 * @param str String
 * @paran len Length of string
 * @param newline If true, print a newline afterwards.
 */
void ConsolePrint(const ConsoleInfo_t *ci, const char *str, size_t len, bool newline)
{
#ifdef _WIN32
	// Windows: If printing to console and UTF-8 is not enabled,
	// convert to UTF-16 and use WriteConsoleW().
	if (ci->is_console && ci->is_real_console) {
		fflush(ci->stream);
		int ret = win32_write_to_console(ci, str, static_cast<int>(len));
		if (ret == 0) {
			// Text written to console successfully.
			if (newline) {
				ret = win32_write_to_console(ci, "\n", 1);
				if (ret != 0) {
					// Failed to write the newline?
					// Use stdio as a fallback.
					fputc('\n', ci->stream);
				}
			}
		} else {
			// Failed to write to console.
			// Use stdio as a fallback.
			fwrite(str, 1, len, ci->stream);
			if (newline) {
				fputc('\n', ci->stream);
			}
		}
	} else
#endif /* _WIN32 */
	{
		// Regular stdio output.
		fwrite(str, 1, len, ci->stream);
		if (newline) {
			fputc('\n', ci->stream);
		}
	}
}

/**
 * Print a newline to the console.
 * Equivalent to fputc('\n', stdout).
 *
 * @param ci ConsoleInfo_t
 */
void ConsolePrintNewline(const ConsoleInfo_t *ci)
{
#ifdef _WIN32
	// Windows: If printing to console and UTF-8 is not enabled,
	// convert to UTF-16 and use WriteConsoleW().
	if (ci->is_console && ci->is_real_console) {
		fflush(ci->stream);
		int ret = win32_write_to_console(ci, "\n", 1);
		if (ret != 0) {
			// Failed to write to console.
			// Use stdio as a fallback.
			fputc('\n', ci->stream);
		}
	} else
#endif /* _WIN32 */
	{
		// Regular stdio output.
		fputc('\n', ci->stream);
	}
}

/**
 * Set the console text color.
 * @param ci ConsoleInfo_t
 * @param color Console text color (ANSI escape sequence value, 0-7)
 * @param bold If true, bold the text (or use high-intensity).
 */
void ConsoleSetTextColor(ConsoleInfo_t *ci, uint8_t color, bool bold)
{
	if (!ci->is_console) {
		// Not a console. No colors.
		return;
	}
#ifndef _WIN32
	if (!ci->supports_ansi) {
		// Non-Windows: No support for ANSI colors.
		return;
	}
#endif /* !_WIN32 */

	assert(color < 8);
	color &= 0x07;

#ifdef _WIN32
	// Windows: If printing to a real console, and ANSI escape sequences
	// are not supported, set Win32 console attributes.
	if (ci->is_real_console && !ci->supports_ansi) {
		// Set Win32 console attributes.
		ci->wAttributesCur &= ~0x0F;
		ci->wAttributesCur |= win32_color_map[color];
		if (bold) {
			ci->wAttributesCur |= FOREGROUND_INTENSITY;
		}
		SetConsoleTextAttribute(ci->hConsole, ci->wAttributesCur);
		return;
	}
#endif /* _WIN32 */

	// ANSI escape sequences are supported.
	char buf[32];
	int len = 0;
	if (bold) {
		len = snprintf(buf, sizeof(buf), "\033[3%u;1m", color);
	} else {
		len = snprintf(buf, sizeof(buf), "\033[3%um", color);
	}
#ifdef _WIN32
	if (ci->is_real_console) {
		WriteConsoleA(ci->hConsole, buf, len, nullptr, nullptr);
	} else
#endif /* _WIN32 */
	{
		fwrite(buf, 1, len, ci->stream);
	}
}

/**
 * Reset the console text color to the original value.
 * @param ci ConsoleInfo_t
 */
void ConsoleResetTextColor(ConsoleInfo_t *ci)
{
	if (!ci->is_console) {
		// Not a console. No colors.
		return;
	}
#ifndef _WIN32
	if (!ci->supports_ansi) {
		// Non-Windows: No support for ANSI colors.
		return;
	}
#endif /* !_WIN32 */

#ifdef _WIN32
	// Windows: If printing to a real console, and ANSI escape sequences
	// are not supported, set Win32 console attributes.
	if (ci->is_real_console && !ci->supports_ansi) {
		// Set Win32 console attributes.
		ci->wAttributesCur = ci->wAttributesOrig;
		SetConsoleTextAttribute(ci->hConsole, ci->wAttributesOrig);
		return;
	}
#endif /* _WIN32 */

	// ANSI escape sequences are supported.
	static const char ansi_color_reset[] = "\033[0m";
#ifdef _WIN32
	if (ci->is_real_console) {
		WriteConsoleA(ci->hConsole, ansi_color_reset, sizeof(ansi_color_reset)-1, nullptr, nullptr);
	} else
#endif /* _WIN32 */
	{
		fwrite(ansi_color_reset, 1, sizeof(ansi_color_reset)-1, ci->stream);
	}
}

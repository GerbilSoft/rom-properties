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

// librptext
#include "librptext/conversion.hpp"
using LibRpText::utf8_to_utf16;

// C++ STL classes
using std::array;
using std::ostream;
using std::string;
using std::u16string;

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
 * Detect if a standard handle is a console or not.
 * @param ci	[out] ConsoleInfo_t
 * @param fd	[in] Standard handle, e.g. STD_OUTPUT_HANDLE or STD_ERROR_HANDLE
 */
static void init_win32_ConsoleInfo_t(ConsoleInfo_t *ci, DWORD fd)
{
	// Default attributes (white on black)
	ci->wAttributesOrig = 0x07;

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
		// Not a console.
		ci->is_real_console = false;

		// NOTE: Might be a MinTTY fake console.
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
#endif /* _WIN32 */

/**
 * Initialize VT detection.
 */
void init_vt(void)
{
#ifdef _WIN32
	ci_stdout.stream = stdout;
	ci_stderr.stream = stderr;

	init_win32_ConsoleInfo_t(&ci_stdout, STD_OUTPUT_HANDLE);
	init_win32_ConsoleInfo_t(&ci_stderr, STD_ERROR_HANDLE);
#else /* !_WIN32 */
	// On other systems, use isatty() to determine if
	// stdout is a tty or a file.
	ci_stdout.is_console = !!isatty(fileno(stdout));
	ci_stderr.is_console = !!isatty(fileno(stderr));
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
int win32_write_to_console(ConsoleInfo_t *ci, const char *str, int len)
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

#ifdef UNICODE
	// Convert to UTF-16 first.
	u16string wstr = utf8_to_utf16(str, len);
	const wchar_t *p = reinterpret_cast<const wchar_t*>(wstr.data());
	for (int size = static_cast<int>(wstr.size()); size > 0; size -= CHUNK_SIZE) {
		const DWORD chunk_len = static_cast<DWORD>((size > CHUNK_SIZE) ? CHUNK_SIZE : size);
		WriteConsoleW(hConsole, p, chunk_len, nullptr, nullptr);
		p += chunk_len;
	}
#else /* !UNICODE */
	// FIXME: Convert to ANSI?
	if (len < 0) {
		len = static_cast<int>(strlen(str));
	}
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
 * Write text with ANSI escape sequences to the Windows console. (stdout)
 * Color escapes will be handled using SetConsoleTextAttribute().
 *
 * @param str Source text
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_console_print_ansi_color(const char *str)
{
	// Map ANSI colors (red=1) to Windows colors (blue=1).
	static constexpr array<uint8_t, 8> color_map = {
		0, 4, 2, 6, 1, 5, 3, 7
	};

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(hStdOut != nullptr && hStdOut != INVALID_HANDLE_VALUE);
	if (!hStdOut || hStdOut == INVALID_HANDLE_VALUE) {
		// Cannot access the console handle...
		return -ENOTTY;
	}

	WORD wAttributes = ci_stdout.wAttributesOrig;

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

		// Supported escape sequences:
		// - 0: Reset
		// - 1: Bold
		// - 31-37: Foreground color
		// - 41-47: Background color
		// - m: End of sequence
		// - ;: separator
		//"\033[31;1m";

		// Check for a '['.
		str++;
		if (*str != '[') {
			// Not a valid escape sequence.
			continue;
		}

		int num;
seq_loop:
		// Find the next ';' or 'm'.
		str++;
		const char *pSep = nullptr;;
		for (const char *p = str; *p != '\0'; p++) {
			if (*p == ';' || *p == 'm') {
				// Found a separator.
				pSep = p;
				break;
			}
		}
		if (!pSep) {
			// No separator...
			// Not a valid escape sequence.
			continue;
		}

		// Parse the decimal number in: [str, pSep)
		if (str == pSep) {
			// Invalid number.
			continue;
		}
		num = 0;
		for (; str < pSep; str++) {
			const char chr = *str;
			if (chr >= '0' && chr <= '9') {
				num *= 10;
				num += (chr - '0');
			} else {
				// Invalid number.
				num = -1;
				break;
			}
		}

		// Check the number.
		if (num == 0) {
			// Reset
			wAttributes = ci_stdout.wAttributesOrig;
		} else if (num == 1) {
			// Bold
			wAttributes |= FOREGROUND_INTENSITY;
		} else if (num >= 30 && num <= 37) {
			// Foreground color
			wAttributes &= ~0x0007;
			wAttributes |= color_map[num - 30];
		} else if (num >= 40 && num <= 37) {
			// Background color
			wAttributes &= ~0x0070;
			wAttributes |= (color_map[num - 40] << 4);
		} else {
			// Not a valid escape sequence.
			continue;
		}

		// Is this a separator or the end of the sequence?
		if (*str == ';') {
			goto seq_loop;
		}

		// Set the console attributes.
		SetConsoleTextAttribute(hStdOut, wAttributes);
		str++;
	}

	// Restore the original console attributes.
	SetConsoleTextAttribute(hStdOut, ci_stdout.wAttributesOrig);
	return 0;
}
#endif /* _WIN32 */

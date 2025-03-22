/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * vt.cpp: ANSI Virtual Terminal handling.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "vt.hpp"

// C++ STL classes
using std::array;
using std::ostream;
using std::string;

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include <winternl.h>
#  include <tchar.h>

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name;
	WCHAR NameBuffer[1];	// flex array
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

// NOTE: ObjectNameInformation isn't defined in the Windows 7 SDK.
#  define ObjectNameInformation ((OBJECT_INFORMATION_CLASS)1)

typedef NTSTATUS (WINAPI *pfnNtQueryObject_t)(
	_In_opt_ HANDLE Handle,
	_In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
	_Out_opt_ PVOID ObjectInformation,	// FIXME: Missing bcount annotation.
	_In_ ULONG ObjectInformationLength,
	_Out_opt_ PULONG returnLength
	);
#else /* !_WIN32 */
#  include <unistd.h>
#endif /* _WIN32 */

// Is stdout a console?
// If it is, we can print ANSI escape sequences.
bool is_stdout_console = false;

#ifdef _WIN32
// Windows 10 1607 ("Anniversary Update") adds support for ANSI escape sequences.
// For older Windows, we'll need to parse the sequences manually and
// call SetConsoleTextAttribute().
bool does_console_support_ansi = false;
static WORD wAttributesOrig = 0x07;	// default is white on black
#endif /* _WIN32 */

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
#endif /* _WIN32 */

/**
 * Initialize VT detection.
 */
void init_vt(void)
{
#ifdef _WIN32
	// On Windows 10 1607+ ("Anniversary Update"), we can enable
	// VT escape sequence processing.
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!hStdOut) {
		// No stdout...
		is_stdout_console = false;
		does_console_support_ansi = false;
		return;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hStdOut, &dwMode)) {
		// Not a console.
		// NOTE: Might be a MinTTY fake console.
		if (check_mintty(hStdOut)) {
			// This is MinTTY.
			is_stdout_console = true;
			does_console_support_ansi = true;
			return;
		}

		// Not MinTTY.
		is_stdout_console = false;
		does_console_support_ansi = false;
		return;
	}

	// We have a console.
	is_stdout_console = true;

	// Does it support ANSI escape sequences?
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (SetConsoleMode(hStdOut, dwMode)) {
		// ANSI escape sequences enabled.
		does_console_support_ansi = true;
		return;
	}

	// Failed to enable ANSI escape sequences.
	does_console_support_ansi = false;

	// Save the original console text attributes.
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
		wAttributesOrig = csbi.wAttributes;
	} else {
		// Failed to get the console text attributes.
		// Default to white on black.
		wAttributesOrig = 0x07;
	}
#else /* !_WIN32 */
	// On other systems, use isatty() to determine if
	// stdout is a tty or a file.
	is_stdout_console = !!isatty(fileno(stdout));
#endif
}

#ifdef _WIN32
/**
 * Write text with ANSI escape sequences to a Windows console.
 * Color escapes will be handled using SetConsoleTextAttribute().
 * TODO: Unicode?
 *
 * @param os Output stream (should be cout)
 * @param str Source text
 */
void cout_win32_ansi_color(ostream &os, const char *str)
{
	// Map ANSI colors (red=1) to Windows colors (blue=1).
	static constexpr array<uint8_t, 8> color_map = {
		0, 4, 2, 6, 1, 5, 3, 7
	};

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(hStdOut != nullptr);
	if (!hStdOut) {
		// Cannot access the console handle...
		// Print the string as-is.
		os << str;
		return;
	}

	WORD wAttributes = wAttributesOrig;

	while (*str != '\0') {
		// Find an escape character.
		const char *pEsc = strchr(str, '\033');
		if (!pEsc) {
			// No more escape characters.
			// Send the rest of the buffer.
			os << str;
			break;
		}

		// Found an escape character.
		// Send everything up to the escape.
		if (str != pEsc) {
			// TODO: std::string_view() if available?
			os << string(str, pEsc - str);
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
			wAttributes = wAttributesOrig;
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

		// Flush the output stream before setting attributes.
		os.flush();
		// Set the console attributes.
		SetConsoleTextAttribute(hStdOut, wAttributes);
		str++;
	}

	// Flush the output stream before restoring attributes.
	os.flush();
	// Restore the original console attributes.
	SetConsoleTextAttribute(hStdOut, wAttributesOrig);
}
#endif /* _WIN32 */

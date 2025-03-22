/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * vt.cpp: ANSI Virtual Terminal handling.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "vt.hpp"

// C includes
#include <stdio.h>

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
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
static WORD console_attrs_orig = 0x07;	// default is white on black
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
		console_attrs_orig = csbi.wAttributes;
	} else {
		// Failed to get the console text attributes.
		// Default to white on black.
		console_attrs_orig = 0x07;
	}
#else /* !_WIN32 */
	// On other systems, use isatty() to determine if
	// stdout is a tty or a file.
	is_stdout_console = !!isatty(fileno(stdout));
#endif
}

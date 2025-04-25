/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * vt.hpp: ANSI Virtual Terminal handling.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#include <stdint.h>
#include <string.h>
#include "stdboolx.h"

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#else /* !_WIN32 */
#  include <stdio.h>
#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ConsoleInfo_t {
	FILE *stream;		// File handle, e.g. stdout or stderr
	bool is_console;	// True if this is a real console and not redirected to a file.
#ifdef _WIN32
	bool supports_ansi;	// True if the console supports ANSI escape sequences.
	bool is_real_console;	// True if this is a real Windows console and not e.g. MinTTY.

	// Windows 10 1607 ("Anniversary Update") adds support for ANSI escape sequences.
	// For older Windows, we'll need to parse the sequences manually and
	// call SetConsoleTextAttribute().
	uint16_t wAttributesOrig;	// For real consoles: Original attributes

	HANDLE hConsole;	// Console handle, or nullptr if not a real console.
#endif /* _WIN32 */
} ConsoleInfo_t;

extern ConsoleInfo_t ci_stdout;
extern ConsoleInfo_t ci_stderr;

/**
 * Initialize VT detection.
 */
void init_vt(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// C++ includes
#include <ostream>
#include <string>

#ifdef _WIN32
/**
 * Write UTF-8 text to the Windows console.
 * Direct write using WriteConsole(); no ANSI escape interpretation.
 * @param ci ConsoleInfo_t
 * @param str UTF-8 text string
 * @param len Length of UTF-8 text string (if -1, use strlen)
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_write_to_console(const ConsoleInfo_t *ci, const char *str, int len = -1);

/**
 * Write text with ANSI escape sequences to the Windows console. (stdout)
 * Color escapes will be handled using SetConsoleTextAttribute().
 *
 * @param str Source text
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_console_print_ansi_color(const char *str);
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
 * @param len Length of string
 * @param newline If true, print a newline afterwards.
 */
void ConsolePrint(const ConsoleInfo_t *ci, const char *str, size_t len, bool newline);

/**
 * Print text to the console.
 *
 * On Windows, if a real console is in use, use WriteConsole().
 *
 * On other systems, or if we're not using a real console on Windows,
 * use regular stdio functions.
 *
 * @param ci ConsoleInfo_t
 * @param str C string
 * @param newline If true, print a newline afterwards.
 */
static inline void ConsolePrint(const ConsoleInfo_t *ci, const char *str, bool newline = false)
{
	ConsolePrint(ci, str, strlen(str), newline);
}

/**
 * Print text to the console.
 *
 * On Windows, if a real console is in use, use WriteConsole().
 *
 * On other systems, or if we're not using a real console on Windows,
 * use regular stdio functions.
 *
 * @param ci ConsoleInfo_t
 * @param str String (C++)
 * @param newline If true, print a newline afterwards.
 */
static inline void ConsolePrint(const ConsoleInfo_t *ci, const std::string &str, bool newline = false)
{
	ConsolePrint(ci, str.data(), str.size(), newline);
}

/**
 * Print a newline to the console.
 * Equivalent to fputc('\n', stdout).
 *
 * @param ci ConsoleInfo_t
 */
void ConsolePrintNewline(const ConsoleInfo_t *ci);

#endif /* __cplusplus */

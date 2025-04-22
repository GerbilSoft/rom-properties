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
#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

// Is stdout a console?
// If it is, we can print ANSI escape sequences.
extern bool is_stdout_console;

#ifdef _WIN32
// Windows 10 1607 ("Anniversary Update") adds support for ANSI escape sequences.
// For older Windows, we'll need to parse the sequences manually and
// call SetConsoleTextAttribute().
extern bool does_console_support_ansi;
#endif /* _WIN32 */

/**
 * Initialize VT detection.
 */
void init_vt(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <ostream>

#ifdef _WIN32
/**
 * Write UTF-8 text to the Windows console.
 * Direct write using WriteConsole(); no ANSI escape interpretation.
 * @param str UTF-8 text string
 * @param len Length of UTF-8 text string (if -1, use strlen)
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_write_to_console(const char *str, int len = -1);

/**
 * Write text with ANSI escape sequences to the Windows console.
 * Color escapes will be handled using SetConsoleTextAttribute().
 *
 * @param str Source text
 * @return 0 on success; negative POSIX error code on error.
 */
int win32_console_print_ansi_color(const char *str);
#endif /* _WIN32 */

#endif /* __cplusplus */

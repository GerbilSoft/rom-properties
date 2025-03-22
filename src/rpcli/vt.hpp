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
 * Write text with ANSI escape sequences to a Windows console.
 * Color escapes will be handled using SetConsoleTextAttribute().
 * TODO: Unicode?
 *
 * @param os Output stream (should be cout)
 * @param str Source text
 */
void cout_win32_ansi_color(std::ostream &os, const char *str);
#endif /* _WIN32 */

#endif /* __cplusplus */

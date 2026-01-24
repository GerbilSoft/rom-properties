/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt.h: Virtual Terminal wrapper functions.                             *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#include <stddef.h>
#include <stdint.h>
#include "stdboolx.h"

#include "tcharx.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _gsvt_console;
typedef struct _gsvt_console gsvt_console;

/** Standard I/O handle wrappers **/
extern gsvt_console *gsvt_stdout;
extern gsvt_console *gsvt_stderr;

/** Basic functions **/

/**
 * Initialize VT detection.
 */
void gsvt_init(void);

/**
 * Force-enable color for the specified gsvt_console.
 * @param vt
 */
void gsvt_force_color_on(gsvt_console *vt);

/**
 * Force-disable color for the specified gsvt_console.
 * @param vt
 */
void gsvt_force_color_off(gsvt_console *vt);

/**
 * Is the specified gsvt_console an actual console?
 * @param vt
 * @return True if it is; false if it isn't.
 */
bool gsvt_is_console(const gsvt_console *vt);

/**
 * Does the specified gsvt_console support ANSI escape sequences?
 * @param vt
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_ansi(const gsvt_console *vt);

/**
 * Does the terminal support Sixel?
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_sixel(void);

/**
 * Get the size of a single character cell on the terminal.
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @param pWidth	[out] Character width
 * @param pHeight	[out] Character height
 * @return 0 on success; negative POSIX error code on error.
 */
int gsvt_get_cell_size(int *pWidth, int *pHeight);

/** stdio wrapper functions **/

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
size_t gsvt_fwrite(const char *ptr, size_t nmemb, gsvt_console *vt);

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
int gsvt_fputs(const char *s, gsvt_console *vt);

/**
 * fflush() wrapper function for gsvt_console.
 *
 * @param vt
 * @return 0 on success; negative POSIX error code on error.
 */
int gsvt_fflush(gsvt_console *vt);

/** Convenience functions **/

/**
 * Print a newline to the specified gsvt_console.
 * @param vt
 */
void gsvt_newline(gsvt_console *vt);

/** Color functions (NOPs if the console doesn't support color) **/

enum AnsiColor8 {
	ANSI_COLOR_8_BLACK	= 0,
	ANSI_COLOR_8_RED	= 1,
	ANSI_COLOR_8_GREEN	= 2,
	ANSI_COLOR_8_YELLOW	= 3,
	ANSI_COLOR_8_BLUE	= 4,
	ANSI_COLOR_8_MAGENTA	= 5,
	ANSI_COLOR_8_CYAN	= 6,
	ANSI_COLOR_8_WHITE	= 7,
};

/**
 * Set the text color.
 * @param vt
 * @param color ANSI text color (8 color options)
 * @param bold If true, enable bold rendering. (commonly rendered as "bright")
 */
void gsvt_text_color_set8(gsvt_console *vt, uint8_t color, bool bold);

/**
 * Reset the text color to its original value.
 * @param vt
 */
void gsvt_text_color_reset(gsvt_console *vt);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

// C++ includes
#include <string>

/** C++ inline overloads for convenience purposes **/

/**
 * fputs() wrapper function for gsvt_console.
 *
 * On Windows, if using a standard Windows console and ANSI escape sequences
 * are not supported, color will be emulated using SetConsoleTextAttribute().
 *
 * @param s C++ string
 * @param vt
 * @return Non-negative number on success, or EOF on error.
 */
static inline int gsvt_fputs(const std::string &s, gsvt_console *vt)
{
	// NOTE: Should return a non-negative number on success, and EOF on error.
	return static_cast<int>(gsvt_fwrite(s.data(), s.size(), vt));
}

#endif /* __cplusplus */

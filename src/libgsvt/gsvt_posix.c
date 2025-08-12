/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt_posix.c: Virtual Terminal wrapper functions. (POSIX version)       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gsvt.h"
#include "common.h"

// pthreads, for pthread_once()
#include <pthread.h>

// C includes
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctypex.h"

struct _gsvt_console {
	FILE *stream;		// File handle, e.g. stdout or stderr
	bool is_console;	// True if this is a real console and not redirected to a file
	bool supports_ansi;	// True if the console supports ANSI escape sequences
};

static gsvt_console __gsvt_stdout;
static gsvt_console __gsvt_stderr;

gsvt_console *gsvt_stdout = &__gsvt_stdout;
gsvt_console *gsvt_stderr = &__gsvt_stderr;

static bool is_color_TERM = false;

/** Basic functions **/

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
	char *const TERM_lower = strdup(TERM);
	for (char *p = TERM_lower; *p != '\0'; p++) {
		*p = tolower(*p);
	}

	/// Check for matching terminals.

	// Match the beginning of the string.
	struct match_begin_t {
		uint8_t len;
		char term[7];
	};
	static const struct match_begin_t match_begin[5] = {
		{6, "screen"},
		{5, "xterm"},
		{5, "vt100"},
		{5, "vt220"},
		{4, "rxvt"},
	};
	for (const struct match_begin_t *p = match_begin;
	     p != &match_begin[ARRAY_SIZE(match_begin)]; p++)
	{
		if (!strncmp(TERM_lower, p->term, p->len)) {
			// Found a match!
			is_color_TERM = true;
			free(TERM_lower);
			return;
		}
	}

	// Match the entire string.
	struct match_whole_t {
		char term[8];
	};
	static const struct match_whole_t match_whole[4] = {
		{"color"},
		{"ansi"},
		{"cygwin"},
		{"linux"},
	};
	for (const struct match_whole_t *p = match_whole;
	     p != &match_whole[ARRAY_SIZE(match_whole)]; p++)
	{
		if (!strcmp(TERM_lower, p->term)) {
			// Found a match!
			is_color_TERM = true;
			free(TERM_lower);
			return;
		}
	}

	// Terminal does not support ANSI color.
	is_color_TERM = false;
	free(TERM_lower);
	return;
}

/**
 * Initialize console information for the specified file descriptor.
 * @param vt	[out] gsvt_console
 * @param f	[in] FILE*
 */
static void gsvt_init_posix(gsvt_console *vt, FILE *f)
{
	vt->stream = f;

	// Use isatty() to determine if this is a tty or a file.
	if (isatty(fileno(f))) {
		// Is a tty.
		vt->is_console = true;
		// If $TERM matches a valid ANSI color terminal, ANSI color is supported.
		vt->supports_ansi = is_color_TERM;
	} else {
		// Not a tty.
		vt->is_console = false;
		vt->supports_ansi = false;
	}
}

/**
 * Initialize VT detection.
 * Internal function; called by pthread_once().
 */
static void gsvt_init_int(void)
{
	// Initialize the gsvt_console variables using stdout/stderr.
	check_TERM_variable();
	gsvt_init_posix(&__gsvt_stdout, stdout);
	gsvt_init_posix(&__gsvt_stderr, stderr);
}

/**
 * Initialize VT detection.
 */
void gsvt_init(void)
{
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;
	pthread_once(&once_control, gsvt_init_int);
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
	// NOTE: Simple wrapper around fwrite().
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
	// NOTE: Simple wrapper around fputs().
	return fputs(s, vt->stream);
}

/**
 * fflush() wrapper function for gsvt_console.
 *
 * @param vt
 * @return 0 on success; negative POSIX error code on error.
 */
int gsvt_fflush(gsvt_console *vt)
{
	// NOTE: Simple wrapper around fflush().
	int ret = fflush(vt->stream);
	return (ret == 0) ? 0 : errno;
}

/** Convenience functions **/

/**
 * Print a newline to the specified gsvt_console.
 * @param vt
 */
void gsvt_newline(gsvt_console *vt)
{
	fputc('\n', vt->stream);
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
	if (!vt->is_console || !vt->supports_ansi) {
		// Not a console, or console does not support ANSI escape sequences.
		return;
	}

	assert(color < 8);
	color &= 0x07;

	fprintf(vt->stream, "\033[3%u%sm", color, (bold ? ";1" : ""));
}

/**
 * Reset the text color to its original value.
 * @param vt
 */
void gsvt_text_color_reset(gsvt_console *vt)
{
	if (!vt->is_console || !vt->supports_ansi) {
		// Not a console, or console does not support ANSI escape sequences.
		return;
	}

	static const char ansi_color_reset[] = "\033[0m";
	fwrite(ansi_color_reset, 1, sizeof(ansi_color_reset)-1, vt->stream);
}

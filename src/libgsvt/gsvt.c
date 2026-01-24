/***************************************************************************
 * ROM Properties Page shell extension. (libgsvt)                          *
 * gsvt.c: Virtual Terminal wrapper functions. (common functions)          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gsvt.h"
#include "gsvt_p.h"
#include "common.h"

// C includes
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ctypex.h"

// Is Sixel supported? (cached)
// -1 = not checked; 0 = not supported; 1 = supported
static int8_t sixel_supported = -1;

// Cached cell size (if TTY) [-1 if not initialized; 0 if not a TTY]
static int cell_size_w = -1;
static int cell_size_h = -1;

/**
 * Does the terminal support Sixel?
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_sixel(void)
{
	if (sixel_supported >= 0) {
		return !!sixel_supported;
	}
	sixel_supported = 0;

	// Query the device attributes.
	TCHAR buf[32];
	buf[0] = _T('\0');
	int ret = gsvt_query_tty("\x1B[c", buf, sizeof(buf));
	if (ret != 0) {
		// Error retrieving device attributes.
		return false;
	}

	// Returned string should start with "\x1B[?" and end with 'c'.
	if (_tmemcmp_inline(buf, _T("\x1B[?"), 3) != 0) {
		// Incorrect prefix.
		return false;
	}

	// In between the prefix and ending character, there should be a
	// semicolon-separated list of numeric values.
	// TODO: Combine common code in gsvt_win32.c for parsing numeric lists?
	const TCHAR *const p_end = &buf[sizeof(buf)];
	int num = -1;
	for (const TCHAR *p = &buf[3]; p < p_end; p++) {
		TCHAR chr = *p;
		if (isdigit_ascii(chr)) {
			// This is a digit.
			if (num < 0) {
				num = (chr & 0x0F);
			} else {
				num *= 10;
				num += (chr & 0x0F);
			}
		} else if (chr == _T(';')) {
			// If the value is 4, then Sixel is supported.
			if (num == 4) {
				sixel_supported = 1;
			}
			num = -1;
		} else if (chr == _T('c')) {
			// End of list.
			// Check the final value.
			// If the value is 4, then Sixel is supported.
			if (num == 4) {
				sixel_supported = 1;
			}
			break;
		} else {
			// Invalid character...
			sixel_supported = 0;
			return false;
		}
	}

	return !!sixel_supported;
}

/**
 * Get the size of a single character cell on the terminal.
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @param pWidth	[out] Character width
 * @param pHeight	[out] Character height
 * @return 0 on success; negative POSIX error code on error.
 */
int gsvt_get_cell_size(int *pWidth, int *pHeight)
{
	// Is the cell size cached already?
	if (cell_size_w >= 0) {
		// Cell size is cached.
		*pWidth = cell_size_w;
		*pHeight = cell_size_h;
		return (likely(cell_size_w > 0)) ? 0 : -ENOTTY;
	}

	// Attempt to get the cell size.
	TCHAR buf[16];
	buf[0] = '\0';
	int ret = gsvt_query_tty("\x1B[16t", buf, sizeof(buf));
	if (ret != 0) {
		// Error retrieving the cell size.
		// Assume the cell size is not available.
		cell_size_w = 0;
		cell_size_h = 0;
		return ret;
	}

	// Use sscanf() to verify the string.
	int start_code = 0;
	TCHAR end_code = '\0';
	int s = _stscanf(buf, _T("\x1B[%d;%d;%d%c"), &start_code, pHeight, pWidth, &end_code);

	ret = 0;
	if (s != 4 || start_code != 6 || end_code != _T('t')) {
		// Not valid...
		// Assume the cell size is not available.
		cell_size_w = 0;
		cell_size_h = 0;
		ret = -EIO;
	}

	// Retrieved the width and height.
	return ret;
}

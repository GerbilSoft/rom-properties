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

// Are Sixel or Kitty supported?
static bool checked_graphics = false;
static bool supports_sixel = false;
static bool supports_kitty = false;

// Cached cell size (if TTY) [-1 if not initialized; 0 if not a TTY]
static int cell_size_w = -1;
static int cell_size_h = -1;

/**
 * Check for Sixel and Kitty graphics protocol support.
 */
static void gsvt_check_graphics_protocol_support(void)
{
	if (checked_graphics) {
		// Should not happen...
		return;
	}
	supports_sixel = false;
	supports_kitty = false;
	checked_graphics = true;

	// Query both Kitty protocol support and the device attributes.
	// Reference: https://sw.kovidgoyal.net/kitty/graphics-protocol/#querying-support-and-available-transmission-mediums
	//
	// NOTE: On Windows, there doesn't seem to be a straight-forward way to
	// do a non-blocking read with ReadConsole() [and ReadConsoleInput() isn't
	// working properly for tty commands], so we have to specify an end char
	// to expect. We'll expect 'c' for Device Attributes, since any terminal
	// that supports Kitty also supports Device Attributes.
	//
	TCHAR buf[128];
	buf[0] = _T('\0');
#ifdef _WIN32
	// NOTE: Neither the Windows command prompt nor Windows Terminal currently
	// support Kitty, and attempting to query Kitty on the Windows command prompt
	// results in weird garbage appearing. Disable Kitty checks on Windows for now.
	// TODO: Do any Windows terminal emulators support Kitty?
	int ret = gsvt_query_tty("\x1B[c", buf, ARRAY_SIZE(buf), _T('c'));
#else /* !_WIN32 */
	int ret = gsvt_query_tty("\x1B_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\x1B\\\x1B[c", buf, ARRAY_SIZE(buf), _T('c'));
#endif /* _WIN32 */
	if (ret != 0) {
		// Error querying protocol support.
		return;
	}

	// If Kitty is supported, the Kitty protocol data will start with "\x1B_G" and end with "\x1B\\".
	// If Sixel is supported, the device attributes will start with "\x1B[?" and end with 'c'.

	const TCHAR *const p_end = &buf[ARRAY_SIZE(buf)];
	const TCHAR *p = buf;
	do {
		p = _tmemchr(p, _T('\x1B'), (p_end - p));
		if (!p) {
			// No more escape sequences.
			break;
		}

		// Check if this is a Kitty or device attributes response.
		p++;
		if (p[0] == _T('_') && p[1] == _T('G')) {
			// It's a Kitty response.
			// Kitty is supported.

			// The response ends with "\x1B\\".
			p += 2;
			p = _tmemchr(p, _T('\x1B'), (p_end - p));
			if (!p || p[1] != _T('\\')) {
				// End of response not found...
				break;
			}

			supports_kitty = true;
			p++;
		} else if (p[0] == _T('[') && p[1] == _T('?')) {
			// Device Attributes repsonse.
			// Parse the Device Attributes list and check for Sixel.
			int num = -1;
			for (p += 2; p < p_end; p++) {
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
						supports_sixel = true;
					}
					num = -1;
				} else if (chr == _T('c')) {
					// End of list.
					// Check the final value.
					// If the value is 4, then Sixel is supported.
					if (num == 4) {
						supports_sixel = true;
					}
					break;
				} else {
					// Invalid character...
					supports_sixel = false;
					break;
				}
			}
		}
	} while (p && p < p_end);
}

/**
 * Does the terminal support the Sixel graphics protocol?
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_sixel(void)
{
	if (!checked_graphics) {
		gsvt_check_graphics_protocol_support();
	}
	return supports_sixel;
}

/**
 * Does the terminal support the Kitty graphics protocol?
 * NOTE: Both stdin and stdout must be a tty for this function to succeed.
 * @return True if it does; false if it doesn't.
 */
bool gsvt_supports_kitty(void)
{
	if (!checked_graphics) {
		gsvt_check_graphics_protocol_support();
	}
	return supports_kitty;
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
	int ret = gsvt_query_tty("\x1B[16t", buf, sizeof(buf), _T('t'));
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

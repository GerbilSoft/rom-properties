/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpGtk.hpp"

// C++ STL classes
using std::string;

/**
 * Convert an RP file dialog filter to GTK+.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*:-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * NOTE: GTK+ doesn't use strings for file filters. Instead, it has
 * GtkFileFilter objects that are added to a GtkFileChooser.
 * To reduce overhead, the GtkFileChooser is passed to this function
 * so the GtkFileFilter objects can be added directly.
 *
 * @param fileChooser GtkFileChooser*
 * @param filter RP file dialog filter. (UTF-8, from gettext())
 * @return 0 on success; negative POSIX error code on error.
 */
int rpFileDialogFilterToGtk(GtkFileChooser *fileChooser, const char *filter)
{
	assert(fileChooser != nullptr);
	assert(filter != nullptr && filter[0] != '\0');
	if (!fileChooser || !filter || filter[0] == '\0')
		return -EINVAL;

	// Split the string.
	gchar **const strv = g_strsplit(filter, "|", 0);
	if (!strv)
		return -EINVAL;

	const gchar *const *pStrv = strv;
	do {
		// String indexes:
		// - 0: Display name
		// - 1: Pattern
		// - 2: MIME type (optional)

		// Separator 1: Between display name and pattern.
		assert(pStrv[0] != nullptr);
		if (!pStrv[0]) {
			// Missing token...
			g_strfreev(strv);
			return -EINVAL;
		}

		GtkFileFilter *const fileFilter = gtk_file_filter_new();
		gtk_file_filter_set_name(fileFilter, pStrv[0]);

		// Separator 2: Between pattern and MIME types.
		assert(pStrv[1] != nullptr);
		if (!pStrv[1]) {
			// Missing token...
			g_object_unref(fileFilter);
			g_strfreev(strv);
			return -EINVAL;
		}

		// Split the pattern. (';'-separated)
		gchar **const strv_ext = g_strsplit(pStrv[1], ";", 0);
		if (strv_ext) {
			for (const gchar *const *pStrvExt = strv_ext; *pStrvExt != nullptr; pStrvExt++) {
				gtk_file_filter_add_pattern(fileFilter, *pStrvExt);
			}
			g_strfreev(strv_ext);
		}

		// Separator 3: Between MIME types and the next display name.
		if (pStrv[2] && pStrv[2][0] != '-') {
			// Split the pattern. (';'-separated)
			gchar **const strv_mime = g_strsplit(pStrv[2], ";", 0);
			if (strv_mime) {
				for (const gchar *const *pStrvMime = strv_mime; *pStrvMime != nullptr; pStrvMime++) {
					gtk_file_filter_add_mime_type(fileFilter, *pStrvMime);
				}
				g_strfreev(strv_mime);
			}
		}

		// Add the GtkFileFilter.
		gtk_file_chooser_add_filter(fileChooser, fileFilter);

		if (!pStrv[2]) {
			// MIME type token is missing. We're done here.
			break;
		}

		// Next set of 3 tokens.
		pStrv += 3;
	} while (*pStrv != nullptr);

	g_strfreev(strv);
	return 0;
}

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
string convert_accel_to_gtk(const char *str)
{
	// GTK+ uses '_' for accelerators, not '&'.
	string s_ret = str;
	const size_t accel_pos = s_ret.find('&');
	if (accel_pos != string::npos) {
		s_ret[accel_pos] = '_';
	}
	return s_ret;
}

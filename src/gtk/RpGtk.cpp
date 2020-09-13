/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpGtk.hpp"

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
 * @return Qt file dialog filter.
 */
int rpFileDialogFilterToGtk(GtkFileChooser *fileChooser, const char *filter)
{
	assert(fileChooser != nullptr);
	assert(filter != nullptr && filter[0] != '\0');
	if (!fileChooser || !filter || filter[0] == '\0')
		return -EINVAL;

	// Temporary string so we can use strtok_r().
	gchar *const tmpfilter = g_strdup(filter);
	assert(tmpfilter != nullptr);
	gchar *saveptr = nullptr;

	// First strtok_r() call.
	char *token = strtok_r(tmpfilter, "|", &saveptr);
	do {
		GtkFileFilter *const fileFilter = gtk_file_filter_new();

		// Separator 1: Between display name and pattern.
		// (strtok_r() call was done in the previous iteration.)
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			g_object_unref(fileFilter);
			g_free(tmpfilter);
			return -EINVAL;
		}
		gtk_file_filter_set_name(fileFilter, token);

		// Separator 2: Between pattern and MIME types.
		token = strtok_r(nullptr, "|", &saveptr);
		assert(token != nullptr);
		if (!token) {
			// Missing token...
			g_object_unref(fileFilter);
			g_free(tmpfilter);
			return -EINVAL;
		}

		// Split the pattern. (';'-separated)
		char *saveptr2 = nullptr;
		const char *token2 = strtok_r(token, ";", &saveptr2);
		while (token2 != nullptr) {
			gtk_file_filter_add_pattern(fileFilter, token2);
			token2 = strtok_r(nullptr, ";", &saveptr2);
		}

		// Separator 3: Between MIME types and the next display name.
		// NOTE: strtok_r() skips empty fields. The special indicator
		// "-" will be used to indicate "no MIME types".
		token = strtok_r(nullptr, "|", &saveptr);
		if (token && !(token[0] == '-' && token[1] == ' ')) {
			// Split the pattern. (';'-separated)
			saveptr2 = nullptr;
			token2 = strtok_r(token, ";", &saveptr2);
			while (token2 != nullptr) {
				gtk_file_filter_add_mime_type(fileFilter, token2);
				token2 = strtok_r(nullptr, ";", &saveptr2);
			}
		}

		// Add the GtkFileFilter.
		gtk_file_chooser_add_filter(fileChooser, fileFilter);

		// Next token.
		token = strtok_r(nullptr, "|", &saveptr);
	} while (token != nullptr);

	g_free(tmpfilter);
	return 0;
}

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
 * Convert an RP file dialog filter to GtkFileFilter objects.
 *
 * Internal function used by both the GtkFileChooserDialog version
 * and the GtkFileDialog (GTK 4.10) version.
 *
 * The RP file dialog filter must have been split using g_strsplit().
 *
 * @param pStrv Pointer to current position in strv
 * @return GtkFileFilter, or nullptr on error.
 */
static GtkFileFilter *rpFileFilterToGtk_int(const gchar *const *pStrv)
{
	// String indexes:
	// - 0: Display name
	// - 1: Pattern
	// - 2: MIME type (optional)
	assert(pStrv[0] != nullptr);
	assert(pStrv[1] != nullptr);
	if (!pStrv[0] || !pStrv[1]) {
		// Missing token...
		return nullptr;
	}

	GtkFileFilter *const fileFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(fileFilter, pStrv[0]);

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

	// Return the GtkFileFilter.
	return fileFilter;
}

// TODO: Consolidate the GTK2/GTK3 and GTK4 filter functions.

/**
 * Convert an RP file dialog filter to GTK2/GTK3 for GtkFileChooser.
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
 * @param filter RP file dialog filter (UTF-8, from gettext())
 * @return 0 on success; negative POSIX error code on error
 */
int rpFileFilterToGtkFileChooser(GtkFileChooser *fileChooser, const char *filter)
{
	assert(fileChooser != nullptr);
	assert(filter != nullptr && filter[0] != '\0');
	if (!fileChooser || !filter || filter[0] == '\0')
		return -EINVAL;

	// Split the string.
	gchar **const strv = g_strsplit(filter, "|", 0);
	if (!strv)
		return -EINVAL;

	int ret = 0;
	const gchar *const *pStrv = strv;
	do {
		// String indexes:
		// - 0: Display name
		// - 1: Pattern
		// - 2: MIME type (optional)
		GtkFileFilter *const fileFilter = rpFileFilterToGtk_int(pStrv);
		if (!fileFilter) {
			// Parse error...
			ret = -EINVAL;
			break;
		}

		// Add the GtkFileFilter to the GtkFileChooserDialog.
		gtk_file_chooser_add_filter(fileChooser, fileFilter);

		if (!pStrv[2]) {
			// MIME type token is missing. We're done here.
			break;
		}

		// Next set of 3 tokens.
		pStrv += 3;
	} while (*pStrv != nullptr);

	g_strfreev(strv);
	return ret;
}

#if GTK_CHECK_VERSION(4,9,1)
/**
 * Convert an RP file dialog filter to GTK4 for GtkFileDialog.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * NOTE: GTK+ doesn't use strings for file filters. Instead, a
 * GListModel of GtkFileFilter objects is added to a GtkFileDialog.
 * To reduce overhead, the GtkFileDialog is passed to this function
 * so the GtkFileFilter objects can be added directly.
 *
 * @param fileDialog GtkFileDialog*
 * @param filter RP file dialog filter (UTF-8, from gettext())
 * @return 0 on success; negative POSIX error code on error.
 */
int rpFileFilterToGtkFileDialog(GtkFileDialog *fileDialog, const char *filter)
{
	assert(fileDialog != nullptr);
	assert(filter != nullptr && filter[0] != '\0');
	if (!fileDialog || !filter || filter[0] == '\0')
		return -EINVAL;

	// Create a GListStore for GtkFileFilters.
	GListStore *const listStore = g_list_store_new(GTK_TYPE_FILE_FILTER);

	// Split the string.
	gchar **const strv = g_strsplit(filter, "|", 0);
	if (!strv) {
		g_object_unref(listStore);
		return -EINVAL;
	}

	int ret = 0;
	const gchar *const *pStrv = strv;
	do {
		// String indexes:
		// - 0: Display name
		// - 1: Pattern
		// - 2: MIME type (optional)
		GtkFileFilter *const fileFilter = rpFileFilterToGtk_int(pStrv);
		if (!fileFilter) {
			// Parse error...
			ret = -EINVAL;
			break;
		}

		// Add the GtkFileFilter to the GListStore.
		g_list_store_append(listStore, fileFilter);

		if (!pStrv[2]) {
			// MIME type token is missing. We're done here.
			break;
		}

		// Next set of 3 tokens.
		pStrv += 3;
	} while (*pStrv != nullptr);

	// Set the GtkFileDialog's filters.
	gtk_file_dialog_set_filters(fileDialog, G_LIST_MODEL(listStore));
	// fileDialog takes a reference to listStore.
	g_object_unref(listStore);

	g_strfreev(strv);
	return ret;
}
#endif /* GTK_CHECK_VERSION(4,9,1) */

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

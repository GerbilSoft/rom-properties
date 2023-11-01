/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQt.cpp: Qt wrappers for some libromdata functionality.                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpGtk.hpp"
#include "gtk-i18n.h"

// Use the new GtkFileDialog class on GTK 4.10 and later.
#if GTK_CHECK_VERSION(4,9,1)
#  define USE_GTK4_FILE_DIALOG 1
#endif /* GTK_CHECK_VERSION(4,9,1) */

// C++ STL classes
using std::string;

// Simple struct for passing multiple values to the rpGtk_getFileName_int() callback functions.
typedef struct _rpGtk_getFileName_int_callback_data_t {
	rpGtk_fileDialogCallback callback;
	gpointer user_data;
	bool bSave;
} rpGtk_getFileName_int_callback_data_t;

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

#ifndef USE_GTK4_FILE_DIALOG
/**
 * Convert an RP file dialog filter to GTK2/GTK3 for GtkFileChooser.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*|-"
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
static int rpFileFilterToGtkFileChooser(GtkFileChooser *fileChooser, const char *filter)
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
#endif /* !USE_GTK4_FILE_DIALOG */

#if USE_GTK4_FILE_DIALOG
/**
 * Convert an RP file dialog filter to GTK4 for GtkFileDialog.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*|-"
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
static int rpFileFilterToGtkFileDialog(GtkFileDialog *fileDialog, const char *filter)
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

/**
 * Callback from rpGtk_getFileName_int(). (GtkFileDialog version)
 * @param fileDialog GtkFileDialog
 * @param res GAsyncResult
 * @param gfncbdata Callback data (user_data)
 */
static void
rpGtk_getFileName_AsyncReadyCallback(GtkFileDialog *fileDialog, GAsyncResult *res, rpGtk_getFileName_int_callback_data_t *gfncbdata)
{
	GFile *file;
	if (gfncbdata->bSave) {
		file = gtk_file_dialog_save_finish(fileDialog, res, nullptr);
	} else {
		file = gtk_file_dialog_open_finish(fileDialog, res, nullptr);
	}
	g_object_unref(fileDialog);

	// Run the callback.
	// NOTE: Callback function takes ownership of the GFile.
	gfncbdata->callback(file, gfncbdata->user_data);
	g_free(gfncbdata);
}
#else /* !USE_GTK4_FILE_DIALOG */
/**
 * Callback from rpGtk_getFileName_int(). (GtkFileChooserDialog version)
 * @param fileDialog GtkFileChooserDialog
 * @param response_id Response ID
 * @param gfncbdata Callback data (user_data)
 */
static void
rpGtk_getFileName_fileDialog_response(GtkFileChooserDialog *fileDialog, gint response_id, rpGtk_getFileName_int_callback_data_t *gfncbdata)
{
	GFile *file = nullptr;
	if (response_id == GTK_RESPONSE_ACCEPT) {
		// Get the GFile from the dialog.
		file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(fileDialog));
	}

	// Dialog is no longer needed.
#if GTK_CHECK_VERSION(4,0,0)
	gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_destroy(GTK_WIDGET(fileDialog));
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Run the callback.
	// NOTE: Callback function takes ownership of the GFile.
	gfncbdata->callback(file, gfncbdata->user_data);
	g_free(gfncbdata);
}
#endif /* !USE_GTK4_FILE_DIALOG */

/**
 * Prompt the user to open or save a file. (internal function)
 *
 * The dialog is opened as modal, but is handled asynchronously.
 * The callback function is run when the dialog is closed.
 *
 * @param gfndata (in): rpGtk_getFileName_t
 * @param bSave (in): True for save; false for open
 * @return 0 on success; negative POSIX error code on error
 */
static int rpGtk_getFileName_int(const rpGtk_getFileName_t *gfndata, bool bSave)
{
#if USE_GTK4_FILE_DIALOG
	// GTK 4.10.0 introduces a new GtkFileDialog.
	GtkFileDialog *const fileDialog = gtk_file_dialog_new();
	if (gfndata->title) {
		gtk_file_dialog_set_title(fileDialog, gfndata->title);
	}
#else /* !USE_GTK4_FILE_DIALOG */
	GtkFileChooserAction accept_action;
	const char *accept_text;
	if (bSave) {
		accept_action = GTK_FILE_CHOOSER_ACTION_SAVE;
		accept_text = GTK_I18N_STR_SAVE;
	} else {
		accept_action = GTK_FILE_CHOOSER_ACTION_OPEN;
		accept_text = GTK_I18N_STR_OPEN;
	}
	GtkWidget *const fileDialog = gtk_file_chooser_dialog_new(
		gfndata->title,			// title
		gfndata->parent,		// parent
		accept_action,			// action
		GTK_I18N_STR_CANCEL, GTK_RESPONSE_CANCEL,
		accept_text, GTK_RESPONSE_ACCEPT,
		nullptr);
	gtk_widget_set_name(fileDialog, "rpGtk_getFileName");
#endif /* USE_GTK4_FILE_DIALOG */

#if GTK_CHECK_VERSION(4,0,0)
	// GTK4, GtkFileChooserDialog and/or GtkFileDialog
	// Set the initial folder. (A GFile is required.)
	if (gfndata->init_dir) {
		GFile *const set_file = g_file_new_for_path(gfndata->init_dir);
		if (set_file) {
#  if USE_GTK4_FILE_DIALOG
			gtk_file_dialog_set_initial_folder(fileDialog, set_file);
#  else /* !USE_GTK4_FILE_DIALOG */
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), set_file, nullptr);
#  endif /* !USE_GTK4_FILE_DIALOG */
			g_object_unref(set_file);
		}
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	// GTK2/GTK3: Require overwrite confirmation. (save dialogs only)
	// NOTE: GTK4 has *mandatory* overwrite confirmation.
	// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12
	if (bSave) {
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fileDialog), true);
	}

	// Set the initial folder.
	if (gfndata->init_dir) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), gfndata->init_dir);
	}
#endif

	// Set the initial name.
	if (gfndata->init_name) {
#  if USE_GTK4_FILE_DIALOG
		gtk_file_dialog_set_initial_name(fileDialog, gfndata->init_name);
#  else /* !USE_GTK4_FILE_DIALOG */
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileDialog), gfndata->init_name);
#  endif /* !USE_GTK4_FILE_DIALOG */
	}

	// Set the file filters.
	if (gfndata->filter) {
#if USE_GTK4_FILE_DIALOG
		rpFileFilterToGtkFileDialog(fileDialog, gfndata->filter);
#else /* !USE_GTK4_FILE_DIALOG */
		rpFileFilterToGtkFileChooser(GTK_FILE_CHOOSER(fileDialog), gfndata->filter);
#endif /* USE_GTK4_FILE_DIALOG */
	}

	// Data for the GtkFileChooserDialog/GtkFileDialog callback function.
	rpGtk_getFileName_int_callback_data_t *const gfncbdata =
		static_cast<rpGtk_getFileName_int_callback_data_t*>(g_malloc(sizeof(*gfncbdata)));
	gfncbdata->callback = gfndata->callback;
	gfncbdata->user_data = gfndata->user_data;
	gfncbdata->bSave = bSave;

	// Prompt for a filename.
#if USE_GTK4_FILE_DIALOG
	gtk_file_dialog_set_modal(fileDialog, true);
	if (bSave) {
		gtk_file_dialog_save(fileDialog, gfndata->parent, nullptr,
			(GAsyncReadyCallback)rpGtk_getFileName_AsyncReadyCallback, gfncbdata);
	} else {
		gtk_file_dialog_open(fileDialog, gfndata->parent, nullptr,
			(GAsyncReadyCallback)rpGtk_getFileName_AsyncReadyCallback, gfncbdata);
	}
#else /* !USE_GTK4_FILE_DIALOG */
	g_signal_connect(fileDialog, "response", G_CALLBACK(rpGtk_getFileName_fileDialog_response), gfncbdata);
	gtk_window_set_transient_for(GTK_WINDOW(fileDialog), gfndata->parent);
	gtk_window_set_modal(GTK_WINDOW(fileDialog), true);
	gtk_widget_set_visible(GTK_WIDGET(fileDialog), true);
#endif /* !USE_GTK4_FILE_DIALOG */

	return 0;
}

/**
 * Prompt the user to open a file.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * The dialog is opened as modal, but is handled asynchronously.
 * The callback function is run when the dialog is closed.
 *
 * @param gfndata (in): rpGtk_getFileName_t
 * @return 0 on success; negative POSIX error code on error
 */
int rpGtk_getOpenFileName(const rpGtk_getFileName_t *gfndata)
{
	return rpGtk_getFileName_int(gfndata, false);
}

/**
 * Prompt the user to save a file.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*|-"
 * Similar the same as Windows, but with '|' instead of '\0'.
 * Also, no terminator sequence is needed.
 * The "(*.bin; *.srl)" part is added to the display name if needed.
 * A third segment provides for semicolon-separated MIME types. (May be "-" for 'any'.)
 *
 * The dialog is opened as modal, but is handled asynchronously.
 * The callback function is run when the dialog is closed.
 *
 * @param gfndata (in): rpGtk_getFileName_t
 * @return 0 on success; negative POSIX error code on error
 */
int rpGtk_getSaveFileName(const rpGtk_getFileName_t *gfndata)
{
	return rpGtk_getFileName_int(gfndata, true);
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

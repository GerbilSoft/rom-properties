/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpGtk.h: glib/gtk+ wrappers for some libromdata functionality.          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

// GTK+ major version.
// We can't simply use GTK_MAJOR_VERSION because
// that has parentheses.
#if GTK_CHECK_VERSION(5, 0, 0)
#  error Needs updating for GTK5.
#elif GTK_CHECK_VERSION(4, 0, 0)
#  define GTK_MAJOR_STR "4"
#elif GTK_CHECK_VERSION(3, 0, 0)
#  define GTK_MAJOR_STR "3"
#elif GTK_CHECK_VERSION(2, 0, 0)
#  define GTK_MAJOR_STR "2"
#else
#  error GTK+ is too old.
#endif

/**
 * File dialog callback function.
 * @param file (in) (transfer full): Selected file, or NULL if no file was selected
 * @param user_data (in) (transfer full): Specified user data when calling the original function
 */
typedef void (*rpGtk_fileDialogCallback)(GFile *file, gpointer user_data);

/**
 * File dialog data struct
 */
typedef struct _rpGtk_getFileName_t {
	GtkWindow *parent;			// (nullable) Parent window to set as modal
	const char *title;			// Dialog title
	const char *filter;			// RP file dialog filter (UTF-8, from gettext())
	const char *init_dir;			// (nullable) Initial directory
	const char *init_name;			// (nullable) Initial name
	rpGtk_fileDialogCallback callback;	// Callback function
	gpointer user_data;			// User data for the callback function
} rpGtk_getFileName_t;

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
int rpGtk_getOpenFileName(const rpGtk_getFileName_t *gfndata);

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
int rpGtk_getSaveFileName(const rpGtk_getFileName_t *gfndata);

G_END_DECLS

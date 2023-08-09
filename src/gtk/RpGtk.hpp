/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpGtk.hpp: glib/gtk+ wrappers for some libromdata functionality.        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * Convert an RP file dialog filter to GTK2/GTK3 for GtkFileChooserDialog.
 *
 * RP syntax: "Sega Mega Drive ROM images|*.gen;*.bin|application/x-genesis-rom|All Files|*.*|-"
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
 * @return 0 on success; negative POSIX error code on error.
 */
int rpFileChooserDialogFilterToGtk(GtkFileChooser *fileChooser, const char *filter);

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
int rpFileDialogFilterToGtk(GtkFileDialog *fileDialog, const char *filter);
#endif /* GTK_CHECK_VERSION(4,9,1) */

G_END_DECLS

#ifdef __cplusplus

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
std::string convert_accel_to_gtk(const char *str);

#endif /* __cplusplus */

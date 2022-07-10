/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpGtk.hpp: glib/gtk+ wrappers for some libromdata functionality.        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_RPGTK_HPP__
#define __ROMPROPERTIES_GTK_RPGTK_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * Convert an RP file dialog filter to GTK+.
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
int rpFileDialogFilterToGtk(GtkFileChooser *fileChooser, const char8_t *filter);

G_END_DECLS

#ifdef __cplusplus

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
std::string convert_accel_to_gtk(const char *str);

/**
 * Convert Win32/Qt-style accelerator notation ('&') to GTK-style ('_').
 * NOTE: GTK+ uses char*, so we're returning an std::string, not std::u8string.
 * @param str String with '&' accelerator
 * @return String with '_' accelerator
 */
static inline std::string convert_accel_to_gtk(const char8_t *str)
{
	return convert_accel_to_gtk(reinterpret_cast<const char*>(str));
}

#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_GTK_RPGTK_HPP__ */

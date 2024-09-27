/***************************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                                     *
 * nautilus-extension-mini.h: nautilus-extension struct definitions for compatibility. *
 *                                                                                     *
 * Copyright (c) 2017-2023 by David Korth.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                                           *
 ***************************************************************************************/

#pragma once

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// Struct definitions from libnautilus-extension.
// GTK3 version; Nautilus 42 and older.

// N.B.: This interface is removed in Nautilus 43 (GTK4).
// Replaced by NautilusPropertiesModelProviderInterface.
struct _NautilusPropertyPageProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_pages) (NautilusPropertyPageProvider *provider,
	                     GList                        *files);
};
typedef struct _NautilusPropertyPageProviderInterface NautilusPropertyPageProviderInterface;

// N.B.: This interface is changed in Nautilus 43 (GTK4).
struct _NautilusMenuProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_file_items)       (NautilusMenuProvider *provider,
					GtkWidget            *window,
					GList                *files);
	GList *(*get_background_items) (NautilusMenuProvider *provider,
					GtkWidget            *window,
					NautilusFileInfo     *current_folder);
};
typedef struct _NautilusMenuProviderInterface NautilusMenuProviderInterface;

// Nemo: Name and description provider interface
// For the plugin manager.
struct _NemoNameAndDescProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_name_and_desc) (NemoNameAndDescProvider *provider);
};
typedef struct _NemoNameAndDescProviderInterface NemoNameAndDescProviderInterface;

G_END_DECLS

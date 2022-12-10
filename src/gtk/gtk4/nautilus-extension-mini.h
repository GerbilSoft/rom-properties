/***************************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * nautilus-extension-mini.h: nautilus-extension struct definitions for compatibility. *
 *                                                                                     *
 * Copyright (c) 2017-2022 by David Korth.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                                           *
 ***************************************************************************************/

#ifndef __ROMPROPERTIES_GTK4_NAUTILUS_EXTENSION_MINI_H__
#define __ROMPROPERTIES_GTK4_NAUTILUS_EXTENSION_MINI_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// Struct definitions from libnautilus-extension.
// GTK4 version: Nautilus 43 and newer.

struct _NautilusPropertiesModelProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_models) (NautilusPropertiesModelProvider *provider,
	                      GList                           *files);
};
typedef struct _NautilusPropertiesModelProviderInterface NautilusPropertiesModelProviderInterface;

struct _NautilusMenuProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_file_items)       (NautilusMenuProvider *provider,
					GList                *files);
	GList *(*get_background_items) (NautilusMenuProvider *provider,
					NautilusFileInfo     *current_folder);
};
typedef struct _NautilusMenuProviderInterface NautilusMenuProviderInterface;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK4_NAUTILUS_EXTENSION_MINI_H__ */

/***************************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                                     *
 * nautilus-extension-mini.h: nautilus-extension struct definitions for compatibility. *
 *                                                                                     *
 * Copyright (c) 2017-2022 by David Korth.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                                           *
 ***************************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_NAUTILUS_EXTENSION_MINI_H__
#define __ROMPROPERTIES_GTK3_NAUTILUS_EXTENSION_MINI_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// Struct definitions from libnautilus-extension.
// GTK3 version; Nautilus 42 and older.

struct _NautilusPropertyPageProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_pages) (NautilusPropertyPageProvider *provider,
	                     GList                        *files);
};

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK3_NAUTILUS_EXTENSION_MINI_H__ */

/***************************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                                     *
 * nautilus-extension-mini.h: nautilus-extension struct definitions for compatibility. *
 *                                                                                     *
 * Copyright (c) 2017-2024 by David Korth.                                             *
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

	GList *(*get_pages)	(NautilusPropertyPageProvider *provider,
				 GList                        *files);
};
typedef struct _NautilusPropertyPageProviderInterface NautilusPropertyPageProviderInterface;

// N.B.: This interface is changed in Nautilus 43 (GTK4).
struct _NautilusMenuProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_file_items)	(NautilusMenuProvider *provider,
					 GtkWidget            *window,
					 GList                *files);
	GList *(*get_background_items)	(NautilusMenuProvider *provider,
					 GtkWidget            *window,
					 NautilusFileInfo     *current_folder);
};
typedef struct _NautilusMenuProviderInterface NautilusMenuProviderInterface;

struct _NautilusInfoProviderInterface
{
	GTypeInterface g_iface;

	NautilusOperationResult (*update_file_info)	(NautilusInfoProvider     *provider,
							 NautilusFileInfo         *file,
							 GClosure                 *update_complete,
							 NautilusOperationHandle **handle);
	void                    (*cancel_update)	(NautilusInfoProvider     *provider,
							 NautilusOperationHandle  *handle);
};
typedef struct _NautilusInfoProviderInterface NautilusInfoProviderInterface;

G_END_DECLS

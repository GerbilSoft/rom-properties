/***************************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
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

struct _NautilusColumnProviderInterface
{
	GTypeInterface g_iface;

	GList *(*get_columns) (NautilusColumnProvider *provider);
};
typedef struct _NautilusColumnProviderInterface NautilusColumnProviderInterface;

G_END_DECLS

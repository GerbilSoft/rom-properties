/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * thunarx-mini.h: ThunarX struct definitions for compatibility.           *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// Struct definitions from thunarx.
// Compatible with:
// - Thunar 1.6.12
// - Thunar 1.8.14

struct _ThunarxPropertyPageProviderIface {
	/*< private >*/
	GTypeInterface __parent__;

	/*< public >*/
	GList *(*get_pages) (ThunarxPropertyPageProvider *provider,
			     GList                       *files);

	/*< private >*/
	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);
	void (*reserved5) (void);
};
typedef struct _ThunarxPropertyPageProviderIface ThunarxPropertyPageProviderIface;

// N.B. This interface is essentially the same on GTK2 as on GTK3,
// but on GTK3, the functions return ThunarxMenuItems, whereas on
// GTK2, they return GtkActions.
struct _ThunarxMenuProviderIface {
	/*< private >*/
	GTypeInterface __parent__;

	/*< public >*/
	GList *(*get_file_menu_items)    (ThunarxMenuProvider *provider,
					  GtkWidget           *window,
					  GList               *files);

	GList *(*get_folder_menu_items)  (ThunarxMenuProvider *provider,
					  GtkWidget           *window,
					  ThunarxFileInfo     *folder);

	GList *(*get_dnd_menu_items)     (ThunarxMenuProvider *provider,
					  GtkWidget           *window,
					  ThunarxFileInfo     *folder,
					  GList               *files);

	/*< private >*/
	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
};
typedef struct _ThunarxMenuProviderIface ThunarxMenuProviderIface;

G_END_DECLS

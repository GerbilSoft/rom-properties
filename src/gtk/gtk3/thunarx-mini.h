/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * thunarx-mini.h: ThunarX struct definitions for compatibility.           *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK3_THUNARX_MINI_H__
#define __ROMPROPERTIES_GTK3_THUNARX_MINI_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

// From thunarx/thunarx-property-page-provider.h
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

// From thunarx/thunarx-property-page.h
// Compatible with:
// - Thunar 1.6.12
// - Thunar 1.8.14
struct _ThunarxPropertyPageClass {
	GtkBinClass __parent__;

	/*< private >*/
	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);
};
typedef struct _ThunarxPropertyPageClass ThunarxPropertyPageClass;

// From thunarx/thunarx-property-page.h
struct _ThunarxPropertyPagePrivate;
typedef struct _ThunarxPropertyPagePrivate ThunarxPropertyPagePrivate;

// From thunarx/thunarx-property-page.h
// Compatible with:
// - Thunar 1.6.12
// - Thunar 1.8.14
struct _ThunarxPropertyPage {
	GtkBin __parent__;

	/*< private >*/
	ThunarxPropertyPagePrivate *priv;
};
typedef struct _ThunarxPropertyPage ThunarxPropertyPage;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK3_THUNARX_MINI_H__ */

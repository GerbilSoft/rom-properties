/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpConfigTab.h: Configuration tab interface.                             *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CONFIG_RPCONFIGTAB_HPP__
#define __ROMPROPERTIES_GTK_CONFIG_RPCONFIGTAB_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RP_CONFIG_TYPE_TAB	(rp_config_tab_get_type())

G_DECLARE_INTERFACE(RpConfigTab, rp_config_tab, RP_CONFIG, TAB, GObject)

typedef struct _RpConfigTabInterface {
	GTypeInterface parent_iface;

	gboolean (*has_defaults)(RpConfigTab *tab);

	void (*reset)(RpConfigTab *tab);
	void (*load_defaults)(RpConfigTab *tab);
	void (*save)(RpConfigTab *tab, GKeyFile *keyFile);
} RpConfigTabInterface;

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		rp_config_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		rp_config_tab_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

gboolean	rp_config_tab_has_defaults	(RpConfigTab *tab) G_GNUC_INTERNAL;

void		rp_config_tab_reset		(RpConfigTab *tab) G_GNUC_INTERNAL;
void		rp_config_tab_load_defaults	(RpConfigTab *tab) G_GNUC_INTERNAL;
void		rp_config_tab_save		(RpConfigTab *tab, GKeyFile *keyFile) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_CONFIG_RPCONFIGTAB_HPP__ */

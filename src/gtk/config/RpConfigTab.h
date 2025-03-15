/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpConfigTab.h: Configuration tab interface.                             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#if !GTK_CHECK_VERSION(2, 91, 0)
// GTK2 needs a GtkAlignment widget to add padding.
#  define RP_USE_GTK_ALIGNMENT 1
#endif

#define RP_TYPE_CONFIG_TAB (rp_config_tab_get_type())

G_DECLARE_INTERFACE(RpConfigTab, rp_config_tab, RP, CONFIG_TAB, GObject)

typedef struct _RpConfigTabInterface {
	GTypeInterface parent_iface;

	gboolean (*has_defaults)(RpConfigTab *tab);

	void (*reset)(RpConfigTab *tab);
	void (*load_defaults)(RpConfigTab *tab);		// can be NULL
	void (*save)(RpConfigTab *tab, GKeyFile *keyFile);
} RpConfigTabInterface;

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
GType		rp_config_tab_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;

gboolean	rp_config_tab_has_defaults	(RpConfigTab *tab) G_GNUC_INTERNAL;

void		rp_config_tab_reset		(RpConfigTab *tab) G_GNUC_INTERNAL;
void		rp_config_tab_load_defaults	(RpConfigTab *tab) G_GNUC_INTERNAL;
void		rp_config_tab_save		(RpConfigTab *tab, GKeyFile *keyFile) G_GNUC_INTERNAL;

G_END_DECLS

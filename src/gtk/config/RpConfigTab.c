/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RpConfigTab.c: Configuration tab interface.                             *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpConfigTab.h"

/* Signal identifiers */
typedef enum {
	SIGNAL_MODIFIED,	// Tab was modified

	SIGNAL_LAST
} RpConfigTabSignalID;
static guint signals[SIGNAL_LAST];

G_DEFINE_INTERFACE(RpConfigTab, rp_config_tab, G_TYPE_OBJECT);

static void
rp_config_tab_default_init(RpConfigTabInterface *iface)
{
	// TODO: "has-defaults" property?
	RP_UNUSED(iface);

	/** Signals **/
	signals[SIGNAL_MODIFIED] = g_signal_new("modified",
		RP_TYPE_CONFIG_TAB,
		G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL,
		G_TYPE_NONE, 0);
}

/** Interface functions **/

gboolean rp_config_tab_has_defaults(RpConfigTab *tab)
{
	g_return_val_if_fail(RP_IS_CONFIG_TAB(tab), false);

	// Assume tabs have the "Defaults" button by default if the function isn't defined.
	RpConfigTabInterface *const iface = RP_CONFIG_TAB_GET_IFACE(tab);
	return (iface->has_defaults != NULL) ? iface->has_defaults(tab) : TRUE;
}

void rp_config_tab_reset(RpConfigTab *tab)
{
	g_return_if_fail(RP_IS_CONFIG_TAB(tab));

	RpConfigTabInterface *const iface = RP_CONFIG_TAB_GET_IFACE(tab);
	assert(iface->reset != NULL);
	g_return_if_fail(iface->reset != NULL);
	return iface->reset(tab);
}

void rp_config_tab_load_defaults(RpConfigTab *tab)
{
	g_return_if_fail(RP_IS_CONFIG_TAB(tab));

	// NOTE: load_defaults *can* be NULL.
	RpConfigTabInterface *const iface = RP_CONFIG_TAB_GET_IFACE(tab);
	if (iface->load_defaults) {
		iface->load_defaults(tab);
	}
}

void rp_config_tab_save(RpConfigTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_CONFIG_TAB(tab));

	RpConfigTabInterface *const iface = RP_CONFIG_TAB_GET_IFACE(tab);
	assert(iface->save != NULL);
	g_return_if_fail(iface->save != NULL);
	return iface->save(tab, keyFile);
}

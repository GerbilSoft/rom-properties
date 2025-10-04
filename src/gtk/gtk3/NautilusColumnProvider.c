/*****************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                           *
 * NautilusColumnProvider.c: Nautilus (and forks) Column Provider Definition *
 *                                                                           *
 * Copyright (c) 2017-2025 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

// Reference: https://github.com/xfce-mirror/thunar-archive-plugin/blob/master/thunar-archive-plugin/tap-provider.c

#include "stdafx.h"
#include "NautilusColumnProvider.h"
#include "NautilusExtraInterfaces.h"

// nautilus-extension.h mini replacement
#if GTK_CHECK_VERSION(4, 0, 0)
#  include "../gtk4/NautilusPlugin.hpp"
#  include "../gtk4/nautilus-extension-mini.h"
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
#  include "NautilusPlugin.hpp"
#  include "nautilus-extension-mini.h"
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

static void rp_nautilus_column_provider_interface_init(NautilusColumnProviderInterface *iface);

static GList*
rp_nautilus_column_provider_get_columns(NautilusColumnProvider *provider);

struct _RpNautilusColumnProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusColumnProvider {
	GObject __parent__;
};

// Array of column data we provide.
// Exported here so it can be shared with NautilusInfoProvider.
// (Uses NULL termination.)
const struct rp_nautilus_column_provider_column_desc_data_t rp_nautilus_column_provider_column_desc_data[] = {
	{"rp-game-id",		"Game ID"},
	{"rp-title-id",		"Title ID"},
	{"rp-media-id",		"Media ID"},
	{"rp-os-version",	"OS Version"},
	{"rp-encryption-key",	"Encryption Key"},
	{"rp-pixel-format",	"Pixel Format"},
	{"rp-region-code",	"Region Code"},
	{"rp-category",		"Category"},

	{NULL,			NULL}
};

#if !GLIB_CHECK_VERSION(2, 59, 1)
#  if defined(__GNUC__) && __GNUC__ >= 8
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif /* !GLIB_CHECK_VERSION(2, 59, 1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusColumnProvider, rp_nautilus_column_provider,
	G_TYPE_OBJECT, (GTypeFlags)0,
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_COLUMN_PROVIDER, rp_nautilus_column_provider_interface_init)
);

#if !GLIB_CHECK_VERSION(2, 59, 1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2, 59, 1) */

void
rp_nautilus_column_provider_register_type_ext(GTypeModule *g_module)
{
	rp_nautilus_column_provider_register_type(G_TYPE_MODULE(g_module));

#ifdef HAVE_EXTRA_INTERFACES
	// Add extra fork-specific interfaces.
	rp_nautilus_extra_interfaces_add(g_module, RP_TYPE_NAUTILUS_COLUMN_PROVIDER);
#endif /* HAVE_EXTRA_INTERFACES */
}

static void
rp_nautilus_column_provider_class_init(RpNautilusColumnProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_column_provider_class_finalize(RpNautilusColumnProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_column_provider_init(RpNautilusColumnProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_column_provider_interface_init(NautilusColumnProviderInterface *iface)
{
	iface->get_columns = rp_nautilus_column_provider_get_columns;
}

static GList*
rp_nautilus_column_provider_get_columns(NautilusColumnProvider *provider)
{
	g_return_val_if_fail(RP_IS_NAUTILUS_COLUMN_PROVIDER(provider), NULL);

	// Create columns.
	// NOTE: Creating the list in reverse, since g_list_prepend() is faster than g_list_append().
	GList *list = NULL;
	for (int i = ARRAY_SIZE_I(rp_nautilus_column_provider_column_desc_data)-2; i >= 0; i--) {
		const struct rp_nautilus_column_provider_column_desc_data_t *const p = &rp_nautilus_column_provider_column_desc_data[i];
		list = g_list_prepend(list, nautilus_column_new(
			p->name,	// name
			p->name,	// attribute
			p->label,	// label
			p->label));	// description (TODO?)
	}

	return list;
}

/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rom-properties-plugin.c: Nautilus Plugin Definition.                    *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rom-properties-provider.hpp"

static GType type_list[1];

#if defined(RP_UI_GTK3_GNOME)
// GNOME 3 desktop
#elif defined(RP_UI_GTK3_MATE)
// MATE desktop (v1.18.0+; GTK+ 3.x)
# define nautilus_module_initialize	caja_module_initialize
# define nautilus_module_shutdown	caja_module_shutdown
# define nautilus_module_list_types	caja_module_list_types
#elif defined(RP_UI_GTK3_CINNAMON)
// Cinnamon desktop
# define nautilus_module_initialize	nemo_module_initialize
# define nautilus_module_shutdown	nemo_module_shutdown
# define nautilus_module_list_types	nemo_module_list_types
#else
# error GTK3 desktop environment not set and/or supported.
#endif

G_MODULE_EXPORT void nautilus_module_initialize		(GTypeModule *module);
G_MODULE_EXPORT void nautilus_module_shutdown		(void);
G_MODULE_EXPORT void nautilus_module_list_types		(const GType	**types,
							 gint		 *n_types);
							 
G_MODULE_EXPORT void
nautilus_module_initialize(GTypeModule *module)
{
	if (getuid() == 0 || geteuid() == 0) {
		g_critical("*** " G_LOG_DOMAIN " does not support running as root.");
		return;
	}

#ifdef G_ENABLE_DEBUG
	g_message("Initializing " G_LOG_DOMAIN " extension");
#endif

	/* Register the types provided by this module */
	// NOTE: G_DEFINE_DYNAMIC_TYPE() marks the *_register_type()
	// functions as static, so we're using wrapper functions here.
	rom_properties_provider_register_type_ext(module);

	/* Setup the plugin provider type list */
	type_list[0] = TYPE_ROM_PROPERTIES_PROVIDER;
}

G_MODULE_EXPORT void
nautilus_module_shutdown(void)
{
#ifdef G_ENABLE_DEBUG
	g_message("Shutting down " G_LOG_DOMAIN " extension");
#endif
}

G_MODULE_EXPORT void
nautilus_module_list_types(const GType	**types,
			   gint		 *n_types)
{
	*types = type_list;
	*n_types = G_N_ELEMENTS(type_list);
}

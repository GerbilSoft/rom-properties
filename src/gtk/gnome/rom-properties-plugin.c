/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rom-properties-plugin.c: Nautilus Plugin Definition.                    *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "rom-properties-provider.hpp"

// GLib on non-Windows platforms defines G_MODULE_EXPORT to a no-op.
// This doesn't work when we use symbol visibility settings.
#if !defined(_WIN32) && (defined(__GNUC__) && __GNUC__ >= 4)
#ifdef G_MODULE_EXPORT
#undef G_MODULE_EXPORT
#endif
#define G_MODULE_EXPORT __attribute__ ((visibility ("default")))
#endif /* !_WIN32 && __GNUC__ >= 4 */

G_MODULE_EXPORT void nautilus_module_initialize		(GTypeModule *module);
G_MODULE_EXPORT void nautilus_module_shutdown		(void);
G_MODULE_EXPORT void nautilus_module_list_types		(const GType	**types,
							 gint		 *n_types);

static GType type_list[1];

G_MODULE_EXPORT void
nautilus_module_initialize(GTypeModule *module)
{
#ifdef G_ENABLE_DEBUG
	g_message("Initializing rom-properties-gnome extension");
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
	g_message("Shutting down rom-properties-gnome extension");
#endif
}

G_MODULE_EXPORT void
nautilus_module_list_types(const GType	**types,
			   gint		 *n_types)
{
	*types = type_list;
	*n_types = G_N_ELEMENTS(type_list);
}

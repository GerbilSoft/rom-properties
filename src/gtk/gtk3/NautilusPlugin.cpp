/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusPlugin.cpp: Nautilus (and forks) Plugin Definition              *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "NautilusPlugin.hpp"

#include "AchGDBus.hpp"
#include "NautilusMenuProvider.h"
#include "NautilusPropertyPageProvider.hpp"
#include "NautilusExtraInterfaces.h"
#include "plugin-helper.h"

static GType type_list[2];

// C includes (C++ namespace)
#include <cassert>

// Function pointers
static void *libextension_so;
PFN_NAUTILUS_FILE_INFO_GET_TYPE			pfn_nautilus_file_info_get_type;
PFN_NAUTILUS_FILE_INFO_GET_MIME_TYPE		pfn_nautilus_file_info_get_mime_type;
PFN_NAUTILUS_FILE_INFO_GET_URI			pfn_nautilus_file_info_get_uri;
PFN_NAUTILUS_FILE_INFO_GET_URI_SCHEME		pfn_nautilus_file_info_get_uri_scheme;
PFN_NAUTILUS_FILE_INFO_LIST_COPY		pfn_nautilus_file_info_list_copy;
PFN_NAUTILUS_FILE_INFO_LIST_FREE		pfn_nautilus_file_info_list_free;
PFN_NAUTILUS_MENU_ITEM_GET_TYPE			pfn_nautilus_menu_item_get_type;
PFN_NAUTILUS_MENU_ITEM_NEW			pfn_nautilus_menu_item_new;
PFN_NAUTILUS_MENU_PROVIDER_GET_TYPE		pfn_nautilus_menu_provider_get_type;
PFN_NAUTILUS_PROPERTY_PAGE_PROVIDER_GET_TYPE	pfn_nautilus_property_page_provider_get_type;
PFN_NAUTILUS_PROPERTY_PAGE_NEW			pfn_nautilus_property_page_new;

static void
rp_nautilus_register_types(GTypeModule *g_module)
{
	/* Register the types provided by this module */
	// NOTE: G_DEFINE_DYNAMIC_TYPE() marks the *_register_type()
	// functions as static, so we're using wrapper functions here.
	rp_nautilus_property_page_provider_register_type_ext(g_module);
	rp_nautilus_menu_provider_register_type_ext(g_module);

	/* Setup the plugin provider type list */
	type_list[0] = RP_TYPE_NAUTILUS_PROPERTY_PAGE_PROVIDER;
	type_list[1] = RP_TYPE_NAUTILUS_MENU_PROVIDER;

#ifdef ENABLE_ACHIEVEMENTS
	// Register AchGDBus.
	AchGDBus::instance();
#endif /* ENABLE_ACHIEVEMENTS */
}

/** Per-frontend initialization functions **/

#define NAUTILUS_MODULE_INITIALIZE_FUNC_INT(prefix) do { \
	CHECK_UID(); \
	SHOW_INIT_MESSAGE(); \
	VERIFY_GTK_VERSION(); \
\
	assert(libextension_so == nullptr); \
	if (libextension_so != nullptr) { \
		/* TODO: Reference count? */ \
		g_critical("*** " G_LOG_DOMAIN ": " #prefix "_module_initialize() called twice?"); \
		return; \
	} \
\
	/* dlopen() the extension library. */ \
	libextension_so = dlopen("lib" #prefix "-extension.so.1", RTLD_LAZY | RTLD_LOCAL); \
	if (!libextension_so) { \
		g_critical("*** " G_LOG_DOMAIN ": dlopen() failed: %s\n", dlerror()); \
		return; \
	} \
\
	/* Load symbols. */ \
	DLSYM(nautilus_file_info_get_type,		prefix##_file_info_get_type); \
	DLSYM(nautilus_file_info_get_mime_type,		prefix##_file_info_get_mime_type); \
	DLSYM(nautilus_file_info_get_uri,		prefix##_file_info_get_uri); \
	DLSYM(nautilus_file_info_get_uri_scheme,	prefix##_file_info_get_uri_scheme); \
	DLSYM(nautilus_file_info_list_copy,		prefix##_file_info_list_copy); \
	DLSYM(nautilus_file_info_list_free,		prefix##_file_info_list_free); \
	DLSYM(nautilus_menu_item_get_type,		prefix##_menu_item_get_type); \
	DLSYM(nautilus_menu_item_new,			prefix##_menu_item_new); \
	DLSYM(nautilus_menu_provider_get_type,		prefix##_menu_provider_get_type); \
	DLSYM(nautilus_property_page_provider_get_type,	prefix##_property_page_provider_get_type); \
	DLSYM(nautilus_property_page_new,		prefix##_property_page_new); \
} while (0)

extern "C" G_MODULE_EXPORT void
nautilus_module_initialize(GTypeModule *g_module)
{
	NAUTILUS_MODULE_INITIALIZE_FUNC_INT(nautilus);

	// Symbols loaded. Register our types.
	rp_nautilus_register_types(g_module);
}

extern "C" G_MODULE_EXPORT void
caja_module_initialize(GTypeModule *g_module)
{
	NAUTILUS_MODULE_INITIALIZE_FUNC_INT(caja);

	// Initialize Caja-specific function pointers.
	rp_caja_init(libextension_so);

	// Symbols loaded. Register our types.
	rp_nautilus_register_types(g_module);
}

extern "C" G_MODULE_EXPORT void
nemo_module_initialize(GTypeModule *g_module)
{
	NAUTILUS_MODULE_INITIALIZE_FUNC_INT(nemo);

	// Initialize Nemo-specific function pointers.
	rp_nemo_init(libextension_so);

	// Symbols loaded. Register our types.
	rp_nautilus_register_types(g_module);
}

/** Common shutdown and list_types functions. **/

extern "C" G_MODULE_EXPORT void
nautilus_module_shutdown(void)
{
#ifdef G_ENABLE_DEBUG
	g_message("Shutting down " G_LOG_DOMAIN " extension");
#endif

	if (libextension_so) {
		dlclose(libextension_so);
		libextension_so = nullptr;
	}
}

extern "C" G_MODULE_EXPORT void
nautilus_module_list_types(const GType	**types,
			   gint		 *n_types)
{
	*types = type_list;
	*n_types = G_N_ELEMENTS(type_list);
}

extern "C" {

/** Symbol aliases for MATE (Caja) **/

G_MODULE_EXPORT void caja_module_shutdown		(void)
	__attribute__((alias("nautilus_module_shutdown")));
G_MODULE_EXPORT void caja_module_list_types		(const GType	**types,
							 gint		 *n_types)
	__attribute__((alias("nautilus_module_list_types")));

/** Symbol aliases for Cinnamon (Nemo) **/

G_MODULE_EXPORT void nemo_module_shutdown		(void)
	__attribute__((alias("nautilus_module_shutdown")));
G_MODULE_EXPORT void nemo_module_list_types		(const GType	**types,
							 gint		 *n_types)
	__attribute__((alias("nautilus_module_list_types")));

}

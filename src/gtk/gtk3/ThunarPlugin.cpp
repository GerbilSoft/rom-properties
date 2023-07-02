/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * ThunarPlugin.cpp: ThunarX Plugin Definition                             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"
#include "ThunarPlugin.hpp"

#include "AchGDBus.hpp"
#include "plugin-helper.h"
#include "ThunarMenuProvider.hpp"
#include "ThunarPropertyPageProvider.hpp"

// Thunar version is based on GTK+ version.
#if GTK_CHECK_VERSION(3,0,0)
#  define LIBTHUNARX_SO_FILENAME "libthunarx-3.so"
#  define THUNARX_MAJOR_VERSION 1
#  define THUNARX_MINOR_VERSION 8
#  define THUNARX_MICRO_VERSION 0
#else /* !GTK_CHECK_VERSION(3,0,0) */
#  define LIBTHUNARX_SO_FILENAME "libthunarx-2.so"
#  define THUNARX_MAJOR_VERSION 1
#  define THUNARX_MINOR_VERSION 6
#  define THUNARX_MICRO_VERSION 0
#endif

static GType type_list[2];

// C includes.
#include <assert.h>

// Function pointers.
static void *libextension_so;

PFN_THUNARX_CHECK_VERSION pfn_thunarx_check_version;
PFN_THUNARX_FILE_INFO_GET_TYPE pfn_thunarx_file_info_get_type;
PFN_THUNARX_FILE_INFO_GET_MIME_TYPE pfn_thunarx_file_info_get_mime_type;
PFN_THUNARX_FILE_INFO_GET_URI pfn_thunarx_file_info_get_uri;
PFN_THUNARX_FILE_INFO_LIST_COPY pfn_thunarx_file_info_list_copy;
PFN_THUNARX_FILE_INFO_LIST_FREE pfn_thunarx_file_info_list_free;
#if GTK_CHECK_VERSION(3,0,0)
PFN_THUNARX_MENU_ITEM_GET_TYPE pfn_thunarx_menu_item_get_type;
PFN_THUNARX_MENU_ITEM_NEW pfn_thunarx_menu_item_new;
#endif /* GTK_CHECK_VERSION(3,0,0) */
PFN_THUNARX_MENU_PROVIDER_GET_TYPE pfn_thunarx_menu_provider_get_type;
PFN_THUNARX_PROPERTY_PAGE_PROVIDER_GET_TYPE pfn_thunarx_property_page_provider_get_type;
PFN_THUNARX_PROPERTY_PAGE_NEW pfn_thunarx_property_page_new;

static void
rp_thunar_register_types(ThunarxProviderPlugin *plugin)
{
	/* Register the types provided by this plugin */
	// NOTE: G_DEFINE_DYNAMIC_TYPE() marks the *_register_type()
	// functions as static, so we're using wrapper functions here.
	rp_thunar_property_page_provider_register_type_ext(plugin);
	rp_thunar_menu_provider_register_type_ext(plugin);

	/* Setup the plugin provider type list */
	type_list[0] = RP_TYPE_THUNAR_PROPERTY_PAGE_PROVIDER;
	type_list[1] = RP_TYPE_THUNAR_MENU_PROVIDER;

#ifdef ENABLE_ACHIEVEMENTS
	// Register AchGDBus.
	AchGDBus::instance();
#endif /* ENABLE_ACHIEVEMENTS */
}

/** Per-frontend initialization functions. **/

extern "C" G_MODULE_EXPORT void
thunar_extension_initialize(ThunarxProviderPlugin *plugin)
{
	CHECK_UID();
	SHOW_INIT_MESSAGE();
	VERIFY_GTK_VERSION();

	assert(libextension_so == nullptr);
	if (libextension_so != nullptr) {
		// TODO: Reference count?
		g_critical("*** " G_LOG_DOMAIN ": thunar_extension_initialize() called twice?");
		return;
	}

	// dlopen() libthunar-x?.so.
	libextension_so = dlopen(LIBTHUNARX_SO_FILENAME, RTLD_LAZY | RTLD_LOCAL);
	if (!libextension_so) {
		g_critical("*** " G_LOG_DOMAIN ": dlopen() failed: %s\n", dlerror());
		return;
	}

	// Load symbols.
	DLSYM(thunarx_check_version,			thunarx_check_version);
	DLSYM(thunarx_file_info_get_type,		thunarx_file_info_get_type);
	DLSYM(thunarx_file_info_get_mime_type,		thunarx_file_info_get_mime_type);
	DLSYM(thunarx_file_info_get_uri,		thunarx_file_info_get_uri);
	DLSYM(thunarx_file_info_list_copy,		thunarx_file_info_list_copy);
	DLSYM(thunarx_file_info_list_free,		thunarx_file_info_list_free);
#if GTK_CHECK_VERSION(3,0,0)
	DLSYM(thunarx_menu_item_get_type,		thunarx_menu_item_get_type);
	DLSYM(thunarx_menu_item_new,			thunarx_menu_item_new);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	DLSYM(thunarx_menu_provider_get_type,		thunarx_menu_provider_get_type);
	DLSYM(thunarx_property_page_provider_get_type,	thunarx_property_page_provider_get_type);
	DLSYM(thunarx_property_page_new,		thunarx_property_page_new);

	// Verify that the ThunarX versions are compatible.
	const gchar *mismatch = thunarx_check_version(
		THUNARX_MAJOR_VERSION, THUNARX_MINOR_VERSION, THUNARX_MICRO_VERSION);
	if (G_UNLIKELY(mismatch != nullptr)) {
		g_warning ("Version mismatch: %s", mismatch);
		dlclose(libextension_so);
		libextension_so = nullptr;
		return;
	}

	// Symbols loaded. Register our types.
	rp_thunar_register_types(plugin);
}

/** Common shutdown and list_types functions. **/

extern "C" G_MODULE_EXPORT void
thunar_extension_shutdown(void)
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
thunar_extension_list_types(const GType	**types,
			    gint	 *n_types)
{
	*types = type_list;
	*n_types = G_N_ELEMENTS(type_list);
}

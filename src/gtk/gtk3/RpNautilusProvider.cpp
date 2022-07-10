/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpNautilusProvider.cpp: Nautilus (and forks) Provider Definition.       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - audio-tags plugin for Xfce/Thunar
 * - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page-provider.html
 * - https://github.com/GNOME/nautilus/blob/bb433582165da10ab07337d707ea448703c3865f/src/nautilus-image-properties-page.c
 */

#include "stdafx.h"
#include "RpNautilusProvider.hpp"
#include "RpNautilusPlugin.hpp"
#include "is-supported.hpp"

#include "../RomDataView.hpp"

#include "librpbase/RomData.hpp"
using LibRpBase::RomData;

// NautilusPropertyPageProviderInterface definition.
extern "C"
struct _NautilusPropertyPageProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_pages) (NautilusPropertyPageProvider *provider,
	                     GList                        *files);
};

static void   rp_nautilus_provider_page_provider_init	(NautilusPropertyPageProviderInterface	*iface);
static GList *rp_nautilus_provider_get_pages		(NautilusPropertyPageProvider		*provider,
							 GList					*files);

struct _RpNautilusProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusProvider {
	GObject __parent__;
};

#if !GLIB_CHECK_VERSION(2,59,1)
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#  if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#   pragma GCC diagnostic push
#  endif
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusProvider, rp_nautilus_provider,
	G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE(NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
			rp_nautilus_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_nautilus_provider_register_type_ext(GTypeModule *module)
{
	rp_nautilus_provider_register_type(module);
}

static void
rp_nautilus_provider_class_init(RpNautilusProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_provider_class_finalize(RpNautilusProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_provider_init(RpNautilusProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_provider_page_provider_init(NautilusPropertyPageProviderInterface *iface)
{
	iface->get_pages = rp_nautilus_provider_get_pages;
}

static GList*
rp_nautilus_provider_get_pages(NautilusPropertyPageProvider *provider, GList *files)
{
	RP_UNUSED(provider);
	GList *pages = nullptr;
	GList *file;
	NautilusFileInfo *info;

	if (g_list_length(files) != 1) 
		return nullptr;

	file = g_list_first(files);
	if (G_UNLIKELY(file == nullptr))
		return nullptr;

	info = NAUTILUS_FILE_INFO(file->data);
	gchar *const uri = nautilus_file_info_get_uri(info);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		return nullptr;
	}

	// Attempt to open the URI.
	RomData *const romData = rp_gtk_open_uri(uri);
	if (G_LIKELY(romData != nullptr)) {
		// Create the RomDataView.
		// TODO: Add some extra padding to the top...
		GtkWidget *const romDataView = rom_data_view_new_with_romData(uri, romData, RP_DFT_GNOME);
		gtk_widget_set_name(romDataView, "romDataView");
		gtk_widget_show(romDataView);
		romData->unref();

		// tr: Tab title
		const char8_t *const tabTitle = C_("RomDataView", "ROM Properties");

		// Create the NautilusPropertyPage.
		NautilusPropertyPage *const page = nautilus_property_page_new(
			"RomPropertiesPage::property_page",
			gtk_label_new(reinterpret_cast<const char*>(tabTitle)), romDataView);

		// Add the page to the pages provided by this plugin.
		pages = g_list_prepend(pages, page);
	}

	g_free(uri);
	return pages;
}

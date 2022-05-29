/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * RpThunarProvider.cpp: ThunarX Provider Definition.                      *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpThunarProvider.hpp"
#include "is-supported.hpp"

#include "../RomDataView.hpp"

#include "librpbase/RomData.hpp"
using LibRpBase::RomData;

// thunarx.h mini replacement
#include "thunarx-mini.h"

static void   rp_thunar_provider_page_provider_init	(ThunarxPropertyPageProviderIface *iface);
static GList *rp_thunar_provider_get_pages		(ThunarxPropertyPageProvider      *page_provider,
							 GList                            *files);

struct _RpThunarProviderClass {
	GObjectClass __parent__;
};

struct _RpThunarProvider {
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
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpThunarProvider, rp_thunar_provider,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0),
	G_IMPLEMENT_INTERFACE_DYNAMIC(THUNARX_TYPE_PROPERTY_PAGE_PROVIDER,
		rp_thunar_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_thunar_provider_register_type_ext(ThunarxProviderPlugin *plugin)
{
	rp_thunar_provider_register_type(G_TYPE_MODULE(plugin));
}

static void
rp_thunar_provider_class_init(RpThunarProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_thunar_provider_class_finalize(RpThunarProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_thunar_provider_init(RpThunarProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_thunar_provider_page_provider_init(ThunarxPropertyPageProviderIface *iface)
{
	iface->get_pages = rp_thunar_provider_get_pages;
}

static GList*
rp_thunar_provider_get_pages(ThunarxPropertyPageProvider *page_provider, GList *files)
{
	RP_UNUSED(page_provider);
	GList *pages = nullptr;
	GList *file;
	ThunarxFileInfo *info;

	if (g_list_length(files) != 1) 
		return nullptr;

	file = g_list_first(files);
	if (G_UNLIKELY(file == nullptr))
		return nullptr;

	info = THUNARX_FILE_INFO(file->data);
	gchar *const uri = thunarx_file_info_get_uri(info);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		return nullptr;
	}

	// Attempt to open the URI.
	RomData *const romData = rp_gtk_open_uri(uri);
	if (G_LIKELY(romData != nullptr)) {
		// Create the RomDataView.
		GtkWidget *const romDataView = rom_data_view_new_with_romData(uri, romData, RP_DFT_XFCE);
		romData->unref();
		gtk_widget_show(romDataView);

		// tr: Tab title.
		const char *const tabTitle = C_("RomDataView", "ROM Properties");

		// Create the ThunarxPropertyPage.
		GtkWidget *const page = thunarx_property_page_new(tabTitle);
		gtk_container_add(GTK_CONTAINER(page), romDataView);

		// Add the page to the pages provided by this plugin.
		pages = g_list_prepend(pages, page);
	}

	g_free(uri);
	return pages;
}

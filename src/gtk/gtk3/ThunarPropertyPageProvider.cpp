/*****************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                           *
 * ThunarPropertyPageProvider.cpp: ThunarX Property Page Provider Definition *
 *                                                                           *
 * Copyright (c) 2017-2023 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "ThunarPropertyPageProvider.hpp"

#include "../RomDataView.hpp"
#include "../xattr/XAttrView.hpp"
#include "is-supported.hpp"

#include "librpbase/RomData.hpp"
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;
using LibRpBase::RomData;

// thunarx.h mini replacement
#include "thunarx-mini.h"

static void   rp_thunar_property_page_provider_page_provider_init	(ThunarxPropertyPageProviderIface *iface);
static GList *rp_thunar_property_page_provider_get_pages		(ThunarxPropertyPageProvider      *page_provider,
									 GList                            *files)
									G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

struct _RpThunarPropertyPageProviderClass {
	GObjectClass __parent__;
};

struct _RpThunarPropertyPageProvider {
	GObject __parent__;
};

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && __GNUC__ >= 8
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpThunarPropertyPageProvider, rp_thunar_property_page_provider,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0),
	G_IMPLEMENT_INTERFACE_DYNAMIC(THUNARX_TYPE_PROPERTY_PAGE_PROVIDER,
		rp_thunar_property_page_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_thunar_property_page_provider_register_type_ext(ThunarxProviderPlugin *plugin)
{
	rp_thunar_property_page_provider_register_type(G_TYPE_MODULE(plugin));
}

static void
rp_thunar_property_page_provider_class_init(RpThunarPropertyPageProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_thunar_property_page_provider_class_finalize(RpThunarPropertyPageProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_thunar_property_page_provider_init(RpThunarPropertyPageProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_thunar_property_page_provider_page_provider_init(ThunarxPropertyPageProviderIface *iface)
{
	iface->get_pages = rp_thunar_property_page_provider_get_pages;
}

/**
 * Instantiate a Thunarx Property Page with a RomDataView for this URI.
 * @param uri URI
 * @return ThunarxPropertyPage with RomDataView, or nullptr on error.
 */
static GtkWidget*
rp_thunar_property_page_provider_get_RomDataView(const gchar *uri)
{
	// Attempt to open the URI.
	RomData *const romData = rp_gtk_open_uri(uri);
	if (G_UNLIKELY(!romData)) {
		// Unable to open the URI as a RomData object.
		return nullptr;
	}

	// Create the RomDataView.
	GtkWidget *const romDataView = rp_rom_data_view_new_with_romData(uri, romData, RP_DFT_XFCE);
	gtk_widget_set_name(romDataView, "romDataView");
	gtk_widget_show(romDataView);
	romData->unref();

	// tr: Tab title.
	const char *const tabTitle = C_("RomDataView", "ROM Properties");

	// Create the ThunarxPropertyPage
	GtkWidget *const page = thunarx_property_page_new(tabTitle);
	gtk_container_add(GTK_CONTAINER(page), romDataView);
	return page;
}

/**
 * Instantiate a Thunarx Property Page with an XAttrView for this URI.
 * @param uri URI
 * @return ThunarxPropertyPage with XAttrView, or nullptr on error.
 */
static GtkWidget*
rp_thunar_property_page_provider_get_XAttrView(const gchar *uri)
{
	// TODO: Actually open the file.
	// For now, add a test widget.

	GtkWidget *const xattrView = rp_xattr_view_new(uri);
	if (!rp_xattr_view_has_attributes(RP_XATTR_VIEW(xattrView))) {
		// No attributes available.
		g_object_ref_sink(xattrView);
		g_object_unref(xattrView);
		return nullptr;
	}
	gtk_widget_set_name(xattrView, "xattrView");
	gtk_widget_show(xattrView);

	// tr: Tab title.
	const char *const tabTitle = "xattrs";

	// Create the ThunarxPropertyPage
	GtkWidget *const page = thunarx_property_page_new(tabTitle);
	gtk_container_add(GTK_CONTAINER(page), xattrView);
	return page;
}

static GList*
rp_thunar_property_page_provider_get_pages(ThunarxPropertyPageProvider *page_provider, GList *files)
{
	RP_UNUSED(page_provider);

	assert(files->prev == nullptr);	// `files` should be the list head
	GList *const file = g_list_first(files);
	if (G_UNLIKELY(file == nullptr)) {
		// No files...
		return nullptr;
	}

	// TODO: Handle multiple files?
	if (file->next != nullptr) {
		// Only handles single files.
		return nullptr;
	}

	ThunarxFileInfo *const info = THUNARX_FILE_INFO(file->data);
	gchar *const uri = thunarx_file_info_get_uri(info);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		return nullptr;
	}

	GList *list = nullptr;
	GtkWidget *page;

	// Check if XAttrView is enabled.
	const Config *const config = Config::instance();
	if (config->showXAttrView()) {
		// XAttrView is enabled.
		page = rp_thunar_property_page_provider_get_XAttrView(uri);
		if (page) {
			list = g_list_prepend(list, page);
		}
	}

	// RomDataView
	page = rp_thunar_property_page_provider_get_RomDataView(uri);
	if (page) {
		list = g_list_prepend(list, page);
	}

	g_free(uri);
	return list;
}

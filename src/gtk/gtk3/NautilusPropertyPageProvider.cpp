/********************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                              *
 * NautilusPropertyPageProvider.cpp: Nautilus Property Page Provider Definition *
 *                                                                              *
 * Copyright (c) 2017-2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                                    *
 ********************************************************************************/

/**
 * References:
 * - audio-tags plugin for Xfce/Thunar
 * - http://api.xfce.m8t.in/xfce-4.10/nautilusx-1.4.0/ThunarxPropertyPage.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page-provider.html
 * - https://github.com/GNOME/nautilus/blob/bb433582165da10ab07337d707ea448703c3865f/src/nautilus-image-properties-page.c
 */

#include "stdafx.h"
#include "NautilusPropertyPageProvider.hpp"

#include "is-supported.hpp"
#include "NautilusPlugin.hpp"

#include "../RomDataView.hpp"
#include "../xattr/XAttrView.hpp"

#include "librpbase/RomData.hpp"
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;

// nautilus-extension.h mini replacement
#include "nautilus-extension-mini.h"

static void   rp_nautilus_property_page_provider_page_provider_init	(NautilusPropertyPageProviderInterface	*iface);
static GList *rp_nautilus_property_page_provider_get_pages		(NautilusPropertyPageProvider		*provider,
									 GList					*files)
									G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

static void	rp_nemo_name_and_desc_provider_init			(NemoNameAndDescProviderInterface *iface);
static GList*	rp_nemo_name_and_desc_provider_get_name_and_desc	(NemoNameAndDescProvider *provider);

struct _RpNautilusPropertyPageProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusPropertyPageProvider {
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
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusPropertyPageProvider, rp_nautilus_property_page_provider,
	G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER, rp_nautilus_property_page_provider_page_provider_init)
	G_IMPLEMENT_INTERFACE_DYNAMIC(NEMO_TYPE_NAME_AND_DESC_PROVIDER, rp_nemo_name_and_desc_provider_init)
);

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_nautilus_property_page_provider_register_type_ext(GTypeModule *module)
{
	rp_nautilus_property_page_provider_register_type(module);
}

static void
rp_nautilus_property_page_provider_class_init(RpNautilusPropertyPageProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_property_page_provider_class_finalize(RpNautilusPropertyPageProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_property_page_provider_init(RpNautilusPropertyPageProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_property_page_provider_page_provider_init(NautilusPropertyPageProviderInterface *iface)
{
	iface->get_pages = rp_nautilus_property_page_provider_get_pages;
}

static void
rp_nemo_name_and_desc_provider_init(NemoNameAndDescProviderInterface *iface)
{
	iface->get_name_and_desc = rp_nemo_name_and_desc_provider_get_name_and_desc;
}

/**
 * Instantiate a Nautilus Property Page with a RomDataView for this URI.
 * @param uri URI
 * @return NautilusPropertyPage with RomDataView, or nullptr on error.
 */
static NautilusPropertyPage*
rp_nautilus_property_page_provider_get_RomDataView(const gchar *uri)
{
	// Attempt to open the URI.
	const RomDataPtr romData = rp_gtk_open_uri(uri);
	if (G_UNLIKELY(!romData)) {
		// Unable to open the URI as a RomData object.
		return nullptr;
	}

	// Create the RomDataView.
	GtkWidget *const romDataView = rp_rom_data_view_new_with_romData(uri, romData, RP_DFT_GNOME);
	gtk_widget_set_name(romDataView, "romDataView");
	gtk_widget_show(romDataView);

	// tr: Tab title.
	const char *const tabTitle = C_("RomDataView", "ROM Properties");

	// Create the NautilusPropertyPage
	return nautilus_property_page_new(
		"RomProperties::RomDataView",
		gtk_label_new(tabTitle), romDataView);
}

/**
 * Instantiate a Nautilus Property Page with an XAttrView for this URI.
 * @param uri URI
 * @return NautilusPropertyPage with XAttrView, or nullptr on error.
 */
static NautilusPropertyPage*
rp_nautilus_property_page_provider_get_XAttrView(const gchar *uri)
{
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

	// Create the NautilusPropertyPage
	return nautilus_property_page_new(
		"RomProperties::XAttrView",
		gtk_label_new(tabTitle), xattrView);
}

static GList*
rp_nautilus_property_page_provider_get_pages(NautilusPropertyPageProvider *provider, GList *files)
{
	RP_UNUSED(provider);

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

	NautilusFileInfo *const info = NAUTILUS_FILE_INFO(file->data);
	gchar *const uri = nautilus_file_info_get_uri(info);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		return nullptr;
	}

	GList *list = nullptr;
	NautilusPropertyPage *page;

	// Check if XAttrView is enabled.
	const Config *const config = Config::instance();
	if (config->getBoolConfigOption_default(Config::BoolConfig::Options_ShowXAttrView)) {
		// XAttrView is enabled.
		page = rp_nautilus_property_page_provider_get_XAttrView(uri);
		if (page) {
			list = g_list_prepend(list, page);
		}
	}

	// RomDataView
	page = rp_nautilus_property_page_provider_get_RomDataView(uri);
	if (page) {
		list = g_list_prepend(list, page);
	}

	g_free(uri);
	return list;
}

static GList*
rp_nemo_name_and_desc_provider_get_name_and_desc(NemoNameAndDescProvider *provider)
{
	RP_UNUSED(provider);
	g_return_val_if_fail(RP_IS_NAUTILUS_PROPERTY_PAGE_PROVIDER(provider), nullptr);
	return g_list_append(nullptr, g_strdup("RpNautilusPropertyPageProvider:::ROM Properties Page - property page extension"));
}

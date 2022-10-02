/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * RpNautilusPropertiesModelProvider.cpp: Nautilus properties model provider *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2022 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 ***************************************************************************/

#include "stdafx.h"
#include "RpNautilusPropertiesModelProvider.hpp"
#include "RpNautilusPlugin.hpp"
#include "is-supported.hpp"

#include "../RomDataView.hpp"

#include "librpbase/RomData.hpp"
using LibRpBase::RomData;

// NautilusPropertiesModelProviderInterface definition.
extern "C"
struct _NautilusPropertiesModelProviderInterface {
	GTypeInterface g_iface;

	GList *(*get_models) (NautilusPropertiesModelProvider *provider,
	                      GList                           *files);
};

static void   rp_nautilus_properties_model_provider_page_provider_init	(NautilusPropertiesModelProviderInterface	*iface);
static GList *rp_nautilus_properties_model_provider_get_models		(NautilusPropertiesModelProvider		*provider,
									 GList						*files);

struct _RpNautilusPropertiesModelProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusPropertiesModelProvider {
	GObject __parent__;
};

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#    if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#      pragma GCC diagnostic push
#    endif
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusPropertiesModelProvider, rp_nautilus_properties_model_provider,
	G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE(NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER,
			rp_nautilus_properties_model_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_nautilus_properties_model_provider_register_type_ext(GTypeModule *module)
{
	rp_nautilus_properties_model_provider_register_type(module);
}

static void
rp_nautilus_properties_model_provider_class_init(RpNautilusPropertiesModelProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_properties_model_provider_class_finalize(RpNautilusPropertiesModelProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_properties_model_provider_init(RpNautilusPropertiesModelProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_properties_model_provider_page_provider_init(NautilusPropertiesModelProviderInterface *iface)
{
	iface->get_models = rp_nautilus_properties_model_provider_get_models;
}

static GList*
rp_nautilus_properties_model_provider_get_models(NautilusPropertiesModelProvider *provider, GList *files)
{
	RP_UNUSED(provider);
	GList *models = nullptr;
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
		// FIXME
		// TODO: Populate the models asynchronously?

		// Create a sample model.
		// TODO: Make sure it gets freed properly.
		GListStore *listStore = g_list_store_new(NAUTILUS_TYPE_PROPERTIES_ITEM);
		g_list_store_append(listStore, nautilus_properties_item_new("RP Name", "RP Value"));

		NautilusPropertiesModel *model = nautilus_properties_model_new("ROM Properties", G_LIST_MODEL(listStore));
		models = g_list_prepend(models, model);
#if 0
		// Create the RomDataView.
		// TODO: Add some extra padding to the top...
		GtkWidget *const romDataView = rom_data_view_new_with_romData(uri, romData, RP_DFT_GNOME);
		gtk_widget_set_name(romDataView, "romDataView");
		gtk_widget_show(romDataView);
		romData->unref();

		// tr: Tab title.
		const char *const tabTitle = C_("RomDataView", "ROM Properties");

		// Create the NautilusPropertyPage.
		NautilusPropertyPage *const page = nautilus_property_page_new(
			"RomPropertiesPage::property_page",
			gtk_label_new(tabTitle), romDataView);

		// Add the page to the pages provided by this plugin.
		pages = g_list_prepend(pages, page);
#endif
	}

	g_free(uri);
	return models;
}

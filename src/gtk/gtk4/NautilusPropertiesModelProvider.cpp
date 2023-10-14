/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * NautilusPropertiesModelProvider.cpp: Nautilus properties model provider   *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2022 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "NautilusPropertiesModelProvider.hpp"
#include "NautilusPropertiesModel.hpp"

#include "NautilusPlugin.hpp"
#include "is-supported.hpp"

#include "../RomDataView.hpp"

#include "librpbase/RomData.hpp"
using namespace LibRpBase;

// nautilus-extension.h mini replacement
#include "nautilus-extension-mini.h"

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
#  if defined(__GNUC__) && __GNUC__ >= 8
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusPropertiesModelProvider, rp_nautilus_properties_model_provider,
	G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_PROPERTIES_MODEL_PROVIDER,
		rp_nautilus_properties_model_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && __GNUC__ > 8
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

	// Attempt to open the URI.
	const RomDataPtr romData = rp_gtk_open_uri(uri);
	if (G_UNLIKELY(!romData)) {
		// Unable to open the URI as a RomData object.
		g_free(uri);
		return nullptr;
	}

	// Create the RpNautilusPropertiesModel and return it in a GList.
	NautilusPropertiesModel *const model = rp_nautilus_properties_model_new(romData);
	assert(model != nullptr);
	g_free(uri);
	if (model) {
		// Check for achievements here.
		// NOTE: We can't determine when the NautilusPropertiesModel is actually
		// displayed, since it's an abstract model and not a GtkWidget.
		romData->checkViewedAchievements();
		return g_list_prepend(nullptr, model);
	}

	// No model...
	return nullptr;
}

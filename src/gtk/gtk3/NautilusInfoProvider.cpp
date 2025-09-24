/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusInfoProvider.cpp: Nautilus (and forks) Info Provider Definition *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://github.com/xfce-mirror/thunar-archive-plugin/blob/master/thunar-archive-plugin/tap-provider.c

#include "stdafx.h"
#include "NautilusInfoProvider.hpp"
#include "NautilusExtraInterfaces.h"

// rp_gtk_open_uri()
#include "is-supported.hpp"

// librpbase
#include "librpbase/RomMetaData.hpp"
using namespace LibRpBase;

// C++ STL classes
using std::array;

// nautilus-extension.h mini replacement
#if GTK_CHECK_VERSION(4, 0, 0)
#  include "../gtk4/NautilusPlugin.hpp"
#  include "../gtk4/nautilus-extension-mini.h"
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
#  include "NautilusPlugin.hpp"
#  include "nautilus-extension-mini.h"
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

static void rp_nautilus_info_provider_interface_init(NautilusInfoProviderInterface *iface);
static void rp_nautilus_info_provider_dispose(GObject *object);
static void rp_nautilus_info_provider_finalize(GObject *object);

static NautilusOperationResult
rp_nautilus_info_provider_update_file_info(
	NautilusInfoProvider     *provider,
	NautilusFileInfo         *file_info,
	GClosure                 *update_complete,
	NautilusOperationHandle **handle);

static gboolean
rp_nautilus_info_provider_process(RpNautilusInfoProvider *provider);

static void
rp_nautilus_info_provider_cancel_update(
	NautilusInfoProvider     *provider,
	NautilusOperationHandle  *handle);

struct _RpNautilusInfoProviderClass {
	GObjectClass __parent__;
};

// Info request
// Also used as the NautilusOperationHandle.
// NOTE: request_info does *not* own these objects.
struct request_info {
	NautilusFileInfo *file_info;
	GClosure *update_complete;
};

struct _RpNautilusInfoProvider {
	GObject __parent__;

	GQueue request_queue;	// Request queue (element is struct request_info*)
	guint idle_process;	// Idle function for processing
};

#if !GLIB_CHECK_VERSION(2, 59, 1)
#  if defined(__GNUC__) && __GNUC__ >= 8
/* Disable GCC 8 -Wcast-function-type warnings. (Fixed in glib-2.59.1 upstream.) */
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif /* !GLIB_CHECK_VERSION(2, 59, 1) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusInfoProvider, rp_nautilus_info_provider,
	G_TYPE_OBJECT, (GTypeFlags)0,
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_INFO_PROVIDER, rp_nautilus_info_provider_interface_init)
);

#if !GLIB_CHECK_VERSION(2, 59, 1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2, 59, 1) */

void
rp_nautilus_info_provider_register_type_ext(GTypeModule *g_module)
{
	rp_nautilus_info_provider_register_type(G_TYPE_MODULE(g_module));

#ifdef HAVE_EXTRA_INTERFACES
	// Add extra fork-specific interfaces.
	rp_nautilus_extra_interfaces_add(g_module, RP_TYPE_NAUTILUS_INFO_PROVIDER);
#endif /* HAVE_EXTRA_INTERFACES */
}

static void
rp_nautilus_info_provider_class_init(RpNautilusInfoProviderClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_nautilus_info_provider_dispose;
	gobject_class->finalize = rp_nautilus_info_provider_finalize;
}

static void
rp_nautilus_info_provider_class_finalize(RpNautilusInfoProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_info_provider_init(RpNautilusInfoProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_info_provider_interface_init(NautilusInfoProviderInterface *iface)
{
	iface->update_file_info = rp_nautilus_info_provider_update_file_info;
	iface->cancel_update = rp_nautilus_info_provider_cancel_update;
}

static void
rp_nautilus_info_provider_dispose(GObject *object)
{
	RpNautilusInfoProvider *const provider = RP_NAUTILUS_INFO_PROVIDER(object);

	// Unregister timer sources.
	g_clear_handle_id(&provider->idle_process, g_source_remove);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_nautilus_info_provider_parent_class)->dispose(object);
}

static void
rp_nautilus_info_provider_finalize(GObject *object)
{
	RpNautilusInfoProvider *const provider = RP_NAUTILUS_INFO_PROVIDER(object);

	// Delete any remaining requests and free the queue.
	for (GList *p = provider->request_queue.head; p != NULL; p = p->next) {
		if (p->data) {
			struct request_info *const req = static_cast<struct request_info*>(p->data);
			g_object_unref(req->file_info);
			g_closure_unref(req->update_complete);
			g_free(req);
		}
	}
	g_queue_clear(&provider->request_queue);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_nautilus_info_provider_parent_class)->finalize(object);
}

/**
 * Update file information.
 * @param provider		[in] RpNautilusInfoProvider
 * @param file_info		[in] NautilusFileInfo
 * @param update_complete	[in]
 * @param handle		[out] Request handle (contains struct request_info*)
 * @return NautilusOperationResult
 */
static NautilusOperationResult
rp_nautilus_info_provider_update_file_info(
	NautilusInfoProvider     *provider,
	NautilusFileInfo         *file_info,
	GClosure                 *update_complete,
	NautilusOperationHandle **handle)
{
	g_return_val_if_fail(RP_IS_NAUTILUS_INFO_PROVIDER(provider), NAUTILUS_OPERATION_FAILED);
	g_return_val_if_fail(NAUTILUS_IS_FILE_INFO(file_info), NAUTILUS_OPERATION_FAILED);
	g_return_val_if_fail(handle != nullptr, NAUTILUS_OPERATION_FAILED);

	RpNautilusInfoProvider *const rpp = reinterpret_cast<RpNautilusInfoProvider*>(provider);

	// Put the file in the queue.
	struct request_info *const req = static_cast<struct request_info*>(g_malloc(sizeof(struct request_info)));
	req->file_info = static_cast<NautilusFileInfo*>(g_object_ref(file_info));
	req->update_complete = g_closure_ref(update_complete);
	g_queue_push_tail(&rpp->request_queue, reinterpret_cast<gpointer>(req));

	// The request is the handle.
	*handle = reinterpret_cast<NautilusOperationHandle*>(req);

	// Make sure the idle process is started.
	// TODO: Make it multi-threaded? (needs atomic compare and/or mutex...)
	if (rpp->idle_process == 0) {
		rpp->idle_process = g_idle_add(G_SOURCE_FUNC(rp_nautilus_info_provider_process), provider);
	}

	return NAUTILUS_OPERATION_IN_PROGRESS;
}

/**
 * Process a request.
 * @param provider RpNautilusInfoProvider
 * @return True to continue processing more requests; false if we're done.
 */
static gboolean
rp_nautilus_info_provider_process(RpNautilusInfoProvider *provider)
{
	// Get the next entry from the queue.
	struct request_info *const req = static_cast<struct request_info*>(g_queue_pop_head(&provider->request_queue));
	if (!req) {
		// Nothing left in the queue.
		provider->idle_process = 0;
		return false;
	}

	// Get the URI.
	gchar *const uri = nautilus_file_info_get_uri(req->file_info);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		nautilus_info_provider_update_complete_invoke(
			req->update_complete,
			reinterpret_cast<NautilusInfoProvider*>(provider),
			reinterpret_cast<NautilusOperationHandle*>(req),
			NAUTILUS_OPERATION_FAILED);
		g_object_unref(req->file_info);
		g_closure_unref(req->update_complete);
		g_free(req);
		// Continue processing the queue.
		return true;
	}

	const RomDataPtr romData = rp_gtk_open_uri(uri);
	g_free(uri);
	if (G_UNLIKELY(!romData)) {
		// Unable to open the URI as a RomData object.
		nautilus_info_provider_update_complete_invoke(
			req->update_complete,
			reinterpret_cast<NautilusInfoProvider*>(provider),
			reinterpret_cast<NautilusOperationHandle*>(req),
			NAUTILUS_OPERATION_FAILED);
		g_object_unref(req->file_info);
		g_closure_unref(req->update_complete);
		g_free(req);
		// Continue processing the queue.
		return true;
	}

	// Check for custom metadata propreties.
	// NOTE: Only strings are supported.
	static constexpr size_t custom_property_count = static_cast<size_t>(Property::PropertyCount) - static_cast<size_t>(Property::GameID);
	static const array<const char*, custom_property_count> nautilus_prop_names = {{
		"rp-game-id",
		"rp-title-id",
		"rp-media-id",
		"rp-os-version",
		"rp-encryption-key",
		"rp-pixel-format",
	}};

	// Custom metadata property names start at Prpoerty::GameID.
	const RomMetaData *const metaData = romData->metaData();
	if (metaData && !metaData->empty()) {
		for (const RomMetaData::MetaData &prop : *metaData) {
			if (prop.name < Property::GameID) {
				continue;
			}

			// Only strings are accepted for now.
			assert(prop.type == PropertyType::String);
			if (prop.type != PropertyType::String) {
				continue;
			}

			// Add the property.
			const size_t index = static_cast<size_t>(prop.name) - static_cast<size_t>(Property::GameID);
			nautilus_file_info_add_string_attribute(req->file_info, nautilus_prop_names[index], prop.data.str);
		}
	}

	const Config *const config = Config::instance();
	if (config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon)) {
		// Overlay icon is enabled.
		// Does this RomData object have "dangerous" permissions?
		if (romData->hasDangerousPermissions()) {
			// Add the "security-medium" emblem.
			nautilus_file_info_add_emblem(req->file_info, "security-medium");
		}
	}

	// Finished processing this file.
	nautilus_info_provider_update_complete_invoke(
		req->update_complete,
		reinterpret_cast<NautilusInfoProvider*>(provider),
		reinterpret_cast<NautilusOperationHandle*>(req),
		NAUTILUS_OPERATION_COMPLETE);
	g_object_unref(req->file_info);
	g_closure_unref(req->update_complete);
	g_free(req);
	// Continue processing the queue.
	return true;
}

static void
rp_nautilus_info_provider_cancel_update(
	NautilusInfoProvider     *provider,
	NautilusOperationHandle  *handle)
{
	// Not doing anything here...
	RP_UNUSED(provider);
	RP_UNUSED(handle);
}

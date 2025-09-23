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
using namespace LibRpBase;

// nautilus-extension.h mini replacement
#if GTK_CHECK_VERSION(4, 0, 0)
#  include "../gtk4/NautilusPlugin.hpp"
#  include "../gtk4/nautilus-extension-mini.h"
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
#  include "NautilusPlugin.hpp"
#  include "nautilus-extension-mini.h"
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

static void rp_nautilus_info_provider_interface_init(NautilusInfoProviderInterface *iface);

static NautilusOperationResult
rp_nautilus_info_provider_update_file_info(
	NautilusInfoProvider     *provider,
	NautilusFileInfo         *file,
	GClosure                 *update_complete,
	NautilusOperationHandle **handle);

static void
rp_nautilus_info_provider_cancel_update(
	NautilusInfoProvider     *provider,
	NautilusOperationHandle  *handle);

struct _RpNautilusInfoProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusInfoProvider {
	GObject __parent__;
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
	RP_UNUSED(klass);
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

static NautilusOperationResult
rp_nautilus_info_provider_update_file_info(
	NautilusInfoProvider     *provider,
	NautilusFileInfo         *file,
	GClosure                 *update_complete,
	NautilusOperationHandle **handle)
{
	g_return_val_if_fail(RP_IS_NAUTILUS_INFO_PROVIDER(provider), NAUTILUS_OPERATION_FAILED);
	g_return_val_if_fail(NAUTILUS_IS_FILE_INFO(file), NAUTILUS_OPERATION_FAILED);
	g_return_val_if_fail(handle != nullptr, NAUTILUS_OPERATION_FAILED);

	// Attempt to open the URI.
	gchar *const uri = nautilus_file_info_get_uri(file);
	if (G_UNLIKELY(uri == nullptr)) {
		// No URI...
		return NAUTILUS_OPERATION_FAILED;
	}
	const RomDataPtr romData = rp_gtk_open_uri(uri);
	g_free(uri);
	if (G_UNLIKELY(!romData)) {
		// Unable to open the URI as a RomData object.
		return NAUTILUS_OPERATION_FAILED;
	}

	// TODO: Custom metadata properties.

	const Config *const config = Config::instance();
	if (config->getBoolConfigOption(Config::BoolConfig::Options_ShowDangerousPermissionsOverlayIcon)) {
		// Overlay icon is enabled.
		// Does this RomData object have "dangerous" permissions?
		if (romData->hasDangerousPermissions()) {
			// Add the "security-medium" emblem.
			nautilus_file_info_add_emblem(file, "security-medium");
		}
	}

	// NOTE: The closure is only used if we're checking asynchronously.
	// If we decide to cehck asynchronously, *handle needs to be set to
	// a value that represents the file, and this handle is used as
	// part of nautilus_info_provider_update_complete_invoke().
	// It can also be used with cancel_update().
	RP_UNUSED(update_complete);
	/*if (update_complete) {
		nautilus_info_provider_update_complete_invoke(update_complete, provider, *handle, NAUTILUS_OPERATION_COMPLETE);
	}*/
	return NAUTILUS_OPERATION_COMPLETE;
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

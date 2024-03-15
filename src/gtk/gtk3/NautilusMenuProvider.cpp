/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * NautilusMenuProvider.cpp: Nautilus (and forks) Menu Provider Definition *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://github.com/xfce-mirror/thunar-archive-plugin/blob/master/thunar-archive-plugin/tap-provider.c

#include "stdafx.h"
#include "NautilusMenuProvider.hpp"
#include "MenuProviderCommon.h"

#include "../RomDataView.hpp"

// nautilus-extension.h mini replacement
#if GTK_CHECK_VERSION(4,0,0)
#  include "../gtk4/NautilusPlugin.hpp"
#  include "../gtk4/nautilus-extension-mini.h"
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  include "NautilusPlugin.hpp"
#  include "nautilus-extension-mini.h"
#endif /* GTK_CHECK_VERSION(4,0,0) */

// Supported MIME types
// TODO: Consolidate with the KF5 service menu?
#include "mime-types.convert-to-png.h"

static GQuark rp_item_convert_to_png_quark;

static void   rp_nautilus_menu_provider_page_provider_init	(NautilusMenuProviderInterface *iface);

static GList *rp_nautilus_menu_provider_get_file_items(
	NautilusMenuProvider *provider,
#if !GTK_CHECK_VERSION(4,0,0)
	GtkWidget *window,
#endif /* !GTK_CHECK_VERSION(4,0,0) */
	GList *files) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

struct _RpNautilusMenuProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusMenuProvider {
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
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusMenuProvider, rp_nautilus_menu_provider,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0),
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_MENU_PROVIDER,
		rp_nautilus_menu_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

void
rp_nautilus_menu_provider_register_type_ext(GTypeModule *plugin)
{
	rp_nautilus_menu_provider_register_type(G_TYPE_MODULE(plugin));
}

static void
rp_nautilus_menu_provider_class_init(RpNautilusMenuProviderClass *klass)
{
	RP_UNUSED(klass);

	// Get quarks for the various GLib strings.
	rp_item_convert_to_png_quark = g_quark_from_string("rp-item-convert-to-png");
}

static void
rp_nautilus_menu_provider_class_finalize(RpNautilusMenuProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_nautilus_menu_provider_init(RpNautilusMenuProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_nautilus_menu_provider_page_provider_init(NautilusMenuProviderInterface *iface)
{
	iface->get_file_items = rp_nautilus_menu_provider_get_file_items;
}

static gpointer
rp_item_convert_to_png_ThreadFunc(GList *files)
{
	for (GList *file = files; file != nullptr; file = file->next) {
		gchar *const source_uri = nautilus_file_info_get_uri(NAUTILUS_FILE_INFO(file->data));
		if (G_UNLIKELY(!source_uri))
			continue;

		// TODO: Check for errors.
		rp_menu_provider_convert_to_png(source_uri);

		g_free(source_uri);
	}

	nautilus_file_info_list_free(files);
	return nullptr;
}

static void
rp_item_convert_to_png(NautilusMenuItem *item, gpointer user_data)
{
	RP_UNUSED(user_data);

	GList *const files = static_cast<GList*>(g_object_steal_qdata(G_OBJECT(item), rp_item_convert_to_png_quark));
	if (G_UNLIKELY(!files))
		return;

	// Process the files in a separate thread.
	char thread_name[64];
	snprintf(thread_name, sizeof(thread_name), "rp-convert-to-png-%p", files);
	GThread *const thread = g_thread_new(thread_name, (GThreadFunc)rp_item_convert_to_png_ThreadFunc, files);

	// TODO: Do we want to keep a handle to the thread somewhere?
	g_thread_unref(thread);
}

static GList*
rp_nautilus_menu_provider_get_file_items(
	NautilusMenuProvider *provider,
#if !GTK_CHECK_VERSION(4,0,0)
	GtkWidget *window,
#endif /* !GTK_CHECK_VERSION(4,0,0) */
	GList *files)
{
	RP_UNUSED(provider);
#if !GTK_CHECK_VERSION(4,0,0)
	RP_UNUSED(window);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	// Verify that all specified files are supported.
	bool is_supported = false;
	int file_count = 0;
	for (GList *file = files; file != nullptr; file = file->next) {
		NautilusFileInfo *const file_info = NAUTILUS_FILE_INFO(file->data);

		// FIXME: We don't support writing to non-local files right now.
		// Only allow file:// protocol.
		gchar *const scheme = nautilus_file_info_get_uri_scheme(file_info);
		const bool is_file = (scheme && !g_ascii_strcasecmp(scheme, "file"));
		g_free(scheme);
		if (!is_file) {
			// Not file:// protocol.
			continue;
		}

		// Get the file's MIME type.
		gchar *const mime_type = nautilus_file_info_get_mime_type(file_info);
		if (!mime_type) {
			// No MIME type...
			continue;
		}

		// Check the file against all supported MIME types.
		static const char *const *const p_mime_types_convert_to_png_end =
			&mime_types_convert_to_png[ARRAY_SIZE(mime_types_convert_to_png)];
		is_supported = std::binary_search(mime_types_convert_to_png, p_mime_types_convert_to_png_end, mime_type,
			[](const char *a, const char *b) noexcept -> bool
			{
				return (strcmp(a, b) < 0);
			}
		);
		g_free(mime_type);
		if (!is_supported)
			break;

		file_count++;
	}

	if (!is_supported) {
		// One or more selected file(s) are not supported.
		return nullptr;
	}

	// Create the menu item.
	NautilusMenuItem *const item = nautilus_menu_item_new("rp-convert-to-png",
		C_("ServiceMenu", "Convert to PNG"),
		NC_("ServiceMenu",
			"Convert the selected texture file to PNG format.",
			"Convert the selected texture files to PNG format.",
			file_count),
		"image-png");

	// Save the file list in the menu item.
	g_object_set_qdata_full(G_OBJECT(item), rp_item_convert_to_png_quark,
		nautilus_file_info_list_copy(files),
		(GDestroyNotify)pfn_nautilus_file_info_list_free);
	g_signal_connect_closure(G_OBJECT(item), "activate",
		g_cclosure_new_object(G_CALLBACK(rp_item_convert_to_png), G_OBJECT(item)), TRUE);
	return g_list_prepend(nullptr, item);
}

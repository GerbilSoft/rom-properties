/******************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                            *
 * RpNautilusMenuProvider.cpp: Nautilus (and forks) Menu Provider Definition. *
 *                                                                            *
 * Copyright (c) 2017-2022 by David Korth.                                    *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

// Reference: https://github.com/xfce-mirror/thunar-archive-plugin/blob/master/thunar-archive-plugin/tap-provider.c

#include "stdafx.h"
#include "RpNautilusMenuProvider.hpp"

#include "img/TCreateThumbnail.hpp"
#include "CreateThumbnail.hpp"

#include "../RomDataView.hpp"

#include "librpbase/RomData.hpp"
using LibRpBase::RomData;

// nautilus-extension.h mini replacement
#include "nautilus-extension-mini.h"

// Supported MIME types
// TODO: Consolidate with the KF5 service menu?
static const char *const supported_mime_types[] = {
	"image/astc",
	"image/ktx",
	"image/ktx2",
	"image/vnd.ms-dds",
	"image/vnd.valve.source.texture",
	"image/x-didj-texture",
	"image/x-dds",
	"image/x-godot-stex",
	"image/x-sega-gvr",
	"image/x-sega-pvr",
	"image/x-sega-pvrx",
	"image/x-sega-svr",
	"image/x-vtf",
	"image/x-vtf3",
	"image/x-xbox-xpr0",
};

static GQuark rp_item_convert_to_png_quark;

static void   rp_nautilus_menu_provider_page_provider_init	(NautilusMenuProviderInterface *iface);
static GList *rp_nautilus_menu_provider_get_file_items		(NautilusMenuProvider          *provider,
								 GtkWidget                     *window,
								 GList                         *files);

struct _RpNautilusMenuProviderClass {
	GObjectClass __parent__;
};

struct _RpNautilusMenuProvider {
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
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpNautilusMenuProvider, rp_nautilus_menu_provider,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0),
	G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_MENU_PROVIDER,
		rp_nautilus_menu_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2,59,1)
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
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

static bool
is_file_uri(const gchar *uri)
{
	bool ret = false;
	if (G_UNLIKELY(!uri))
		return ret;

	gchar *const scheme = g_uri_parse_scheme(uri);
	if (!g_ascii_strcasecmp(scheme, "file")) {
		// It's file:// protocol!
		ret = true;
	}

	g_free(scheme);
	return ret;
}

static bool
is_file_uri(NautilusFileInfo *file_info)
{
	gchar *const uri = nautilus_file_info_get_uri(file_info);
	if (G_UNLIKELY(!uri))
		return false;

	const bool ret = is_file_uri(uri);
	g_free(uri);
	return ret;
}

static void
rp_item_convert_to_png(NautilusMenuItem *item, GtkWidget *window)
{
	RP_UNUSED(window);

	GList *const files = static_cast<GList*>(g_object_get_qdata(G_OBJECT(item), rp_item_convert_to_png_quark));
	if (G_UNLIKELY(!files))
		return;

	for (GList *file = files; file != nullptr; file = file->next) {
		gchar *const source_uri = nautilus_file_info_get_uri(NAUTILUS_FILE_INFO(file->data));
		if (G_UNLIKELY(!source_uri))
			continue;

		// FIXME: We don't support writing to non-local files right now.
		// Only allow file:// protocol.
		if (!is_file_uri(source_uri)) {
			// Not file:// protocol.
			g_free(source_uri);
			continue;
		}

		// Create the output filename based on the input filename.
		const size_t source_len = strlen(source_uri);
		if (source_len < 8) {
			// Doesn't have "file://".
			g_free(source_uri);
		}
		// Skip the "file://" portion.
		// NOTE: Needs to be urldecoded.
		const size_t output_len_esc = source_len - 7 + 16;
		gchar *output_file_esc = static_cast<gchar*>(g_malloc(output_len_esc));
		g_strlcpy(output_file_esc, &source_uri[7], output_len_esc);

		// Find the current extension and replace it.
		gchar *const dotpos = strrchr(output_file_esc, '.');
		if (!dotpos) {
			// No file extension. Add it.
			g_strlcat(output_file_esc, ".png", output_len_esc);
		} else {
			// If the dot is after the last slash, we already have a file extension.
			// Otherwise, we don't have one, and need to add it.
			gchar *const slashpos = strrchr(output_file_esc, DIR_SEP_CHR);
			if (slashpos < dotpos) {
				// We already have a file extension.
				strcpy(dotpos, ".png");
			} else {
				// No file extension.
				g_strlcat(output_file_esc, ".png", output_len_esc);
			}
		}

		// Unescape the URI.
		gchar *const output_file = g_uri_unescape_string(output_file_esc, nullptr);

		// Convert the file using rp_create_thumbnail2().
		// TODO: Check for errors?
		rp_create_thumbnail2(source_uri, output_file, 0, RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA);
		g_free(source_uri);
		g_free(output_file_esc);
		g_free(output_file);
	}
}

static GList*
rp_nautilus_menu_provider_get_file_items(NautilusMenuProvider *provider, GtkWidget *window, GList *files)
{
	RP_UNUSED(provider);
	RP_UNUSED(window);

	// Verify that all specified files are supported.
	bool is_supported = false;
	int file_count = 0;
	for (GList *file = files; file != nullptr; file = file->next) {
		NautilusFileInfo *const file_info = NAUTILUS_FILE_INFO(file->data);

		// FIXME: We don't support writing to non-local files right now.
		// Only allow file:// protocol.
		if (!is_file_uri(file_info)) {
			// Not file:// protocol.
			continue;
		}

		// Check the file against all supported MIME types.
		// TODO: Use nautilus_file_info_get_mime_type, then do a binary search?
		is_supported = std::any_of(supported_mime_types, &supported_mime_types[ARRAY_SIZE(supported_mime_types)],
			[file_info](const char *mime_type) {
				return nautilus_file_info_is_mime_type(file_info, mime_type);
			}
		);
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
		g_cclosure_new_object(G_CALLBACK(rp_item_convert_to_png), G_OBJECT(window)), TRUE);
	return g_list_append(nullptr, item);
}

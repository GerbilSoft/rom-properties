/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * ThunarMenuProvider.c: ThunarX Menu Provider Definition                  *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://github.com/xfce-mirror/thunar-archive-plugin/blob/master/thunar-archive-plugin/tap-provider.c

#include "stdafx.h"
#include "ThunarMenuProvider.h"
#include "MenuProviderCommon.h"

// thunarx.h mini replacement
#include "thunarx-mini.h"

static GQuark rp_item_convert_to_png_quark;

static void   rp_thunar_menu_provider_page_provider_init	(ThunarxMenuProviderIface *iface);
static GList *rp_thunar_menu_provider_get_file_menu_items	(ThunarxMenuProvider      *provider,
								 GtkWidget                *window,
								 GList                    *files)
								G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

struct _RpThunarMenuProviderClass {
	GObjectClass __parent__;
};

struct _RpThunarMenuProvider {
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
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RpThunarMenuProvider, rp_thunar_menu_provider,
	G_TYPE_OBJECT, (GTypeFlags)0,
	G_IMPLEMENT_INTERFACE_DYNAMIC(THUNARX_TYPE_MENU_PROVIDER,
		rp_thunar_menu_provider_page_provider_init));

#if !GLIB_CHECK_VERSION(2, 59, 1)
#  if defined(__GNUC__) && __GNUC__ > 8
#    pragma GCC diagnostic pop
#  endif
#endif /* !GLIB_CHECK_VERSION(2, 59, 1) */

void
rp_thunar_menu_provider_register_type_ext(ThunarxProviderPlugin *plugin)
{
	rp_thunar_menu_provider_register_type(G_TYPE_MODULE(plugin));
}

static void
rp_thunar_menu_provider_class_init(RpThunarMenuProviderClass *klass)
{
	RP_UNUSED(klass);

	// Get quarks for the various GLib strings.
	rp_item_convert_to_png_quark = g_quark_from_string("rp-item-convert-to-png");
}

static void
rp_thunar_menu_provider_class_finalize(RpThunarMenuProviderClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_thunar_menu_provider_init(RpThunarMenuProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rp_thunar_menu_provider_page_provider_init(ThunarxMenuProviderIface *iface)
{
	iface->get_file_menu_items = rp_thunar_menu_provider_get_file_menu_items;
}

static gpointer
rp_item_convert_to_png_ThreadFunc(GList *files)
{
	for (GList *file = files; file != NULL; file = file->next) {
		gchar *const source_uri = thunarx_file_info_get_uri(THUNARX_FILE_INFO(file->data));
		if (G_UNLIKELY(!source_uri))
			continue;

		// TODO: Check for errors.
		rp_menu_provider_convert_to_png(source_uri);

		g_free(source_uri);
	}

	thunarx_file_info_list_free(files);
	return NULL;
}

#if GTK_CHECK_VERSION(3, 0, 0)
typedef ThunarxMenuItem MenuItem_t;
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkAction MenuItem_t;
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

static void
rp_item_convert_to_png(MenuItem_t *item, gpointer user_data)
{
	RP_UNUSED(user_data);

	GList *const files = (GList*)g_object_steal_qdata(G_OBJECT(item), rp_item_convert_to_png_quark);
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
rp_thunar_menu_provider_get_file_menu_items(ThunarxMenuProvider *provider, GtkWidget *window, GList *files)
{
	RP_UNUSED(window);
	assert(RP_IS_THUNAR_MENU_PROVIDER(provider));
	g_return_val_if_fail(RP_IS_THUNAR_MENU_PROVIDER(provider), NULL);

	// Verify that all specified files are supported.
	bool is_supported = false;
	int file_count = 0;
	for (GList *file = files; file != NULL; file = file->next) {
		ThunarxFileInfo *const file_info = THUNARX_FILE_INFO(file->data);

		// FIXME: We don't support writing to non-local files right now.
		// Only allow file:// protocol.
		gchar *const scheme = thunarx_file_info_get_uri_scheme(file_info);
		const bool is_file = (scheme && !g_ascii_strcasecmp(scheme, "file"));
		g_free(scheme);
		if (!is_file) {
			// Not file:// protocol.
			continue;
		}

		// Get the file's MIME type.
		gchar *const mime_type = thunarx_file_info_get_mime_type(file_info);
		if (!mime_type) {
			// No MIME type...
			continue;
		}

		// Check if the MIME type is supported.
		is_supported = rp_menu_provider_is_mime_type_supported(mime_type);
		g_free(mime_type);
		if (!is_supported)
			break;

		file_count++;
	}

	if (!is_supported) {
		// One or more selected file(s) are not supported.
		return NULL;
	}

	// Create the menu item.
	// NOTE: Starting with Thunar 1.7/1.8 (GTK3), ThunarxMenuItem is used.
	// Previous versions (GTK2) used GtkAction.
#if GTK_CHECK_VERSION(3, 0, 0)
	ThunarxMenuItem *const item = thunarx_menu_item_new(
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
	GtkAction *const item = gtk_action_new(
#endif /* GTK_CHECK_VERSION(3, 0, 0) */
		"rp-convert-to-png",
		C_("ServiceMenu", "Convert to PNG"),
		NC_("ServiceMenu",
			"Convert the selected texture file to PNG format.",
			"Convert the selected texture files to PNG format.",
			file_count),
		"image-png");

#if GTK_CHECK_VERSION(2, 15, 1) && !GTK_CHECK_VERSION(3, 0, 0)
	// Set the GtkAction's icon name.
	gtk_action_set_icon_name(item, "image-png");
#endif /* GTK_CHECK_VERSION(2, 16, 0) && !GTK_CHECK_VERSION(3, 0, 0) */

	// Save the file list in the menu item.
	g_object_set_qdata_full(G_OBJECT(item), rp_item_convert_to_png_quark,
		thunarx_file_info_list_copy(files),
		(GDestroyNotify)pfn_thunarx_file_info_list_free);
	g_signal_connect_closure(G_OBJECT(item), "activate",
		g_cclosure_new_object(G_CALLBACK(rp_item_convert_to_png), G_OBJECT(window)), TRUE);
	return g_list_prepend(NULL, item);
}

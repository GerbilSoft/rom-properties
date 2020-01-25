/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-page.cpp: ThunarX Properties Page.                       *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - audio-tags plugin
 * - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html
 */

#include "rom-properties-page.hpp"
#include "../RomDataView.hpp"

// libi18n
#include "libi18n/i18n.h"

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

/* Property identifiers */
enum {
	PROP_0,
	PROP_FILE,
	PROP_LAST
};

static void	rom_properties_page_dispose		(GObject		*object);
static void	rom_properties_page_finalize		(GObject		*object);
static void	rom_properties_page_get_property        (GObject		*object,
							 guint			 prop_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_set_property	(GObject		*object,
							 guint			 prop_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_file_changed	(ThunarxFileInfo	*file,
							 RomPropertiesPage      *page);

// XFCE property page class.
struct _RomPropertiesPageClass {
	ThunarxPropertyPageClass __parent__;
};

// XFCE property page.
struct _RomPropertiesPage {
	ThunarxPropertyPage __parent__;

	/* RomDataView */
	GtkWidget	*romDataView;

	/* Properties */
	ThunarxFileInfo	*file;

	// Signal handler ID for file::changed()
	gulong file_changed_signal_handler_id;
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

// NOTE: THUNARX_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
THUNARX_DEFINE_TYPE_EXTENDED(RomPropertiesPage, rom_properties_page,
	THUNARX_TYPE_PROPERTY_PAGE, static_cast<GTypeFlags>(0), {});

#if !GLIB_CHECK_VERSION(2,59,1)
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
#endif /* !GLIB_CHECK_VERSION(2,59,1) */

static GParamSpec *properties[PROP_LAST];

static void
rom_properties_page_class_init(RomPropertiesPageClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = rom_properties_page_dispose;
	gobject_class->finalize = rom_properties_page_finalize;
	gobject_class->get_property = rom_properties_page_get_property;
	gobject_class->set_property = rom_properties_page_set_property;

	/**
	 * RomPropertiesPage:file:
	 *
	 * The #ThunarxFileInfo being displayed on this page.
	 **/
	properties[PROP_FILE] = g_param_spec_object(
		"file", "File", "ThunarxFileInfo of the ROM image being displayed.",
		THUNARX_TYPE_FILE_INFO,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property(gobject_class, PROP_FILE, properties[PROP_FILE]);
}

static void
rom_properties_page_init(RomPropertiesPage *page)
{
	page->file = nullptr;
	page->file_changed_signal_handler_id = 0;

	// Initialize the RomDataView.
	page->romDataView = rom_data_view_new();
	rom_data_view_set_desc_format_type(ROM_DATA_VIEW(page->romDataView), RP_DFT_XFCE);
	gtk_container_add(GTK_CONTAINER(page), page->romDataView);
	gtk_widget_show(page->romDataView);
}

static void
rom_properties_page_dispose(GObject *object)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	// Disconnect signals.
	if (G_LIKELY(page->file_changed_signal_handler_id > 0)) {
		g_signal_handler_disconnect(page->file, page->file_changed_signal_handler_id);
		page->file_changed_signal_handler_id = 0;
	}

	// Unreference the file.
	// NOTE: Might not be needed, but Nautilus 3.x does this.
	if (G_LIKELY(page->file)) {
		g_object_unref(page->file);
		page->file = nullptr;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rom_properties_page_parent_class)->dispose(object);
}

static void
rom_properties_page_finalize(GObject *object)
{
	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rom_properties_page_parent_class)->finalize(object);
}

RomPropertiesPage*
rom_properties_page_new(void)
{
	// tr: Tab title.
	const char *const tabTitle = C_("RomDataView", "ROM Properties");

	RomPropertiesPage *page = static_cast<RomPropertiesPage*>(
		g_object_new(TYPE_ROM_PROPERTIES_PAGE, nullptr));
	thunarx_property_page_set_label(THUNARX_PROPERTY_PAGE(page), tabTitle);
	return page;
}

static void
rom_properties_page_get_property(GObject	*object,
				 guint		 prop_id,
				 GValue		*value,
				 GParamSpec	*pspec)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	switch (prop_id) {
		case PROP_FILE:
			g_value_set_object(value, rom_properties_page_get_file(page));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rom_properties_page_set_property(GObject	*object,
				 guint		 prop_id,
				 const GValue	*value,
				 GParamSpec	*pspec)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	switch (prop_id) {
		case PROP_FILE:
			rom_properties_page_set_file(page, static_cast<ThunarxFileInfo*>(g_value_get_object(value)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * rom_properties_page_get_file:
 * @page : a #RomPropertiesPage.
 *
 * Returns the current #ThunarxFileInfo
 * for the @page.
 *
 * Return value: the file associated with this property page.
 **/
ThunarxFileInfo*
rom_properties_page_get_file(RomPropertiesPage *page)
{
	g_return_val_if_fail(IS_ROM_PROPERTIES_PAGE(page), nullptr);
	return page->file;
}

/**
 * rom_properties_page_set_file:
 * @page : a #RomPropertiesPage.
 * @file : a #ThunarxFileInfo
 *
 * Sets the #ThunarxFileInfo for this @page.
 **/
void
rom_properties_page_set_file	(RomPropertiesPage	*page,
				 ThunarxFileInfo	*file)
{
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(file == nullptr || THUNARX_IS_FILE_INFO(file));

	/* Check if we already use this file */
	if (G_UNLIKELY(page->file == file))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(page->file != nullptr))
	{
		if (G_LIKELY(page->file_changed_signal_handler_id > 0)) {
			g_signal_handler_disconnect(page->file, page->file_changed_signal_handler_id);
			page->file_changed_signal_handler_id = 0;
		}
		g_object_unref(G_OBJECT(page->file));
	}

	/* Assign the value */
	page->file = file;

	/* Connect to the new file (if any) */
	if (G_LIKELY(file != nullptr)) {
		/* Take a reference on the info file */
		g_object_ref(G_OBJECT(page->file));

		rom_properties_page_file_changed(file, page);
		page->file_changed_signal_handler_id = g_signal_connect(G_OBJECT(file), "changed",
			G_CALLBACK(rom_properties_page_file_changed), page);
	} else {
		// Clear the file.
		rom_data_view_set_uri(ROM_DATA_VIEW(page->romDataView), nullptr);
	}

	// File has been changed.
	g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_FILE]);
}

static void
rom_properties_page_file_changed(ThunarxFileInfo	*file,
				 RomPropertiesPage	*page)
{
	g_return_if_fail(THUNARX_IS_FILE_INFO(file));
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(page->file == file);

	// Get the URI.
	gchar *const uri = thunarx_file_info_get_uri(page->file);

	// FIXME: This only works on initial load.
	// Need to update it to reload the ROM on file change.
	// Also, ThunarxFileInfo emits 'changed' *twice* for file changes...
	rom_data_view_set_uri(ROM_DATA_VIEW(page->romDataView), uri);
	g_free(uri);
}

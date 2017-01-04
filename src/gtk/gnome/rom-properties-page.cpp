/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-page.cpp: Nautilus Properties Page.                      *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "rom-properties-page.hpp"
#include "GdkImageConv.hpp"

#include "../RomDataView.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

/**
 * References:
 * - audio-tags plugin for Xfce/Thunar
 * - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page-provider.html
 */

/* Property identifiers */
enum {
	PROP_0,
	PROP_FILE,
};

static void	rom_properties_page_finalize(GObject *object);
static void	rom_properties_page_get_property        (GObject		*object,
							 guint			 prop_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_set_property	(GObject		*object,
							 guint			 prop_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_file_changed	(NautilusFileInfo	*file,
							 RomPropertiesPage      *page);

// XFCE property page class.
struct _RomPropertiesPageClass {
	GtkBoxClass	__parent__;
};

// XFCE property page.
struct _RomPropertiesPage {
	GtkBox		__parent__;

	/* RomDataView */
	GtkWidget	*romDataView;

	/* Properties */
	NautilusFileInfo	*file;
};

G_DEFINE_DYNAMIC_TYPE(RomPropertiesPage, rom_properties_page, GTK_TYPE_BOX);

void
rom_properties_page_register_type_ext(GTypeModule *module)
{
	rom_properties_page_register_type(module);
}

static void
rom_properties_page_class_init(RomPropertiesPageClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = rom_properties_page_finalize;
	gobject_class->get_property = rom_properties_page_get_property;
	gobject_class->set_property = rom_properties_page_set_property;

	/**
	 * RomPropertiesPage:file:
	 *
	 * The #NautilusFileInfo modified on this page.
	 **/
	g_object_class_install_property(gobject_class, PROP_FILE,
		g_param_spec_object("file", "file", "file",
			NAUTILUS_TYPE_FILE_INFO, G_PARAM_READWRITE));
}

static void
rom_properties_page_class_finalize(RomPropertiesPageClass *klass)
{
	((void)klass);
}

static void
rom_properties_page_init(RomPropertiesPage *page)
{
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(page), GTK_ORIENTATION_VERTICAL);

	// Initialize the RomDataView.
	// TODO: GNOME uses left-aligned, unbolded description labels.
	page->romDataView = rom_data_view_new();
	gtk_container_add(GTK_CONTAINER(page), page->romDataView);
	gtk_widget_show(page->romDataView);
	gtk_widget_show(GTK_WIDGET(page));
}

static void
rom_properties_page_finalize(GObject *object)
{
	(*G_OBJECT_CLASS(rom_properties_page_parent_class)->finalize)(object);
}

RomPropertiesPage*
rom_properties_page_new(void)
{
	return static_cast<RomPropertiesPage*>(g_object_new(TYPE_ROM_PROPERTIES_PAGE, nullptr));
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
			rom_properties_page_set_file(page, static_cast<NautilusFileInfo*>(g_value_get_object(value)));
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
 * Returns the current #NautilusFileInfo
 * for the @page.
 *
 * Return value: the file associated with this property page.
 **/
NautilusFileInfo*
rom_properties_page_get_file(RomPropertiesPage *page)
{
	g_return_val_if_fail(IS_ROM_PROPERTIES_PAGE(page), nullptr);
	return page->file;
}

/**
 * rom_properties_page_set_file:
 * @page : a #RomPropertiesPage.
 * @file : a #NautilusFileInfo
 *
 * Sets the #NautilusFileInfo for this @page.
 **/
void
rom_properties_page_set_file	(RomPropertiesPage	*page,
				 NautilusFileInfo	*file)
{
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(file == nullptr || NAUTILUS_IS_FILE_INFO(file));

	/* Check if we already use this file */
	if (G_UNLIKELY(page->file == file))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(page->file != nullptr))
	{
		g_signal_handlers_disconnect_by_func(G_OBJECT(page->file),
			reinterpret_cast<gpointer>(rom_properties_page_file_changed), page);
		g_object_unref(G_OBJECT(page->file));
	}

	/* Assign the value */
	page->file = file;

	/* Connect to the new file (if any) */
	if (G_LIKELY(file != nullptr)) {
		/* Take a reference on the info file */
		g_object_ref(G_OBJECT(page->file));

		rom_properties_page_file_changed(file, page);
		g_signal_connect(G_OBJECT(file), "changed", G_CALLBACK(rom_properties_page_file_changed), page);
	} else {
		// Clear the file.
		rom_data_view_set_filename(ROM_DATA_VIEW(page->romDataView), nullptr);
	}
}

static void
rom_properties_page_file_changed(NautilusFileInfo	*file,
				 RomPropertiesPage	*page)
{
	g_return_if_fail(NAUTILUS_IS_FILE_INFO(file));
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(page->file == file);

	// Get the filename.
	gchar *uri = nautilus_file_info_get_uri(page->file);
	gchar *filename = g_filename_from_uri(uri, nullptr, nullptr);
	g_free(uri);

	rom_data_view_set_filename(ROM_DATA_VIEW(page->romDataView), filename);
	g_free(filename);
}

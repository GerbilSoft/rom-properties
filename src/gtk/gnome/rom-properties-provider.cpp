/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rom-properties-provider.cpp: Nautilus Provider Definition.              *
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

/**
 * References:
 * - audio-tags plugin for Xfce/Thunar
 * - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page.html
 * - https://developer.gnome.org/libnautilus-extension//3.16/libnautilus-extension-nautilus-property-page-provider.html
 * - https://github.com/GNOME/nautilus/blob/bb433582165da10ab07337d707ea448703c3865f/src/nautilus-image-properties-page.c
 */

#include "rom-properties-provider.hpp"

#include "libromdata/file/RpFile.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RpFile;
using LibRomData::RomData;
using LibRomData::RomDataFactory;

#include "../RomDataView.hpp"

static void   rom_properties_provider_page_provider_init	(NautilusPropertyPageProviderIface	*iface);
static GList *rom_properties_provider_get_pages			(NautilusPropertyPageProvider		*provider,
								 GList					*files);

struct _RomPropertiesProviderClass {
	GObjectClass __parent__;
};

struct _RomPropertiesProvider {
	GObject __parent__;
};

// FIXME: G_DEFINE_DYNAMIC_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_DYNAMIC_TYPE_EXTENDED(RomPropertiesProvider, rom_properties_provider,
	G_TYPE_OBJECT, 0,
	G_IMPLEMENT_INTERFACE(NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
			rom_properties_provider_page_provider_init));

void
rom_properties_provider_register_type_ext(GTypeModule *module)
{
	rom_properties_provider_register_type(module);
}

static void
rom_properties_provider_class_init(RomPropertiesProviderClass *klass)
{
	((void)klass);
}

static void
rom_properties_provider_class_finalize(RomPropertiesProviderClass *klass)
{
	((void)klass);
}

static void
rom_properties_provider_init(RomPropertiesProvider *sbr_provider)
{
	((void)sbr_provider);
}

static void
rom_properties_provider_page_provider_init(NautilusPropertyPageProviderIface *iface)
{
	iface->get_pages = rom_properties_provider_get_pages;
}

static GList*
rom_properties_provider_get_pages(NautilusPropertyPageProvider *provider, GList *files)
{
	GList *pages = nullptr;
	GList *file;
	NautilusFileInfo *info;
	((void)provider);

	if (g_list_length(files) != 1) 
		return nullptr;

	file = g_list_first(files);
	if (G_UNLIKELY(file == nullptr))
		return nullptr;

	info = NAUTILUS_FILE_INFO(file->data);

	if (G_LIKELY(rom_properties_get_file_supported(info))) {
		// Get the filename.
		gchar *uri = nautilus_file_info_get_uri(info);
		gchar *filename = g_filename_from_uri(uri, nullptr, nullptr);
		g_free(uri);

		// Create the RomDataView.
		// NOTE: Unlike the Xfce/Thunar version, we don't
		// need to subclass NautilusPropertyPage. Instead,
		// we create a NautilusPropertyPage and add a
		// RomDataView widget to it.
		// TODO: GNOME uses left-aligned, non-bold description labels.
		// TODO: Add some extra padding to the top...
		GtkWidget *romDataView = static_cast<GtkWidget*>(
			g_object_new(rom_data_view_get_type(), nullptr));
		rom_data_view_set_desc_format_type(ROM_DATA_VIEW(romDataView), RP_DFT_GNOME);
		rom_data_view_set_filename(ROM_DATA_VIEW(romDataView), filename);
		gtk_widget_show(romDataView);
		g_free(filename);

		// Create the NautilusPropertyPage.
		NautilusPropertyPage *page = nautilus_property_page_new(
			"RomPropertiesPage::property_page",
			gtk_label_new("ROM Properties"),
			romDataView);

		/* Add the page to the pages provided by this plugin */
		pages = g_list_prepend(pages, page);
	}

	return pages;
}

gboolean
rom_properties_get_file_supported(NautilusFileInfo *info)
{
	gchar *uri;
	gchar *filename;
	gboolean supported = FALSE;

	g_return_val_if_fail(info != nullptr || NAUTILUS_IS_FILE_INFO(info), FALSE);

	// TODO: Support for gvfs.
	uri = nautilus_file_info_get_uri(info);
	filename = g_filename_from_uri(uri, nullptr, nullptr);
	g_free(uri);

	if (G_UNLIKELY(filename == nullptr))
		return FALSE;

	// TODO: Check file extensions and/or MIME types?

	// Open the ROM file.
	RpFile *file = new RpFile(filename, RpFile::FM_OPEN_READ);
	if (file->isOpen()) {
		// Is this ROM file supported?
		// NOTE: We have to create an instance here in order to
		// prevent false positives caused by isRomSupported()
		// saying "yes" while new RomData() says "no".
		RomData *romData = RomDataFactory::create(file, false);
		if (romData != nullptr) {
			supported = TRUE;
			romData->unref();
		}
		delete file;
	}

	g_free(filename);
	return supported;
}

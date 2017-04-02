/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-provider.cpp: ThunarX Provider Definition.               *
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

#include "rom-properties-provider.hpp"
#include "rom-properties-page.hpp"

#include "libromdata/file/RpFile.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RpFile;
using LibRomData::RomData;
using LibRomData::RomDataFactory;

static void   rom_properties_provider_page_provider_init	(ThunarxPropertyPageProviderIface *iface);
static GList *rom_properties_provider_get_pages			(ThunarxPropertyPageProvider      *renamer_provider,
								 GList                            *files);

struct _RomPropertiesProviderClass {
	GObjectClass __parent__;
};

struct _RomPropertiesProvider {
	GObject __parent__;
};


// FIXME: THUNARX_DEFINE_TYPE_WITH_CODE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
THUNARX_DEFINE_TYPE_EXTENDED(RomPropertiesProvider, rom_properties_provider,
	G_TYPE_OBJECT, static_cast<GTypeFlags>(0),
	THUNARX_IMPLEMENT_INTERFACE(THUNARX_TYPE_PROPERTY_PAGE_PROVIDER,
			rom_properties_provider_page_provider_init));

static void
rom_properties_provider_class_init (RomPropertiesProviderClass *klass)
{
	((void)klass);
}

static void
rom_properties_provider_init(RomPropertiesProvider *sbr_provider)
{
	((void)sbr_provider);
}

static void
rom_properties_provider_page_provider_init(ThunarxPropertyPageProviderIface *iface)
{
	iface->get_pages = rom_properties_provider_get_pages;
}

static GList*
rom_properties_provider_get_pages(ThunarxPropertyPageProvider *page_provider, GList *files)
{
	GList *pages = nullptr;
	GList *file;
	ThunarxFileInfo *info;
	((void)page_provider);

	if (g_list_length(files) != 1) 
		return nullptr;

	file = g_list_first(files);
	if (G_UNLIKELY(file == nullptr))
		return nullptr;

	info = THUNARX_FILE_INFO(file->data);

	if (G_LIKELY(rom_properties_get_file_supported(info))) {
		// Create the ROM Properties page.
		RomPropertiesPage *page = rom_properties_page_new();

		/* Assign supported file info to the page */
		rom_properties_page_set_file(page, info);

		/* Add the page to the pages provided by this plugin */
		pages = g_list_prepend(pages, page);
	}

	return pages;
}

gboolean
rom_properties_get_file_supported(ThunarxFileInfo *info)
{
	gchar *uri;
	gchar *filename;
	gboolean supported = FALSE;

	g_return_val_if_fail(info != nullptr || THUNARX_IS_FILE_INFO (info), FALSE);

	// TODO: Support for gvfs.
	uri = thunarx_file_info_get_uri(info);
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

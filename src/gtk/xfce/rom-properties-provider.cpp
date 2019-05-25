/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-provider.cpp: ThunarX Provider Definition.               *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "rom-properties-provider.hpp"
#include "rom-properties-page.hpp"

// librpbase
#include "librpbase/file/RpFile.hpp"
#include "librpbase/RomData.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
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
	RP_UNUSED(klass);
}

static void
rom_properties_provider_init(RomPropertiesProvider *sbr_provider)
{
	RP_UNUSED(sbr_provider);
}

static void
rom_properties_provider_page_provider_init(ThunarxPropertyPageProviderIface *iface)
{
	iface->get_pages = rom_properties_provider_get_pages;
}

static GList*
rom_properties_provider_get_pages(ThunarxPropertyPageProvider *page_provider, GList *files)
{
	RP_UNUSED(page_provider);
	GList *pages = nullptr;
	GList *file;
	ThunarxFileInfo *info;

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
	gboolean supported = false;

	g_return_val_if_fail(info != nullptr || THUNARX_IS_FILE_INFO (info), false);

	// TODO: Support for gvfs.
	uri = thunarx_file_info_get_uri(info);
	filename = g_filename_from_uri(uri, nullptr, nullptr);
	g_free(uri);

	if (G_UNLIKELY(filename == nullptr))
		return false;

	// TODO: Check file extensions and/or MIME types?

	// Open the ROM file.
	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
	if (file->isOpen()) {
		// Is this ROM file supported?
		// NOTE: We have to create an instance here in order to
		// prevent false positives caused by isRomSupported()
		// saying "yes" while new RomData() says "no".
		RomData *const romData = RomDataFactory::create(file);
		if (romData != nullptr) {
			supported = true;
			romData->unref();
		}
	}
	file->unref();

	g_free(filename);
	return supported;
}

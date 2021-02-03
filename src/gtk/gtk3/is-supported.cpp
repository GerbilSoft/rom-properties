/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ 3.x)                         *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "is-supported.hpp"

// librpfile, librpbase, libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRpFile::IRpFile;
using LibRpFile::RpFile;
using LibRpBase::RomData;
using LibRomData::RomDataFactory;

/**
 * Check if the specified URI is supported.
 * @param uri URI rom e.g. nautilus_file_info_get_uri().
 * @return True if supported; false if not.
 */
gboolean rp_gtk3_is_uri_supported(const gchar *uri)
{
	gboolean supported = false;
	g_return_val_if_fail(uri != nullptr && uri[0] != '\0', false);

	// TODO: Check file extensions and/or MIME types?

	// Check if the URI maps to a local file.
	IRpFile *file = nullptr;
	gchar *const filename = g_filename_from_uri(uri, nullptr, nullptr);
	if (filename) {
		// Local file. Use RpFile.
		file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
		g_free(filename);
	} else {
		// Not a local file. Use RpFileGio.
		file = new RpFileGio(uri);
	}

	// Open the ROM file.
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

	return supported;
}

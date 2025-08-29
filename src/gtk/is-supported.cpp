/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "is-supported.hpp"

#include <glib.h>

// librpbase, librpfile, libromdata
#include "libromdata/RomDataFactory.hpp"
#include "librpfile/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * @param uri URI from e.g. nautilus_file_info_get_uri() [UTF-8]
 * @return RomData object if supported; nullptr if not.
 */
RomDataPtr rp_gtk_open_uri(const char *uri)
{
	g_return_val_if_fail(uri != nullptr && uri[0] != '\0', nullptr);

	// TODO: Check file extensions and/or MIME types?

	// Check if the URI maps to a local file.
	gchar *const filename = g_filename_from_uri(uri, nullptr, nullptr);
	if (filename) {
		RomDataPtr romData = RomDataFactory::create(filename);
		g_free(filename);
		return romData;
	}

	// This might be a plain filename and not a URI.
	RomDataPtr romData;
	if (access(uri, R_OK) == 0) {
		// It's a plain filename.
		romData = RomDataFactory::create(uri);
	} else {
		// Not a local file. Use RpFileGio.
		IRpFilePtr file = std::make_shared<RpFileGio>(uri);
		if (file->isOpen()) {
			romData = RomDataFactory::create(file);
		}
	}

	return romData;
}

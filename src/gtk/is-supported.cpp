/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
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
 * Attempt to open a RomData object from the specified filename.
 * @param filename Filename (must be a filename, NOT a URI!)
 * @return RomData object if supported; nullptr if not.
 */
static RomDataPtr rp_gtk_open_filename(const char *filename)
{
	// TODO: Check file extensions and/or MIME types?
	RomDataPtr romData;

	// Local file:
	// - If this is a diretory: Call RomDataFactory::create() with the pathname.
	// - If this is a file: Create an RpFile first.
	if (likely(!FileSystem::is_directory(filename))) {
		// File: Open the file and call RomDataFactory::create() with the opened file.
		IRpFilePtr file = std::make_shared<RpFile>(filename, RpFile::FM_OPEN_READ_GZ);
		if (!file->isOpen()) {
			// Unable to open the file...
			// TODO: Return an error code?
			return {};
		}
		romData = RomDataFactory::create(file);
	} else {
		// Directory: Call RomDataFactory::create() with the filename.
		// (NOTE: Local filenames only!)
		romData = RomDataFactory::create(filename);
	}

	return romData;
}


/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * @param uri URI from e.g. nautilus_file_info_get_uri() [UTF-8]
 * @return RomData object if supported; nullptr if not.
 */
RomDataPtr rp_gtk_open_uri(const char *uri)
{
	g_return_val_if_fail(uri != nullptr && uri[0] != '\0', nullptr);

	// TODO: Check file extensions and/or MIME types?
	RomDataPtr romData;

	// Check if the URI maps to a local file.
	gchar *const filename = g_filename_from_uri(uri, nullptr, nullptr);
	if (filename) {
		romData = rp_gtk_open_filename(filename);
		g_free(filename);
	} else {
		// This might be a plain filename and not a URI.
		if (access(uri, R_OK) == 0) {
			// It's a plain filename.
			romData = rp_gtk_open_filename(uri);
		} else {
			// Not a local file. Use RpFileGio.
			IRpFilePtr file = std::make_shared<RpFileGio>(uri);
			if (file->isOpen()) {
				romData = RomDataFactory::create(file);
			}
		}
	}

	return romData;
}

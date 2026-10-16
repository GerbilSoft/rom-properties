/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * is-supported.hpp: Check if a URI is supported.                          *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "is-supported.hpp"

#include <glib.h>
#include "RpFile_gio.hpp"

// Other rom-properties libraries
#include "librpbase/config/Config.hpp"
#include "librpfile/FileSystem.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C includes
#include <unistd.h>

/**
 * Attempt to open a RomData object from the specified filename.
 * @param filename Filename [UTF-8]
 * @param isThumbnail If true, this file is being used for thumbnailing. Handle "bad FS" checking.
 * @return RomData object if supported; nullptr if not.
 */
static RomDataPtr rp_gtk_open_filename(const char *filename, bool isThumbnail)
{
	if (isThumbnail) {
		const Config *const config = Config::instance();

		if (unlikely(FileSystem::is_directory(filename))) {
			// This is a directory. Check if directory thumbnailing is enabled.
			const bool thumbnailDirectoryPackages = config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages);
			if (!thumbnailDirectoryPackages) {
				// Directory package thumbnailing is disabled.
				return {};
			}
		}

		// We're thumbnailing the file. Check "bad FS" settings.
		const bool enableThumbnailOnNetworkFS = config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS);
		if (FileSystem::isOnBadFS(filename, enableThumbnailOnNetworkFS)) {
			// This file is on a "bad" file system.
			return {};
		}
	}

	// Open the file.
	return RomDataFactory::create(filename);
}

/**
 * Attempt to open a RomData object from the specified GVfs URI.
 * @param uri URI from e.g. nautilus_file_info_get_uri() [UTF-8]
 * @param isThumbnail If true, this file is being used for thumbnailing. Handle "bad FS" checking.
 * @return RomData object if supported; nullptr if not.
 */
RomDataPtr rp_gtk_open_uri(const char *uri, bool isThumbnail)
{
	RomDataPtr romData;
	g_return_val_if_fail(uri != nullptr && uri[0] != '\0', romData);

	// TODO: Check file extensions and/or MIME types?

	// Check if the URI maps to a local file.
	gchar *const filename = g_filename_from_uri(uri, nullptr, nullptr);
	if (filename) {
		// It's a local file.
		romData = rp_gtk_open_filename(filename, isThumbnail);
		g_free(filename);
		return romData;
	}

	// This might be a plain filename and not a URI.
	if (access(uri, R_OK) == 0) {
		// It's a plain filename.
		romData = rp_gtk_open_filename(uri, isThumbnail);
	} else {
		// Not a local file. Use RpFileGio.
		// TODO: Check if it's a directory?
		const Config *const config = Config::instance();
		const bool enableThumbnailOnNetworkFS = config->getBoolConfigOption(Config::BoolConfig::Options_EnableThumbnailOnNetworkFS);
		if (enableThumbnailOnNetworkFS) {
			IRpFilePtr file = std::make_shared<RpFileGio>(uri);
			if (file->isOpen()) {
				romData = RomDataFactory::create(file);
			}
		}
	}

	return romData;
}

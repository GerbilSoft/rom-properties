/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"

// C++ includes.
#include <vector>

namespace LibRpBase {
	class RomData;
}
namespace LibRpFile {
	class IRpFile;
}

namespace LibRomData {

class RomDataFactory
{
	public:
		// Static class
		RomDataFactory() = delete;
		~RomDataFactory() = delete;
	private:
		RP_DISABLE_COPY(RomDataFactory)

	public:
		/**
		 * Bitfield of RomData subclass attributes.
		 */
		enum RomDataAttr {
			// RomData subclass has no attributes.
			RDA_NONE		= 0,

			// RomData subclass has thumbnails.
			RDA_HAS_THUMBNAIL	= (1U << 0),

			// RomData subclass may have "dangerous" permissions.
			RDA_HAS_DPOVERLAY	= (1U << 1),

			// RomData subclass has metadata.
			RDA_HAS_METADATA	= (1U << 2),

			// RomData subclass may be present on devices.
			// If not set, this subclass will be skipped if
			// checking a device node.
			RDA_SUPPORTS_DEVICES	= (1U << 3),

			// Check for game-specific disc file systems.
			// (For internal RomDataFactory use only.)
			RDA_CHECK_ISO		= (1U << 8),
		};

		/**
		 * Create a RomData subclass for the specified ROM file.
		 *
		 * NOTE: RomData::isValid() is checked before returning a
		 * created RomData instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the RomData subclass in order to
		 * be returned.
		 *
		 * @param file ROM file
		 * @param attrs RomDataAttr bitfield. If set, RomData subclass must have the specified attributes.
		 * @return RomData subclass, or nullptr if the ROM isn't supported.
		 */
		RP_LIBROMDATA_PUBLIC
		static LibRpBase::RomData *create(LibRpFile::IRpFile *file, unsigned int attrs = 0);

		/**
		 * Create a RomData subclass for the specified ROM file.
		 *
		 * This version creates a base RpFile for the RomData object.
		 * It does not support extended virtual filesystems like GVfs
		 * or KIO, but it does support directories.
		 *
		 * NOTE: RomData::isValid() is checked before returning a
		 * created RomData instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the RomData subclass in order to
		 * be returned.
		 *
		 * @param filename ROM filename (UTF-8)
		 * @param attrs RomDataAttr bitfield. If set, RomData subclass must have the specified attributes.
		 * @return RomData subclass, or nullptr if the ROM isn't supported.
		 */
		RP_LIBROMDATA_PUBLIC
		static LibRpBase::RomData *create(const char *filename, unsigned int attrs = 0);

#ifdef _WIN32
		/**
		 * Create a RomData subclass for the specified ROM file.
		 *
		 * This version creates a base RpFile for the RomData object.
		 * It does not support extended virtual filesystems like GVfs
		 * or KIO, but it does support directories.
		 *
		 * NOTE: RomData::isValid() is checked before returning a
		 * created RomData instance, so returned objects can be
		 * assumed to be valid as long as they aren't nullptr.
		 *
		 * If imgbf is non-zero, at least one of the specified image
		 * types must be supported by the RomData subclass in order to
		 * be returned.
		 *
		 * @param filenameW ROM filename (UTF-16)
		 * @param attrs RomDataAttr bitfield. If set, RomData subclass must have the specified attributes.
		 * @return RomData subclass, or nullptr if the ROM isn't supported.
		 */
		RP_LIBROMDATA_PUBLIC
		static LibRpBase::RomData *create(const wchar_t *filenameW, unsigned int attrs = 0);
#endif /* _WIN32 */

		struct ExtInfo {
			const char *ext;
			unsigned int attrs;

			ExtInfo(const char *ext, unsigned int attrs)
				: ext(ext)
				, attrs(attrs)
				{ }
		};

		/**
		 * Get all supported file extensions.
		 * Used for Win32 COM registration.
		 *
		 * NOTE: The return value is a struct that includes a flag
		 * indicating if the file type handler supports thumbnails
		 * and/or may have "dangerous" permissions.
		 *
		 * @return All supported file extensions, including the leading dot.
		 */
		RP_LIBROMDATA_PUBLIC	/* TODO: Export on Windows only? */
		static const std::vector<ExtInfo> &supportedFileExtensions(void);

		/**
		 * Get all supported MIME types.
		 * Used for KFileMetaData.
		 *
		 * @return All supported MIME types.
		 */
		RP_LIBROMDATA_PUBLIC
		static const std::vector<const char*> &supportedMimeTypes(void);
};

}

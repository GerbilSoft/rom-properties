/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.hpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__

#include "TextFuncs.hpp"
#include "RomFields.hpp"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <string>
#include <vector>

namespace LibRomData {

/**
 * ROM detection information.
 * Used for the class-specific isRomSupported() functions.
 * TODO: Make isRomSupported() non-static with a static wrapper?
 */
struct DetectInfo {
	// ROM header.
	const uint8_t *pHeader;	// ROM header.
	size_t szHeader;	// Size of header.

	// File information.
	const rp_char *ext;	// File extension, including leading '.'
	int64_t szFile;		// File size. (Required for certain types.)
};

class rp_image;
class RomData
{
	protected:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * ROM data base class.
		 *
		 * A ROM file must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the ROM.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * In addition, subclasses must pass an array of RomFielDesc structs.
		 *
		 * @param file ROM file.
		 * @param fields Array of ROM Field descriptions.
		 * @param count Number of ROM Field descriptions.
		 */
		RomData(FILE *file, const RomFields::Desc *fields, int count);
	public:
		virtual ~RomData();

	private:
		RomData(const RomData &);
		RomData &operator=(const RomData &);

	public:
		/**
		 * Is this ROM valid?
		 * @return True if it is; false if it isn't.
		 */
		bool isValid(void) const;

		/**
		 * Close the opened file.
		 */
		void close(void);

	public:
		/** Class-specific functions that can be used even if isValid() is false. **/

		/**
		 * Get a list of all supported file extensions.
		 * This is to be used for file type registration;
		 * subclasses don't explicitly check the extension.
		 *
		 * NOTE: The extensions include the leading dot,
		 * e.g. ".bin" instead of "bin".
		 *
		 * NOTE 2: The strings in the std::vector should *not*
		 * be freed by the caller.
		 *
		 * @return List of all supported file extensions.
		 */
		virtual std::vector<const rp_char*> supportedFileExtensions(void) const = 0;

		/**
		 * Image types supported by a RomData subclass.
		 */
		enum ImageType {
			// Internal images are contained with the ROM or disc image.
			IMG_INT_ICON = 0,	// Internal icon, e.g. DS launcher icon
			IMG_INT_BANNER,		// Internal banner, e.g. GameCube discs
			IMG_INT_MEDIA,		// Internal media scan, e.g. Dreamcast discs

			// External images are downloaded from websites.
			// TODO
			IMG_EXT_MEDIA,		// External media scan, e.g. GameTDB
			IMG_EXT_BOX,		// External box scan

			// Ranges.
			IMG_INT_MIN = IMG_INT_ICON,
			IMG_INT_MAX = IMG_INT_MEDIA,
			IMG_EXT_MIN = IMG_EXT_MEDIA,
			IMG_EXT_MAX = IMG_EXT_BOX
		};

		/**
		 * Image type bitfield.
		 * Used in cases where multiple image types are supported.
		 */
		enum ImageTypeBF {
			// Internal images are contained with the ROM or disc image.
			IMGBF_INT_ICON   = (1 << IMG_INT_ICON),		// Internal icon, e.g. DS launcher icon
			IMGBF_INT_BANNER = (1 << IMG_INT_BANNER),	// Internal banner, e.g. GameCube discs
			IMGBF_INT_MEDIA  = (1 << IMG_INT_MEDIA),	// Internal media scan, e.g. Dreamcast discs

			// External images are downloaded from websites.
			// TODO
			IMGBF_EXT_MEDIA  = (1 << IMG_EXT_MEDIA),	// External media scan, e.g. GameTDB
			IMGBF_EXT_BOX    = (1 << IMG_EXT_BOX),		// External box scan
		};

		/**
		 * Get a bitfield of image types this class can retrieve.
		 * @return Bitfield of supported image types. (ImageTypesBF)
		 */
		virtual uint32_t supportedImageTypes(void) const;

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) = 0;

		/**
		 * Load an internal image.
		 * Called by RomData::image() if the image data hasn't been loaded yet.
		 * @param imageType Image type to load.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadInternalImage(ImageType imageType);

		/**
		 * Load URLs for an external media type.
		 * Called by RomData::extURL() if the URLs haven't been loaded yet.
		 * @param imageType Image type to load.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadURLs(ImageType imageType);

	public:
		/**
		 * Get the ROM Fields object.
		 * @return ROM Fields object.
		 */
		const RomFields *fields(void) const;

		/**
		 * Get an internal image from the ROM.
		 *
		 * NOTE: The rp_image is owned by this object.
		 * Do NOT delete this object until you're done using this rp_image.
		 *
		 * @param imageType Image type to load.
		 * @return Internal image, or nullptr if the ROM doesn't have one.
		 */
		const rp_image *image(ImageType imageType) const;

		/**
		 * Get a list of URLs for an external media type.
		 *
		 * NOTE: The std::vector<rp_string> is owned by this object.
		 * Do NOT delete this object until you're done using this rp_image.
		 *
		 * @param imageType Image type.
		 * @return List of URLs, or nullptr if the ROM doesn't have one.
		 */
		const std::vector<rp_string> *extURLs(ImageType imageType) const;

		/**
		 * Get a cache key for an external media type.
		 * @return Cache key, or nullptr if not cacheable.
		 */
		const rp_char *cacheKey(ImageType imageType) const;

	protected:
		// TODO: Make a private class?
		bool m_isValid;			// Subclass must set this to true if the ROM is valid.
		FILE *m_file;			// Open file.
		RomFields *const m_fields;	// ROM fields.

		// Internal images.
		rp_image *m_images[IMG_INT_MAX - IMG_INT_MIN + 1];

		// Lists of URLs for external media types.
		// Each vector contains a list of URLs for the given
		// media type, in priority order. ([0] == highest priority)
		// This is done to allow for multiple quality levels.
		// TODO: Allow the user to customize quality levels?
		std::vector<rp_string> m_extURLs[IMG_EXT_MAX - IMG_EXT_MIN + 1];

		// List of cache keys for external media types.
		rp_string m_cacheKey[IMG_EXT_MAX - IMG_EXT_MIN + 1];
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__ */

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

// C++ includes.
#include <string>
#include <vector>

namespace LibRomData {

class IRpFile;
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
		 * In addition, subclasses must pass an array of RomFieldDesc structs.
		 *
		 * @param file ROM file.
		 * @param fields Array of ROM Field descriptions.
		 * @param count Number of ROM Field descriptions.
		 */
		RomData(IRpFile *file, const RomFields::Desc *fields, int count);
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
		/** ROM detection functions. **/

		/**
		 * ROM detection information.
		 * Used for isRomSupported() functions.
		 */
		struct DetectInfo {
			// ROM header.
			const uint8_t *pHeader;	// ROM header.
			size_t szHeader;	// Size of header.

			// File information.
			const rp_char *ext;	// File extension, including leading '.'
			int64_t szFile;		// File size. (Required for certain types.)
		};

		/**
		 * Is a ROM image supported by this object?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Object-specific system ID (>= 0) if supported; -1 if not.
		 */
		virtual int isRomSupported(const DetectInfo *info) const = 0;

		/**
		 * Get the name of the system the loaded ROM is designed for.
		 * TODO: Parameter for long or short name, or region?
		 * @return System name, or nullptr if not supported.
		 */
		virtual const rp_char *systemName(void) const = 0;

		// TODO:
		// - List of supported systems.
		// - Get logo from current system and/or other system?

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
			//IMG_INT_ICON_SMALL,	// Internal small icon. (3DS) [TODO]
			IMG_INT_BANNER,		// Internal banner, e.g. GameCube discs
			IMG_INT_MEDIA,		// Internal media scan, e.g. Dreamcast discs

			// External images are downloaded from websites,
			// such as GameTDB.
			IMG_EXT_MEDIA,		// External media scan
			IMG_EXT_BOX,		// External box scan
			IMG_EXT_BOX_FULL,	// External box scan (both sides)
			IMG_EXT_BOX_3D,		// External box scan (3D version)

			// Ranges.
			IMG_INT_MIN = IMG_INT_ICON,
			IMG_INT_MAX = IMG_INT_MEDIA,
			IMG_EXT_MIN = IMG_EXT_MEDIA,
			IMG_EXT_MAX = IMG_EXT_BOX_3D
		};

		/**
		 * Image type bitfield.
		 * Used in cases where multiple image types are supported.
		 */
		enum ImageTypeBF {
			// Internal images are contained with the ROM or disc image.
			IMGBF_INT_ICON   = (1 << IMG_INT_ICON),		// Internal icon, e.g. DS launcher icon
			//IMGBF_INT_ICON_SMALL = (1 << IMG_INT_ICON_SMALL),	// Internal small icon. (3DS) [TODO]
			IMGBF_INT_BANNER = (1 << IMG_INT_BANNER),	// Internal banner, e.g. GameCube discs
			IMGBF_INT_MEDIA  = (1 << IMG_INT_MEDIA),	// Internal media scan, e.g. Dreamcast discs

			// External images are downloaded from websites,
			// such as GameTDB.
			IMGBF_EXT_MEDIA    = (1 << IMG_EXT_MEDIA),	// External media scan, e.g. GameTDB
			IMGBF_EXT_BOX      = (1 << IMG_EXT_BOX),	// External box scan
			IMGBF_EXT_BOX_FULL = (1 << IMG_EXT_BOX_FULL),	// External box scan (both sides)
			IMGBF_EXT_BOX_3D   = (1 << IMG_EXT_BOX_3D),	// External box scan (3D version)
		};

		/**
		 * Image processing flags.
		 */
		enum ImageProcessingBF {
			IMGPF_CDROM_120MM	= (1 << 0),	// Apply a 120mm CD-ROM transparency mask.
			IMGPF_CDROM_80MM	= (1 << 1),	// Apply an 80mm CD-ROM transparency mask.
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

	private:
		/**
		 * Verify that the specified image type has been loaded.
		 * @param imageType Image type.
		 * @return 0 if loaded; negative POSIX error code on error.
		 */
		int verifyImageTypeLoaded(ImageType imageType) const;

	public:
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
		 * External URLs for a media type.
		 * Includes URL and "cache key" for local caching.
		 */
		struct ExtURL {
			rp_string url;		// URL
			rp_string cache_key;	// Cache key
		};

		/**
		 * Get a list of URLs for an external media type.
		 *
		 * NOTE: The std::vector<extURL> is owned by this object.
		 * Do NOT delete this object until you're done using this rp_image.
		 *
		 * @param imageType Image type.
		 * @return List of URLs and cache keys, or nullptr if the ROM doesn't have one.
		 */
		const std::vector<ExtURL> *extURLs(ImageType imageType) const;

		/**
		 * Get image processing flags.
		 *
		 * These specify post-processing operations for images,
		 * e.g. applying transparency masks.
		 *
		 * @param imageType Image type.
		 * @return Bitfield of ImageProcessingBF operations to perform.
		 */
		uint32_t imgpf(ImageType imageType) const;

	protected:
		// TODO: Make a private class?
		bool m_isValid;			// Subclass must set this to true if the ROM is valid.
		IRpFile *m_file;		// Open file.
		RomFields *const m_fields;	// ROM fields.

		// Internal images.
		rp_image *m_images[IMG_INT_MAX - IMG_INT_MIN + 1];

		// Lists of URLs and cache keys for external media types.
		// Each vector contains a list of URLs for the given
		// media type, in priority order. ([0] == highest priority)
		// This is done to allow for multiple quality levels.
		// TODO: Allow the user to customize quality levels?
		std::vector<ExtURL> m_extURLs[IMG_EXT_MAX - IMG_EXT_MIN + 1];

		// Image processing flags.
		uint32_t m_imgpf[IMG_EXT_MAX];
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__ */

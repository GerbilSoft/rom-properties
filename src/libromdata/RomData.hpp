/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomData.hpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
#include <cstring>

// C++ includes.
#include <string>
#include <vector>

namespace LibRomData {

class IRpFile;
class rp_image;
struct IconAnimData;

class RomDataPrivate;
class RomData
{
	protected:
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
		RomData(IRpFile* file, const RomFields::Desc* fields, int count);

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
		 * In addition, subclasses must pass an array of RomFieldDesc structs
		 * using an allocated RomDataPrivate subclass.
		 *
		 * @param d RomDataPrivate subclass.
		 */
		explicit RomData(RomDataPrivate *d);

	protected:
		/**
		 * RomData destructor is protected.
		 * Use unref() instead.
		 */
		virtual ~RomData();

	private:
		RomData(const RomData &);
		RomData &operator=(const RomData &);
	protected:
		friend class RomDataPrivate;
		RomDataPrivate *const d_ptr;

	public:
		/**
		 * Take a reference to this RomData* object.
		 * @return this
		 */
		RomData *ref(void);

		/**
		 * Unreference this RomData* object.
		 * If ref_count reaches 0, the RomData* object is deleted.
		 */
		void unref(void);

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
		 * Header information.
		 */
		struct HeaderInfo {
			uint32_t addr;		// Start address in the ROM.
			uint32_t size;		// Length.
			const uint8_t *pData;	// Data.
		};

		/**
		 * ROM detection information.
		 * Used for isRomSupported() functions.
		 */
		struct DetectInfo {
			HeaderInfo header;	// ROM header.
			const rp_char *ext;	// File extension, including leading '.'
			int64_t szFile;		// File size. (Required for certain types.)
		};

		/**
		 * Is a ROM image supported by this object?
		 * @param info DetectInfo containing ROM detection information.
		 * @return Object-specific system ID (>= 0) if supported; -1 if not.
		 */
		virtual int isRomSupported(const DetectInfo *info) const = 0;

		enum SystemNameType {
			/**
			 * The SystemNameType enum is a bitfield.
			 *
			 * Type:
			 * - Long: Full company and system name.
			 * - Short: System name only.
			 * - Abbreviation: System initials.
			 *
			 * Region:
			 * - Generic: Most well-known name for the system.
			 * - ROM Local: Localized version based on the ROM region.
			 *   If a ROM is multi-region, the name is selected based
			 *   on the current system locale.
			 */

			SYSNAME_TYPE_LONG		= (0 << 0),
			SYSNAME_TYPE_SHORT		= (1 << 0),
			SYSNAME_TYPE_ABBREVIATION	= (2 << 0),
			SYSNAME_TYPE_MASK		= (3 << 0),

			SYSNAME_REGION_GENERIC		= (0 << 2),
			SYSNAME_REGION_ROM_LOCAL	= (1 << 2),
			SYSNAME_REGION_MASK		= (1 << 2),
		};

	protected:
		/**
		 * Is a SystemNameType bitfield value valid?
		 * @param type Bitfield containing SystemNameType values.
		 * @return True if valid; false if not.
		 */
		static inline bool isSystemNameTypeValid(uint32_t type)
		{
			// Check for an invalid SYSNAME_TYPE.
			if ((type & SYSNAME_TYPE_MASK) > SYSNAME_TYPE_ABBREVIATION)
				return false;
			// Check for any unsupported bits.
			if ((type & ~(SYSNAME_REGION_MASK | SYSNAME_TYPE_MASK)) != 0)
				return false;
			// Type is valid.
			return true;
		}

	public:
		/**
		 * Get the name of the system the loaded ROM is designed for.
		 * @param type System name type. (See the SystemNameType enum.)
		 * @return System name, or nullptr if type is invalid.
		 */
		virtual const rp_char *systemName(uint32_t type) const = 0;

		enum FileType {
			FTYPE_UNKNOWN = 0,

			// ROM image.
			FTYPE_ROM_IMAGE,

			// Optical disc image.
			FTYPE_DISC_IMAGE,

			// Save file.
			FTYPE_SAVE_FILE,

			// "Embedded" disc image.
			// Commonly seen on GameCube demo discs.
			FTYPE_EMBEDDED_DISC_IMAGE,

			// Application package, e.g. WAD, CIA.
			FTYPE_APPLICATION_PACKAGE,

			// NFC dump, e.g. amiibo.
			FTYPE_NFC_DUMP,

			// Floppy and/or hard disk image.
			FTYPE_DISK_IMAGE,

			// End of FileType.
			FTYPE_LAST
		};

		/**
		 * Get the general file type.
		 * @return General file type.
		 */
		FileType fileType(void) const;

		/**
		 * Get the general file type as a string.
		 * @return General file type as a string, or nullptr if unknown.
		 */
		const rp_char *fileType_string(void) const;

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

			// If the image needs to be resized, use
			// nearest neighbor if the new size is an
			// integer multiple of the old size.
			IMGPF_RESCALE_NEAREST	= (1 << 2),

			// File supports animated icons.
			// Call iconAnimData() to get the animated
			// icon frames and control information.
			IMGPF_ICON_ANIMATED	= (1 << 3),
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
		 * Scrape an image URL from a downloaded HTML page.
		 * Needed if IMGPF_EXTURL_NEEDS_HTML_SCRAPING is set.
		 * @param html HTML data.
		 * @param size Size of HTML data.
		 * @return Image URL, or empty string if not found or not supported.
		 */
		virtual rp_string scrapeImageURL(const char *html, size_t size) const;

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

		/**
		 * Get name of an image type
		 * @param imageType Image type.
		 * @return String containing user-friendly name of an image type.
		 */
		static const rp_char *getImageTypeName(ImageType imageType);

		/**
		 * Get the animated icon data.
		 *
		 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
		 * object has an animated icon.
		 *
		 * @return Animated icon data, or nullptr if no animated icon is present.
		 */
		virtual const IconAnimData *iconAnimData(void) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ROMDATA_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData.hpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__

#include "librpbase/common.h"

#include "RomData_decl.hpp"

// C includes.
#include <stdint.h>
#include <stddef.h>	/* size_t */

// C++ includes.
#include <string>
#include <vector>

namespace LibRpBase {

class IRpFile;
class RomFields;
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
		 * @param file ROM file.
		 */
		explicit RomData(IRpFile *file);

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
		RP_DISABLE_COPY(RomData)
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
		 * If ref_cnt reaches 0, the RomData* object is deleted.
		 */
		void unref(void);

	public:
		/**
		 * Is this ROM valid?
		 * @return True if it is; false if it isn't.
		 */
		bool isValid(void) const;

		/**
		 * Is the file open?
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const;

		/**
		 * Close the opened file.
		 */
		virtual void close(void);

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
			const char *ext;	// File extension, including leading '.'
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
		static inline bool isSystemNameTypeValid(unsigned int type)
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
		virtual const char *systemName(unsigned int type) const = 0;

		/**
		 * Get the class name for the user configuration.
		 * @return Class name. (ASCII) (nullptr on error)
		 */
		const char *className(void) const;

		enum FileType {
			FTYPE_UNKNOWN = 0,

			FTYPE_ROM_IMAGE,		// ROM image
			FTYPE_DISC_IMAGE,		// Optical disc image
			FTYPE_SAVE_FILE,		// Save file
			FTYPE_EMBEDDED_DISC_IMAGE,	// "Embedded" disc image, e.g. GameCube TGC
			FTYPE_APPLICATION_PACKAGE,	// Application package, e.g. WAD, CIA
			FTYPE_NFC_DUMP,			// NFC dump, e.g. amiibo
			FTYPE_DISK_IMAGE,		// Floppy and/or hard disk image
			FTYPE_EXECUTABLE,		// Executable, e.g. EXE
			FTYPE_DLL,			// Dynamic Link Library
			FTYPE_DEVICE_DRIVER,		// Device driver
			FTYPE_RESOURCE_LIBRARY,		// Resource library
			FTYPE_ICON_FILE,		// Icon file, e.g. SMDH.
			FTYPE_BANNER_FILE,		// Banner file, e.g. GameCube opening.bnr.
			FTYPE_HOMEBREW,			// Homebrew application, e.g. 3DSX.
			FTYPE_EMMC_DUMP,		// eMMC dump
			FTYPE_TITLE_CONTENTS,		// Title contents, e.g. NCCH.
			FTYPE_FIRMWARE_BINARY,		// Firmware binary, e.g. 3DS FIRM.
			FTYPE_TEXTURE_FILE,		// Texture file, e.g. Sega PVR.
			FTYPE_RELOCATABLE_OBJECT,	// Relocatable Object File (*.o)
			FTYPE_SHARED_LIBRARY,		// Shared Library (similar to DLLs)
			FTYPE_CORE_DUMP,		// Core Dump

			FTYPE_LAST			// End of FileType.
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
		const char *fileType_string(void) const;

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
		 * NOTE 2: The array and the strings in the array should
		 * *not* be freed by the caller.
		 *
		 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
		 */
		virtual const char *const *supportedFileExtensions(void) const = 0;

		/**
		 * Image types supported by a RomData subclass.
		 */
		enum ImageType {
			// Internal images are contained with the ROM or disc image.
			IMG_INT_ICON = 0,	// Internal icon, e.g. DS launcher icon
			IMG_INT_BANNER,		// Internal banner, e.g. GameCube discs
			IMG_INT_MEDIA,		// Internal media scan, e.g. Dreamcast discs
			IMG_INT_IMAGE,		// Internal image, e.g. PVR images.

			// External images are downloaded from websites,
			// such as GameTDB.
			IMG_EXT_MEDIA,		// External media scan
			IMG_EXT_COVER,		// External cover scan
			IMG_EXT_COVER_3D,	// External cover scan (3D version)
			IMG_EXT_COVER_FULL,	// External cover scan (front and back)
			IMG_EXT_BOX,		// External box scan (cover + outer box)

			// Ranges.
			IMG_INT_MIN = IMG_INT_ICON,
			IMG_INT_MAX = IMG_INT_IMAGE,
			IMG_EXT_MIN = IMG_EXT_MEDIA,
			IMG_EXT_MAX = IMG_EXT_BOX,

			// Special value for the user configuration.
			// If specified, all thumbnails will be disabled.
			// (Manual image extraction in rpcli is not affected.)
			IMG_DISABLED = 255,
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
			IMGBF_INT_IMAGE  = (1 << IMG_INT_IMAGE),	// Internal image, e.g. PVR images.

			// External images are downloaded from websites,
			// such as GameTDB.
			IMGBF_EXT_MEDIA      = (1 << IMG_EXT_MEDIA),      // External media scan, e.g. GameTDB
			IMGBF_EXT_COVER      = (1 << IMG_EXT_COVER),      // External cover scan
			IMGBF_EXT_COVER_3D   = (1 << IMG_EXT_COVER_3D),   // External cover scan (3D version)
			IMGBF_EXT_COVER_FULL = (1 << IMG_EXT_COVER_FULL), // External cover scan (front and back)
			IMGBF_EXT_BOX	     = (1 << IMG_EXT_BOX),        // External box scan (cover + outer box)
		};

		/**
		 * Get a bitfield of image types this class can retrieve.
		 * @return Bitfield of supported image types. (ImageTypesBF)
		 */
		virtual uint32_t supportedImageTypes(void) const;

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

		struct ImageSizeDef {
			const char *name;	// Size name, if applicable. [UTF-8] (May be nullptr.)
			uint16_t width;		// Image width. (May be 0 if unknown.)
			uint16_t height;	// Image height. (May be 0 if unknown.)

			// System-dependent values.
			// Don't use these outside of a RomData subclass.
			uint16_t index;		// Image index.
		};

		/**
		 * Get a list of all available image sizes for the specified image type.
		 *
		 * The first item in the returned vector is the "default" size.
		 * If the width/height is 0, then an image exists, but the size is unknown.
		 *
		 * @param imageType Image type.
		 * @return Vector of available image sizes, or empty vector if no images are available.
		 */
		virtual std::vector<ImageSizeDef> supportedImageSizes(ImageType imageType) const;

		/**
		 * Get image processing flags.
		 *
		 * These specify post-processing operations for images,
		 * e.g. applying transparency masks.
		 *
		 * @param imageType Image type.
		 * @return Bitfield of ImageProcessingBF operations to perform.
		 */
		virtual uint32_t imgpf(ImageType imageType) const;

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) = 0;

		/**
		 * Load an internal image.
		 * Called by RomData::image().
		 * @param imageType	[in] Image type to load.
		 * @param pImage	[out] Pointer to const rp_image* to store the image in.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadInternalImage(ImageType imageType, const rp_image **pImage);

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
		 * Includes URL and "cache key" for local caching,
		 * plus the expected image size (if available).
		 */
		struct ExtURL {
			std::string url;	// URL
			std::string cache_key;	// Cache key
			uint16_t width;		// Expected image width. (0 for unknown)
			uint16_t height;	// Expected image height. (0 for unknown)

			// Set this to true if this is a "high-resolution" image.
			// This will be used to determine if it should be
			// downloaded if high-resolution downloads are
			// disabled.
			//
			// NOTE: If only one image size is available, set this
			// to 'false' to allow it to be downloaded. Otherwise,
			// no images will be downloaded.
			bool high_res;
		};

		/**
		 * Special image size values.
		 */
		enum ImageSizeType {
			IMAGE_SIZE_DEFAULT	=  0,
			IMAGE_SIZE_SMALLEST	= -1,
			IMAGE_SIZE_LARGEST	= -2,
			// TODO: IMAGE_SIZE_ALL to get all images.
			// IMAGE_SIZE_ALL	= -3,

			// Minimum allowed value.
			IMAGE_SIZE_MIN_VALUE	= IMAGE_SIZE_LARGEST
		};

		/**
		 * Get a list of URLs for an external image type.
		 *
		 * A thumbnail size may be requested from the shell.
		 * If the subclass supports multiple sizes, it should
		 * try to get the size that most closely matches the
		 * requested size.
		 *
		 * @param imageType	[in]     Image type.
		 * @param pExtURLs	[out]    Output vector.
		 * @param size		[in,opt] Requested image size. This may be a requested
		 *                               thumbnail size in pixels, or an ImageSizeType
		 *                               enum value.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int extURLs(ImageType imageType, std::vector<ExtURL> *pExtURLs, int size = IMAGE_SIZE_DEFAULT) const;

		/**
		 * Scrape an image URL from a downloaded HTML page.
		 * Needed if IMGPF_EXTURL_NEEDS_HTML_SCRAPING is set.
		 * @param html HTML data.
		 * @param size Size of HTML data.
		 * @return Image URL, or empty string if not found or not supported.
		 */
		virtual std::string scrapeImageURL(const char *html, size_t size) const;

		/**
		 * Get name of an image type
		 * @param imageType Image type.
		 * @return String containing user-friendly name of an image type.
		 */
		static const char *getImageTypeName(ImageType imageType);

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

#endif /* __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData.hpp: ROM data base class.                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once
#define __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC
#include "RomData_decl.hpp"

// C includes
#include <stddef.h>	/* size_t */
#include <stdint.h>

// C++ includes
#include <memory>
#include <string>
#include <vector>

// Other rom-properties libraries
#include "img/IconAnimData.hpp"
#include "librpfile/IRpFile.hpp"
#include "librptexture/img/rp_image.hpp"

namespace LibRpBase {

class RomFields;
class RomMetaData;

class RomDataPrivate;
class NOVTABLE RomData
{
protected:
	/**
	 * ROM data base class.
	 *
	 * A ROM file must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param d RomDataPrivate subclass.
	 */
	explicit RomData(RomDataPrivate *d);

public:
	virtual ~RomData();

private:
	RP_DISABLE_COPY(RomData)
protected:
	friend class RomDataPrivate;
	RomDataPrivate *const d_ptr;

public:
	/**
	 * Is this ROM valid?
	 * @return True if it is; false if it isn't.
	 */
	RP_LIBROMDATA_PUBLIC
	bool isValid(void) const;

	/**
	 * Is the file open?
	 * @return True if the file is open; false if it isn't.
	 */
	RP_LIBROMDATA_PUBLIC
	bool isOpen(void) const;

	/**
	 * Close the opened file.
	 */
	virtual void close(void);

	/**
	 * Get a reference to the internal file.
	 * @return Reference to file, or nullptr on error.
	 */
	LibRpFile::IRpFilePtr ref_file(void) const;

	/**
	 * Get the filename that was loaded.
	 * @return Filename, or nullptr on error.
	 */
	RP_LIBROMDATA_PUBLIC
	const char *filename(void) const;

	/**
	 * Is the file compressed? (transparent decompression)
	 * If it is, then ROM operations won't be allowed.
	 * @return True if compressed; false if not.
	 */
	bool isCompressed(void) const;

public:
	/** ROM detection functions. **/

	/**
	 * ROM detection information.
	 * Used for isRomSupported() functions.
	 */
	struct DetectInfo {
		struct {
			uint32_t addr;		// Start address in the ROM.
			uint32_t size;		// Length.
			const uint8_t *pData;	// Data.
		} header;		// ROM header.
		const char *ext;	// File extension, including leading '.'
		off64_t szFile;		// File size. (Required for certain types.)
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

		SYSNAME_TYPE_LONG		= (0U << 0),
		SYSNAME_TYPE_SHORT		= (1U << 0),
		SYSNAME_TYPE_ABBREVIATION	= (2U << 0),
		SYSNAME_TYPE_MASK		= (3U << 0),

		SYSNAME_REGION_GENERIC		= (0U << 2),
		SYSNAME_REGION_ROM_LOCAL	= (1U << 2),
		SYSNAME_REGION_MASK		= (1U << 2),
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
	RP_LIBROMDATA_PUBLIC
	const char *className(void) const;

	enum class FileType {
		Unknown = 0,

		ROM_Image,		// ROM image
		DiscImage,		// Optical disc image
		SaveFile,		// Save file
		EmbeddedDiscImage,	// "Embedded" disc image, e.g. GameCube TGC
		ApplicationPackage,	// Application package, e.g. WAD, CIA
		NFC_Dump,		// NFC dump, e.g. amiibo
		DiskImage,		// Floppy and/or hard disk image
		Executable,		// Executable, e.g. EXE
		DLL,			// Dynamic Link Library
		DeviceDriver,		// Device driver
		ResourceLibrary,	// Resource library
		IconFile,		// Icon file, e.g. SMDH.
		BannerFile,		// Banner file, e.g. GameCube opening.bnr.
		Homebrew,		// Homebrew application, e.g. 3DSX.
		eMMC_Dump,		// eMMC dump
		ContainerFile,		// Container file, e.g. NCCH.
		FirmwareBinary,		// Firmware binary, e.g. 3DS FIRM.
		TextureFile,		// Texture file, e.g. Sega PVR.
		RelocatableObject,	// Relocatable Object File (*.o)
		SharedLibrary,		// Shared Library (similar to DLLs)
		CoreDump,		// Core Dump
		AudioFile,		// Audio file
		BootSector,		// Boot sector
		Bundle,			// Bundle (Mac OS X)
		ResourceFile,		// Resource file
		Partition,		// Partition
		MetadataFile,		// Metadata File
		PatchFile,		// Patch File

		Max
	};

	/**
	 * Get the general file type.
	 * @return General file type.
	 */
	RP_LIBROMDATA_PUBLIC
	FileType fileType(void) const;

	/**
	 * FileType to string conversion function.
	 * @param fileType File type
	 * @return FileType as a string, or nullptr if unknown.
	 */
	RP_LIBROMDATA_PUBLIC
	static const char *fileType_to_string(FileType fileType);

	/**
	 * Get the general file type as a string.
	 * @return General file type as a string, or nullptr if unknown.
	 */
	RP_LIBROMDATA_PUBLIC
	const char *fileType_string(void) const;

	/**
	 * Get the file's MIME type.
	 * @return MIME type, or nullptr if unknown.
	 */
	RP_LIBROMDATA_PUBLIC
	const char *mimeType(void) const;

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
	const char *const *supportedFileExtensions(void) const;

	/**
	 * Get a list of all supported MIME types.
	 * This is to be used for metadata extractors that
	 * must indicate which MIME types they support.
	 *
	 * NOTE: The array and the strings in the array should
	 * *not* be freed by the caller.
	 *
	 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
	 */
	const char *const *supportedMimeTypes(void) const;

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
		IMG_EXT_TITLE_SCREEN,	// External title screen

		// Ranges.
		IMG_INT_MIN = IMG_INT_ICON,
		IMG_INT_MAX = IMG_INT_IMAGE,
		IMG_EXT_MIN = IMG_EXT_MEDIA,
		IMG_EXT_MAX = IMG_EXT_TITLE_SCREEN,

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
		IMGBF_INT_ICON   = (1U << IMG_INT_ICON),	// Internal icon, e.g. DS launcher icon
		IMGBF_INT_BANNER = (1U << IMG_INT_BANNER),	// Internal banner, e.g. GameCube discs
		IMGBF_INT_MEDIA  = (1U << IMG_INT_MEDIA),	// Internal media scan, e.g. Dreamcast discs
		IMGBF_INT_IMAGE  = (1U << IMG_INT_IMAGE),	// Internal image, e.g. PVR images.

		// External images are downloaded from websites,
		// such as GameTDB.
		IMGBF_EXT_MEDIA      = (1U << IMG_EXT_MEDIA),      // External media scan, e.g. GameTDB
		IMGBF_EXT_COVER      = (1U << IMG_EXT_COVER),      // External cover scan
		IMGBF_EXT_COVER_3D   = (1U << IMG_EXT_COVER_3D),   // External cover scan (3D version)
		IMGBF_EXT_COVER_FULL = (1U << IMG_EXT_COVER_FULL), // External cover scan (front and back)
		IMGBF_EXT_BOX	     = (1U << IMG_EXT_BOX),        // External box scan (cover + outer box)
		IMGBF_EXT_TITLE_SCREEN = (1U << IMG_EXT_TITLE_SCREEN), // External title screen
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
		IMGPF_CDROM_120MM	= (1U << 0),	// Apply a 120mm CD-ROM transparency mask.
		IMGPF_CDROM_80MM	= (1U << 1),	// Apply an 80mm CD-ROM transparency mask.

		// If the image needs to be resized, use
		// nearest neighbor if the new size is an
		// integer multiple of the old size.
		IMGPF_RESCALE_NEAREST	= (1U << 2),

		// File supports animated icons.
		// Call iconAnimData() to get the animated
		// icon frames and control information.
		IMGPF_ICON_ANIMATED	= (1U << 3),

		// Image should be rescaled to an 8:7 aspect ratio.
		// This is for Super NES, and only applies to images
		// with 256px and 512px widths.
		IMGPF_RESCALE_ASPECT_8to7	= (1U << 4),

		// Image should be rescaled to the size specified by
		// the second RFT_DIMENSIONS field. (for e.g. ETC2
		// textures that have a power-of-2 size but should
		// be rendered with a smaller size)
		// FIXME: Better way to get the rescale size.
		IMGPF_RESCALE_RFT_DIMENSIONS_2	= (1U << 5),
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
	 * Load metadata properties.
	 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
	 * @return Number of metadata properties read on success; negative POSIX error code on error.
	 */
	virtual int loadMetaData(void);

public:
	// NOTE: This function needs to be public because it might be
	// called by RomData subclasses that own other RomData subclasses.
	/**
	 * Load an internal image.
	 * Called by RomData::image().
	 * @param imageType	[in] Image type to load.
	 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int loadInternalImage(ImageType imageType, LibRpTexture::rp_image_const_ptr &pImage);

public:
	// NOTE: This function needs to be public because it might be
	// called by RomData subclasses that own other RomData subclasses.
	/**
	 * Load an internal mipmap level for IMG_INT_IMAGE.
	 * Called by RomData::mipmap().
	 * @param mipmapLevel	[in] Mipmap level.
	 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int loadInternalMipmap(int mipmapLevel, LibRpTexture::rp_image_const_ptr &pImage);

public:
	/**
	 * Get the ROM Fields object.
	 * @return ROM Fields object.
	 */
	RP_LIBROMDATA_PUBLIC
	const RomFields *fields(void) const;

	/**
	 * Get the ROM Metadata object.
	 * @return ROM Metadata object.
	 */
	RP_LIBROMDATA_PUBLIC
	const RomMetaData *metaData(void) const;

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
	 * @param imageType Image type to load.
	 * @return Internal image, or nullptr if the ROM doesn't have one.
	 */
	RP_LIBROMDATA_PUBLIC
	LibRpTexture::rp_image_const_ptr image(ImageType imageType) const;

	/**
	 * Get an internal image mipmap from the texture.
	 *
	 * This always refers to IMG_INT_IMAGE.
	 *
	 * This is mostly used for texture files to retrieve mipmaps
	 * other than the full image.
	 *
	 * NOTE: For mipmap level 0, this is identical to image(IMG_INT_IMAGE).
	 *
	 * @param mipmapLevel Mipmap level to load.
	 * @return Internal image at the specified mipmap level, or nullptr if the ROM doesn't have one.
	 */
	RP_LIBROMDATA_PUBLIC
	LibRpTexture::rp_image_const_ptr mipmap(int mipmapLevel) const;

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
	 * Get name of an image type
	 * @param imageType Image type.
	 * @return String containing user-friendly name of an image type.
	 */
	RP_LIBROMDATA_PUBLIC
	static const char *getImageTypeName(ImageType imageType);

	/**
	 * Get the animated icon data.
	 *
	 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this
	 * object has an animated icon.
	 *
	 * @return Animated icon data, or nullptr if no animated icon is present.
	 */
	virtual IconAnimDataConstPtr iconAnimData(void) const;

public:
	/**
	 * Does this ROM image have "dangerous" permissions?
	 *
	 * @return True if the ROM image has "dangerous" permissions; false if not.
	 */
	virtual bool hasDangerousPermissions(void) const;

public:
	/**
	 * ROM operation struct.
	 * `const char*` fields are owned by the RomData subclass.
	 */
	struct RomOp {
		const char *desc;	// Description (Use '&' for mnemonics)
		uint32_t flags;		// Flags

		enum RomOpsFlags {
			ROF_ENABLED		= (1U << 0),	// Set to enable the ROM op
			ROF_REQ_WRITABLE	= (1U << 1),	// Requires a writable RomData
			ROF_SAVE_FILE		= (1U << 2),	// Prompt to save a new file
		};

		// Data depends on RomOpsFlags.
		union {
			// ROF_SAVE_FILE
			struct {
				const char *title;	// Dialog title
				const char *filter;	// File filter (RP format) [don't include "All Files"]
				const char *ext;	// New file extension (with leading '.')
			} sfi;
		};

		RomOp()
			: desc(nullptr), flags(0)
		{
			sfi.title = nullptr;
			sfi.filter = nullptr;
			sfi.ext = nullptr;
		}

		RomOp(const char *desc, uint32_t flags)
			: desc(desc), flags(flags)
		{
			sfi.title = nullptr;
			sfi.filter = nullptr;
			sfi.ext = nullptr;
		}
	};

	struct RomOpParams {
		/** OUT: Results **/
		int status;			// Status. (0 == success; negative == POSIX error; positive == other error)
		std::string msg;		// Status message. (optional)
		std::vector<int> fieldIdx;	// Field indexes that were updated.

		/** IN: Parameters **/
		const char *save_filename;	// Filename for saving data.

		RomOpParams()
			: status(0)
			, save_filename(nullptr)
		{}
	};

	/**
	 * Get the list of operations that can be performed on this ROM.
	 * @return List of operations.
	 */
	RP_LIBROMDATA_PUBLIC
	std::vector<RomOp> romOps(void) const;

	/**
	 * Perform a ROM operation.
	 * @param id		[in] Operation index.
	 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	RP_LIBROMDATA_PUBLIC
	int doRomOp(int id, RomOpParams *pParams);

protected:
	/**
	 * Get the list of operations that can be performed on this ROM.
	 * Internal function; called by RomData::romOps().
	 * @return List of operations.
	 */
	virtual std::vector<RomOp> romOps_int(void) const;

	/**
	 * Perform a ROM operation.
	 * Internal function; called by RomData::doRomOp().
	 * @param id		[in] Operation index.
	 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	virtual int doRomOp_int(int id, RomOpParams *pParams);

public:
	/**
	 * Check for "viewed" achievements.
	 * @return Number of achievements unlocked.
	 */
	virtual int checkViewedAchievements(void) const;
};

typedef std::shared_ptr<RomData> RomDataPtr;
typedef std::shared_ptr<const RomData> RomDataConstPtr;

} //namespace LibRpBase

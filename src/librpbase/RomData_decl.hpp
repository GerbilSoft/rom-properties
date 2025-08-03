/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData_decl.hpp: ROM data base class. (Subclass macros)                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__
#  error RomData_decl.hpp should only be included by RomData.hpp
#endif

namespace LibRpBase {
	struct RomDataInfo;
}

// for loadInternalImage() implementation macros
#include <cerrno>

/** Macros for RomData subclasses. **/

/**
 * Initial declaration for a RomData subclass.
 *
 * No constructor is included with this version.
 * ROMDATA_DECL_COMMON_FNS() must be used afterwards.
 *
 * @param klass Class name
 */
#define ROMDATA_DECL_BEGIN_NO_CTOR(klass) \
class klass##Private; \
class klass final : public LibRpBase::RomData { \
private: \
	typedef LibRpBase::RomData super; \
	friend class klass##Private; \
	RP_DISABLE_COPY(klass); \

/**
 * Initial declaration for an exported RomData subclass.
 * NOTE: This should *only* be used for RomDataTestObject.
 *
 * No constructor is included with this version.
 * ROMDATA_DECL_COMMON_FNS() must be used afterwards.
 *
 * @param klass Class name
 */
#define ROMDATA_DECL_BEGIN_NO_CTOR_EXPORT(klass) \
class klass##Private; \
class RP_LIBROMDATA_PUBLIC klass final : public LibRpBase::RomData { \
private: \
	typedef LibRpBase::RomData super; \
	friend class klass##Private; \
	RP_DISABLE_COPY(klass); \

/**
 * Default constructor for a RomData subclass.
 */
#define ROMDATA_DECL_CTOR_DEFAULT(klass) \
public: \
	/**
	 * Read a ROM image. \
	 * \
	 * A ROM image must be opened by the caller. The file handle \
	 * will be ref()'d and must be kept open in order to load \
	 * data from the ROM image. \
	 * \
	 * To close the file, either delete this object or call close(). \
	 * \
	 * NOTE: Check isValid() to determine if this is a valid ROM. \
	 * \
	 * @param file Open ROM image \
	 */ \
	explicit klass(const LibRpFile::IRpFilePtr &file);

/**
 * Common functions for a RomData subclass.
 */
#define ROMDATA_DECL_COMMON_FNS() \
public: \
	/** \
	 * Is a ROM image supported by this class? \
	 * @param info DetectInfo containing ROM detection information. \
	 * @return Class-specific system ID (>= 0) if supported; -1 if not. \
	 */ \
	static int isRomSupported_static(const DetectInfo *info); \
\
	/** \
	 * Is a ROM image supported by this object? \
	 * @param info DetectInfo containing ROM detection information. \
	 * @return Class-specific system ID (>= 0) if supported; -1 if not. \
	 */ \
	int isRomSupported(const DetectInfo *info) const final; \
\
	/** \
	 * Get the name of the system the loaded ROM is designed for. \
	 * @param type System name type. (See the SystemName enum.) \
	 * @return System name, or nullptr if type is invalid. \
	 */ \
	const char *systemName(unsigned int type) const final; \
\
	/** \
	 * Get the RomDataInfo for this class. \
	 * @return RomDataInfo \
	 */ \
	static const LibRpBase::RomDataInfo *romDataInfo_static(void); \
\
	/** \
	 * Get the RomDataInfo for this object. \
	 * @return RomDataInfo \
	 */ \
	const LibRpBase::RomDataInfo *romDataInfo(void) const final; \
\
protected: \
	/** \
	 * Load field data. \
	 * Called by RomData::fields() if the field data hasn't been loaded yet. \
	 * @return 0 on success; negative POSIX error code on error. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	int loadFieldData(void) final;

/**
 * Initial declaration for a RomData subclass.
 * Declares functions common to all RomData subclasses.
 * @param klass Class name
 */
#define ROMDATA_DECL_BEGIN(klass) \
ROMDATA_DECL_BEGIN_NO_CTOR(klass) \
ROMDATA_DECL_CTOR_DEFAULT(klass) \
ROMDATA_DECL_COMMON_FNS()

/**
 * Initial declaration for an exported RomData subclass.
 * NOTE: This should *only* be used for RomDataTestObject.
 * Declares functions common to all RomData subclasses.
 * @param klass Class name
 */
#define ROMDATA_DECL_BEGIN_EXPORT(klass) \
ROMDATA_DECL_BEGIN_NO_CTOR_EXPORT(klass) \
ROMDATA_DECL_CTOR_DEFAULT(klass) \
ROMDATA_DECL_COMMON_FNS()

/**
 * RomData constructors for subclasses that handle directories.
 * [INTERNAL; Use ROMDATA_DECL_CTOR_DIRECTORY() instead.]
 *
 * NOTE: Only static isDirSupported functions are provided.
 */
#define T_ROMDATA_DECL_CTOR_DIRECTORY(klass, CharType) \
public: \
	/** \
	 * Read a directory. (for application packages, extracted file systems, etc.) \
	 * \
	 * NOTE: Check isValid() to determine if the directory is supported by this class. \
	 * \
	 * @param path Local directory path (char for UTF-8, wchar_t for Windows UTF-16) \
	 */ \
	klass(const CharType *path); \
\
	/** \
	 * Is a directory supported by this class? \
	 * @param path Directory to check \
	 * @return Class-specific system ID (>= 0) if supported; -1 if not. \
	 */ \
	static int isDirSupported_static(const CharType *path); \
\
	/** \
	 * Is a directory supported by this class? \
	 * @param path Directory to check \
	 * @return Class-specific system ID (>= 0) if supported; -1 if not. \
	 */ \
	static inline int isDirSupported_static(const std::basic_string<CharType> &path) \
	{ \
		return isDirSupported_static(path.c_str()); \
	}

#if defined(_WIN32) && defined(_UNICODE)
#  define ROMDATA_DECL_CTOR_DIRECTORY(klass) \
	T_ROMDATA_DECL_CTOR_DIRECTORY(klass, char) \
	T_ROMDATA_DECL_CTOR_DIRECTORY(klass, wchar_t)
#else /* !defined(_WIN32) || !defined(_UNICODE) */
#  define ROMDATA_DECL_CTOR_DIRECTORY(klass) \
	T_ROMDATA_DECL_CTOR_DIRECTORY(klass, char)
#endif /* defined(_WIN32) && defined(_UNICODE) */

/**
 * RomData subclass function declaration for loading metadata properties.
 */
#define ROMDATA_DECL_METADATA() \
protected: \
	/** \
	 * Load metadata properties. \
	 * Called by RomData::metaData() if the metadata hasn't been loaded yet. \
	 * @return Number of metadata properties read on success; negative POSIX error code on error. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	int loadMetaData(void) final;

/**
 * RomData subclass function declarations for image handling.
 */
#define ROMDATA_DECL_IMGSUPPORT() \
public: \
	/** \
	 * Get a bitfield of image types this class can retrieve. \
	 * @return Bitfield of supported image types. (ImageTypesBF) \
	 */ \
	static uint32_t supportedImageTypes_static(void); \
\
	/** \
	 * Get a bitfield of image types this object can retrieve. \
	 * @return Bitfield of supported image types. (ImageTypesBF) \
	 */ \
	uint32_t supportedImageTypes(void) const final; \
\
	/** \
	 * Get a list of all available image sizes for the specified image type. \
	 * \
	 * The first item in the returned vector is the "default" size. \
	 * If the width/height is 0, then an image exists, but the size is unknown. \
	 * \
	 * @param imageType Image type. \
	 * @return Vector of available image sizes, or empty vector if no images are available. \
	 */ \
	static std::vector<RomData::ImageSizeDef> supportedImageSizes_static(ImageType imageType); \
\
	/** \
	 * Get a list of all available image sizes for the specified image type. \
	 * \
	 * The first item in the returned vector is the "default" size. \
	 * If the width/height is 0, then an image exists, but the size is unknown. \
	 * \
	 * @param imageType Image type. \
	 * @return Vector of available image sizes, or empty vector if no images are available. \
	 */ \
	std::vector<RomData::ImageSizeDef> supportedImageSizes(ImageType imageType) const final;

/**
 * RomData subclass function declaration for image processing flags.
 */
#define ROMDATA_DECL_IMGPF() \
public: \
	/** \
	 * Get image processing flags. \
	 * \
	 * These specify post-processing operations for images, \
	 * e.g. applying transparency masks. \
	 * \
	 * @param imageType Image type. \
	 * @return Bitfield of ImageProcessingBF operations to perform. \
	 */ \
	uint32_t imgpf(ImageType imageType) const final;

/**
 * RomData subclass function declaration for loading internal images.
 *
 * NOTE: This function needs to be public because it might be
 * called by RomData subclasses that own other RomData subclasses.
 *
 */
#define ROMDATA_DECL_IMGINT() \
public: \
	/** \
	 * Load an internal image. \
	 * Called by RomData::image(). \
	 * @param imageType	[in] Image type to load. \
	 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in. \
	 * @return 0 on success; negative POSIX error code on error. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	int loadInternalImage(ImageType imageType, LibRpTexture::rp_image_const_ptr &pImage) final;

/**
 * RomData subclass function declaration for loading internal image mipmaps.
 *
 * NOTE: This function needs to be public because it might be
 * called by RomData subclasses that own other RomData subclasses.
 *
 */
#define ROMDATA_DECL_IMGINTMIPMAP() \
public: \
	/** \
	 * Load an internal mipmap level for IMG_INT_IMAGE. \
	 * Called by RomData::mipmap(). \
	 * @param mipmapLevel	[in] Mipmap level. \
	 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in. \
	 * @return 0 on success; negative POSIX error code on error. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	int loadInternalMipmap(int mipmapLevel, LibRpTexture::rp_image_const_ptr &pImage) final;

/**
 * RomData subclass function declaration for obtaining URLs for external images.
 */
#define ROMDATA_DECL_IMGEXT() \
public: \
	/** \
	 * Get a list of URLs for an external image type. \
	 * \
	 * A thumbnail size may be requested from the shell. \
	 * If the subclass supports multiple sizes, it should \
	 * try to get the size that most closely matches the \
	 * requested size. \
	 * \
	 * @param imageType     [in]     Image type \
	 * @param extURLs       [out]    Output vector \
	 * @param size          [in,opt] Requested image size. This may be a requested \
	 *                               thumbnail size in pixels, or an ImageSizeType \
	 *                               enum value. \
	 * @return 0 on success; negative POSIX error code on error. \
	 */ \
	int extURLs(ImageType imageType, std::vector<ExtURL> &extURLs, int size = IMAGE_SIZE_DEFAULT) const final;

/**
 * RomData subclass function declaration for loading the animated icon.
 */
#define ROMDATA_DECL_ICONANIM() \
public: \
	/** \
	 * Get the animated icon data. \
	 * \
	 * Check imgpf for IMGPF_ICON_ANIMATED first to see if this \
	 * object has an animated icon. \
	 * \
	 * @return Animated icon data, or nullptr if no animated icon is present. \
	 */ \
	LibRpBase::IconAnimDataConstPtr iconAnimData(void) const final;

/**
 * RomData subclass function declaration for indicating "dangerous" permissions.
 */
#define ROMDATA_DECL_DANGEROUS() \
public: \
	/** \
	 * Does this ROM image have "dangerous" permissions? \
	 * \
	 * @return True if the ROM image has "dangerous" permissions; false if not. \
	 */ \
	bool hasDangerousPermissions(void) const final;

/**
 * RomData subclass function declaration for indicating ROM operations are possible.
 */
#define ROMDATA_DECL_ROMOPS() \
protected: \
	/** \
	 * Get the list of operations that can be performed on this ROM. \
	 * Internal function; called by RomData::romOps(). \
	 * @return List of operations. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	std::vector<RomOp> romOps_int(void) const final; \
\
	/** \
	 * Perform a ROM operation. \
	 * Internal function; called by RomData::doRomOp(). \
	 * @param id		[in] Operation index. \
	 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates) \
	 * @return 0 on success; negative POSIX error code on error. \
	 */ \
	RP_LIBROMDATA_LOCAL \
	int doRomOp_int(int id, RomOpParams *pParams) final;

/**
 * RomData subclass function declaration for "viewed" achievements.
 */
#define ROMDATA_DECL_VIEWED_ACHIEVEMENTS() \
public: \
	/** \
	 * Check for "viewed" achievements. \
	 * \
	 * @return Number of achievements unlocked. \
	 */ \
	int checkViewedAchievements(void) const final;

/**
 * RomData subclass function declaration for closing the internal file handle.
 * Only needed if extra handling is needed, e.g. if multiple files are opened.
 */
#define ROMDATA_DECL_CLOSE() \
public: \
	/** \
	 * Close the opened file. \
	 */ \
	void close(void) final;

/**
 * End of RomData subclass declaration.
 */
#define ROMDATA_DECL_END() };

/** RomData subclass function implementations for static function wrappers. **/

/**
 * Common static function wrappers.
 */
#define ROMDATA_IMPL(klass) \
/** \
 * Is a ROM image supported by this object? \
 * @param info DetectInfo containing ROM detection information. \
 * @return Class-specific system ID (>= 0) if supported; -1 if not. \
 */ \
int klass::isRomSupported(const DetectInfo *info) const \
{ \
	return klass::isRomSupported_static(info); \
} \
\
/** \
 * Get the RomDataInfo for this class. \
 * @return RomDataInfo \
 */ \
const LibRpBase::RomDataInfo *klass::romDataInfo_static(void) \
{ \
	return &klass##Private::romDataInfo; \
} \
\
/** \
 * Get the RomDataInfo for this object. \
 * @return RomDataInfo \
 */ \
const LibRpBase::RomDataInfo *klass::romDataInfo(void) const \
{ \
	return &klass##Private::romDataInfo; \
}

/**
 * Static function wrappers for subclasses that have images.
 */
#define ROMDATA_IMPL_IMG_TYPES(klass) \
/** \
 * Get a bitfield of image types this class can retrieve. \
 * @return Bitfield of supported image types. (ImageTypesBF) \
 */ \
uint32_t klass::supportedImageTypes(void) const \
{ \
	return klass::supportedImageTypes_static(); \
}

#define ROMDATA_IMPL_IMG_SIZES(klass) \
/** \
 * Get a list of all available image sizes for the specified image type. \
 * \
 * The first item in the returned vector is the "default" size. \
 * If the width/height is 0, then an image exists, but the size is unknown. \
 * \
 * @param imageType Image type. \
 * @return Vector of available image sizes, or empty vector if no images are available. \
 */ \
std::vector<RomData::ImageSizeDef> klass::supportedImageSizes(ImageType imageType) const \
{ \
	return klass::supportedImageSizes_static(imageType); \
}

#define ROMDATA_IMPL_IMG(klass) \
	ROMDATA_IMPL_IMG_TYPES(klass) \
	ROMDATA_IMPL_IMG_SIZES(klass)

/** Assertion macros. **/

#define ASSERT_supportedImageSizes(imageType) do { \
	assert((imageType) >= IMG_INT_MIN && (imageType) <= IMG_EXT_MAX); \
	if ((imageType) < IMG_INT_MIN || (imageType) > IMG_EXT_MAX) { \
		/* ImageType is out of range. */ \
		return {}; \
	} \
} while (0)

#define ASSERT_imgpf(imageType) do { \
	assert((imageType) >= IMG_INT_MIN && (imageType) <= IMG_EXT_MAX); \
	if ((imageType) < IMG_INT_MIN || (imageType) > IMG_EXT_MAX) { \
		/* ImageType is out of range. */ \
		return 0; \
	} \
} while (0)

#define ASSERT_loadInternalImage(imageType, pImage) do { \
	assert((imageType) >= IMG_INT_MIN && (imageType) <= IMG_INT_MAX); \
	if ((imageType) < IMG_INT_MIN || (imageType) > IMG_INT_MAX) { \
		/* ImageType is out of range. */ \
		(pImage).reset(); \
		return -ERANGE; \
	} \
} while (0)

#define ASSERT_extURLs(imageType) do { \
	assert((imageType) >= IMG_EXT_MIN && (imageType) <= IMG_EXT_MAX); \
	if ((imageType) < IMG_EXT_MIN || (imageType) > IMG_EXT_MAX) { \
		/* ImageType is out of range. */ \
		return -ERANGE; \
	} \
} while (0)

/**
 * loadInternalImage() implementation for RomData subclasses
 * with only a single type of internal image.
 *
 * @param ourImageType	Internal image type.
 * @param file		IRpFile pointer to check.
 * @param isValid	isValid value to check. (must be true)
 * @param romType	RomType value to check. (must be >= 0; specify 0 if N/A)
 * @param imgCache	Cached image pointer to check. (Specify nullptr if N/A)
 * @param func		Function to load the image.
 */
#define ROMDATA_loadInternalImage_single(ourImageType, file, isValid, romType, imgCache, func) do { \
	if ((imageType) != (ourImageType)) { \
		(pImage).reset(); \
		return -ENOENT; \
	} else if ((imgCache) != nullptr) { \
		(pImage) = (imgCache); \
		return 0; \
	} else if (!(file)) { \
		(pImage).reset(); \
		return -EBADF; \
	} else if (!(isValid) || ((int)(romType)) < 0) { \
		(pImage).reset(); \
		return -EIO; \
	} \
	\
	(pImage) = (func)(); \
	return (pImage) ? 0 : -EIO; \
} while (0)

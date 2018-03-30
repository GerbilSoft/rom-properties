/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomData_decl.hpp: ROM data base class. (Subclass macros)                *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMDATA_DECL_HPP__
#define __ROMPROPERTIES_LIBRPBASE_ROMDATA_DECL_HPP__

#ifndef __ROMPROPERTIES_LIBRPBASE_ROMDATA_HPP__
# error RomData_decl.hpp should only be included by RomData.hpp
#endif

/** Macros for RomData subclasses. **/

/**
 * Initial declaration for a RomData subclass.
 * Declares functions common to all RomData subclasses.
 */
#define ROMDATA_DECL_BEGIN(klass) \
class klass##Private; \
class klass : public LibRpBase::RomData { \
	public: \
		explicit klass(LibRpBase::IRpFile *file); \
	protected: \
		virtual ~klass() { } \
	private: \
		typedef RomData super; \
		friend class klass##Private; \
		RP_DISABLE_COPY(klass); \
	\
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
		 * Get a list of all supported file extensions. \
		 * This is to be used for file type registration; \
		 * subclasses don't explicitly check the extension. \
		 * \
		 * NOTE: The extensions include the leading dot, \
		 * e.g. ".bin" instead of "bin". \
		 * \
		 * NOTE 2: The array and the strings in the array should \
		 * *not* be freed by the caller. \
		 * \
		 * @return NULL-terminated array of all supported file extensions, or nullptr on error. \
		 */ \
		static const char *const *supportedFileExtensions_static(void); \
		\
		/** \
		 * Get a list of all supported file extensions. \
		 * This is to be used for file type registration; \
		 * subclasses don't explicitly check the extension. \
		 * \
		 * NOTE: The extensions include the leading dot, \
		 * e.g. ".bin" instead of "bin". \
		 * \
		 * NOTE 2: The array and the strings in the array should \
		 * *not* be freed by the caller. \
		 * \
		 * @return NULL-terminated array of all supported file extensions, or nullptr on error. \
		 */ \
		const char *const *supportedFileExtensions(void) const final; \
	\
	protected: \
		/** \
		 * Load field data. \
		 * Called by RomData::fields() if the field data hasn't been loaded yet. \
		 * @return 0 on success; negative POSIX error code on error. \
		 */ \
		int loadFieldData(void) final;

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
 */
#define ROMDATA_DECL_IMGINT() \
	public: \
		/** \
		 * Load an internal image. \
		 * Called by RomData::image(). \
		 * @param imageType	[in] Image type to load. \
		 * @param pImage	[out] Pointer to const rp_image* to store the image in. \
		 * @return 0 on success; negative POSIX error code on error. \
		 */ \
		int loadInternalImage(ImageType imageType, const LibRpBase::rp_image **pImage) final;

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
		 * @param imageType     [in]     Image type. \
		 * @param pExtURLs      [out]    Output vector. \
		 * @param size          [in,opt] Requested image size. This may be a requested \
		 *                               thumbnail size in pixels, or an ImageSizeType \
		 *                               enum value. \
		 * @return 0 on success; negative POSIX error code on error. \
		 */ \
		int extURLs(ImageType imageType, std::vector<ExtURL> *pExtURLs, int size = IMAGE_SIZE_DEFAULT) const final;

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
		const LibRpBase::IconAnimData *iconAnimData(void) const final;

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
 * Get a list of all supported file extensions. \
 * This is to be used for file type registration; \
 * subclasses don't explicitly check the extension. \
 * \
 * NOTE: The extensions include the leading dot, \
 * e.g. ".bin" instead of "bin". \
 * \
 * NOTE 2: The array and the strings in the array should \
 * *not* be freed by the caller. \
 * \
 * @return NULL-terminated array of all supported file extensions, or nullptr on error. \
 */ \
const char *const *klass::supportedFileExtensions(void) const \
{ \
	return klass::supportedFileExtensions_static(); \
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

#endif /* __ROMPROPERTIES_LIBRPBASE_ROMDATA_DECL_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat_decl.hpp: Texture file format base class. (Subclass macros)  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_DECL_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_DECL_HPP__

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__
# error FileFormat_decl.hpp should only be included by FileFormat.hpp
#endif

/** Macros for FileFormat subclasses. **/

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
# define FILEFORMAT_GETFIELDS_FUNCTION \
	public: \
		/** \
		 * Get property fields for rom-properties. \
		 * @param fields RomFields object to which fields should be added. \
		 * @return Number of fields added, or 0 on error. \
		 */ \
		int getFields(LibRpBase::RomFields *fields) const final;
#else
# define FILEFORMAT_GETFIELDS_FUNCTION
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/**
 * Initial declaration for a FileFormat subclass.
 * Declares functions common to all FileFormat subclasses.
 */
#define FILEFORMAT_DECL_BEGIN(klass) \
class klass##Private; \
class klass : public LibRpTexture::FileFormat { \
	public: \
		explicit klass(LibRpBase::IRpFile *file); \
	protected: \
		virtual ~klass() { } \
	private: \
		typedef FileFormat super; \
		friend class klass##Private; \
		RP_DISABLE_COPY(klass); \
	\
	public: \
		/** Propety accessors **/ \
		\
		/** \
		 * Get the texture format name. \
		 * @return Texture format name, or nullptr on error. \
		 */ \
		const char *textureFormatName(void) const final; \
		\
		/** \
		 * Get the pixel format, e.g. "RGB888" or "DXT1". \
		 * @return Pixel format, or nullptr if unavailable. \
		 */ \
		const char *pixelFormat(void) const final; \
		\
		/** \
		 * Get the mipmap count. \
		 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps) \
		 */ \
		int mipmapCount(void) const final; \
		\
		FILEFORMAT_GETFIELDS_FUNCTION \
	\
	public: \
		/** Image accessors **/ \
		\
		/** \
		 * Get the image. \
		 * For textures with mipmaps, this is the largest mipmap. \
		 * The image is owned by this object. \
		 * @return Image, or nullptr on error. \
		 */ \
		const rp_image *image(void) const final; \
		\
		/** \
		 * Get the image for the specified mipmap. \
		 * Mipmap 0 is the largest image. \
		 * @param mip Mipmap number. \
		 * @return Image, or nullptr on error. \
		 */ \
		const rp_image *mipmap(int mip) const final;

/**
 * FileFormat subclass function declaration for closing the internal file handle.
 * Only needed if extra handling is needed, e.g. if multiple files are opened.
 */
#define FILEFORMAT_DECL_CLOSE() \
	public: \
		/** \
		 * Close the opened file. \
		 */ \
		void close(void) final;

/**
 * End of FileFormat subclass declaration.
 */
#define FILEFORMAT_DECL_END() };

/** FileFormat subclass function implementations for static function wrappers. **/

/**
 * Common static function wrappers.
 */
#define FILEFORMAT_IMPL(klass) \
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
} \
\
/** \
 * Get a list of all supported MIME types. \
 * This is to be used for metadata extractors that \
 * must indicate which MIME types they support. \
 * \
 * NOTE: The array and the strings in the array should \
 * *not* be freed by the caller. \
 * \
 * @return NULL-terminated array of all supported file extensions, or nullptr on error. \
 */ \
const char *const *klass::supportedMimeTypes(void) const \
{ \
	return klass::supportedMimeTypes_static(); \
}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_DECL_HPP__ */

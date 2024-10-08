/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat_decl.hpp: Texture file format base class. (Subclass macros)  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_FILEFORMAT_HPP__
#  error FileFormat_decl.hpp should only be included by FileFormat.hpp
#endif

/** Macros for FileFormat subclasses. **/

// Other rom-properties libraries
#include "librpfile/IRpFile.hpp"
#include "../img/rp_image.hpp"

namespace LibRpTexture {
	struct TextureInfo;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
#  define FILEFORMAT_GETFIELDS_FUNCTION \
public: \
	/** \
	 * Get property fields for rom-properties. \
	 * @param fields RomFields object to which fields should be added. \
	 * @return Number of fields added, or 0 on error. \
	 */ \
	int getFields(LibRpBase::RomFields *fields) const final;
#else
#  define FILEFORMAT_GETFIELDS_FUNCTION
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/**
 * Initial declaration for a FileFormat subclass.
 *
 * No constructor is included with this version.
 * ROMDATA_DECL_COMMON_FNS() must be used afterwards.
 *
 * @param klass Class name
 */
#define FILEFORMAT_DECL_BEGIN_NO_CTOR(klass) \
class klass##Private; \
class klass final : public LibRpTexture::FileFormat { \
private: \
	typedef FileFormat super; \
	friend class klass##Private; \
	RP_DISABLE_COPY(klass); \

/**
 * Default constructor for a FileFormat subclass.
 */
#define FILEFORMAT_DECL_CTOR_DEFAULT(klass) \
public: \
	/** \
	 * Read a texture file file. \
	 * \
	 * A ROM image must be opened by the caller. The file handle \
	 * will be ref()'d and must be kept open in order to load \
	 * data from the ROM image. \
	 * \
	 * To close the file, either delete this object or call close(). \
	 * \
	 * NOTE: Check isValid() to determine if this is a valid ROM. \
	 * \
	 * @param file Open texture file \
	 */ \
	explicit klass(const LibRpFile::IRpFilePtr &file);

/**
 * Common functions for a FileFormat subclass.
 */
#define FILEFORMAT_DECL_COMMON_FNS() \
public: \
	/** Class-specific functions that can be used even if isValid() is false. **/ \
\
	/** \
	 * Get the static RomDataInfo for this class. \
	 * @return Static RomDataInfo \
	 */ \
	static const LibRpTexture::TextureInfo *textureInfo(void); \
\
public: \
	/** Property accessors **/ \
\
	/** \
	 * Get the pixel format, e.g. "RGB888" or "DXT1". \
	 * @return Pixel format, or nullptr if unavailable. \
	 */ \
	const char *pixelFormat(void) const final; \
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
	LibRpTexture::rp_image_const_ptr image(void) const final; \

/**
 * Initial declaration for a RomData subclass.
 * Declares functions common to all RomData subclasses.
 * @param klass Class name
 */
#define FILEFORMAT_DECL_BEGIN(klass) \
FILEFORMAT_DECL_BEGIN_NO_CTOR(klass) \
FILEFORMAT_DECL_CTOR_DEFAULT(klass) \
FILEFORMAT_DECL_COMMON_FNS()

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
 * FileFormat subclass function declaration for retrieving a mipmap level.
 * Only needed if the FileFormat supports mipmaps. The default implementation
 * returns image() for mip == 0, and nullptr for anything else.
 */
#define FILEFORMAT_DECL_MIPMAP() \
public: \
	/** \
	 * Get the image for the specified mipmap. \
	 * Mipmap 0 is the largest image. \
	 * @param mip Mipmap number. \
	 * @return Image, or nullptr on error. \
	 */ \
	LibRpTexture::rp_image_const_ptr mipmap(int mip) const final;

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
 * Get the static TextureInfo for this class. \
 * @return Static TextureInfo \
 */ \
const LibRpTexture::TextureInfo *klass::textureInfo(void) \
{ \
	return &klass##Private::textureInfo; \
}

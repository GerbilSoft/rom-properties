/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * XboxXPR.hpp: Microsoft Xbox XPR0 texture reader.                        *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_XBOXXPR_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_XBOXXPR_HPP__

#include "FileFormat.hpp"

namespace LibRpTexture {

class XboxXPRPrivate;
class XboxXPR : public FileFormat
{
	public:
		XboxXPR(LibRpBase::IRpFile *file);
		~XboxXPR();

	private:
		typedef FileFormat super;
		RP_DISABLE_COPY(XboxXPR)
	protected:
		friend class XboxXPRPrivate;
		XboxXPRPrivate *const d_ptr;

	public:
		/** Propety accessors **/

		/**
		 * Get the texture format name.
		 * @return Texture format name, or nullptr on error.
		 */
		const char *textureFormatName(void) const final;

		// TODO: Move the dimensions code up to the FileFormat base class.

		/**
		 * Get the image width.
		 * @return Image width.
		 */
		int width(void) const final;

		/**
		 * Get the image height.
		 * @return Image height.
		 */
		int height(void) const final;

		/**
		 * Get the image dimensions.
		 * If the image is 2D, then 'z' will be set to zero.
		 * @param pBuf Three-element array for [x, y, z].
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int getDimensions(int *pBuf) const final;

		/**
		 * Get the pixel format, e.g. "RGB888" or "DXT1".
		 * @return Pixel format, or nullptr if unavailable.
		 */
		const char *pixelFormat(void) const final;

		/**
		 * Get the mipmap count.
		 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
		 */
		int mipmapCount(void) const final;

	public:
		/** Image accessors **/

		/**
		 * Get the image.
		 * For textures with mipmaps, this is the largest mipmap.
		 * The image is owned by this object.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *image(void) const final;

		/**
		 * Get the image for the specified mipmap.
		 * Mipmap 0 is the largest image.
		 * @param num Mipmap number.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *mipmap(int num) const final;
};

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_FILEFORMAT_XBOXXPR_HPP__ */

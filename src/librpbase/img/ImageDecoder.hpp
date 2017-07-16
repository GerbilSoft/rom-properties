/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder.cpp: Image decoding functions.                             *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRpBase {

class rp_image;

class ImageDecoder
{
	private:
		// ImageDecoder is a static class.
		ImageDecoder();
		~ImageDecoder();
		RP_DISABLE_COPY(ImageDecoder)

	public:
		/** Dreamcast **/

		/**
		 * Convert a Dreamcast linear (rectangle) CI4 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI4 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 16*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDreamcastLinearCI4(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/**
		 * Convert a Dreamcast linear (rectangle) CI8 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI8 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 256*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDreamcastLinearCI8(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/**
		 * Convert a Dreamcast linear (rectangle) ARGB4444 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf ARGB4444 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDreamcastLinearARGB4444(int width, int height,
			const uint16_t *img_buf, int img_siz);

		/**
		 * Convert a Dreamcast linear (rectangle) monochrome image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf Monochrome image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/8]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDreamcastLinearMono(int width, int height,
			const uint8_t *img_buf, int img_siz);

		/** GameCube **/

		/**
		 * Convert a GameCube RGB5A3 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf RGB5A3 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromGcnRGB5A3(int width, int height,
			const uint16_t *img_buf, int img_siz);

		/**
		 * Convert a GameCube CI8 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI8 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 256*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromGcnCI8(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/** Nintendo DS **/

		/**
		 * Convert a Nintendo DS CI4 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI4 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 16*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromNDS_CI4(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/** Nintendo 3DS **/

		/**
		 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf RGB565 tiled image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromN3DSTiledRGB565(int width, int height,
			const uint16_t *img_buf, int img_siz);

		/** PlayStation **/

		/**
		 * Convert a PlayStation CI4 image to rp_image.
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI8 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @param pal_buf Palette buffer.
		 * @param pal_siz Size of palette data. [must be >= 16*2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromPS1_CI4(int width, int height,
			const uint8_t *img_buf, int img_siz,
			const uint16_t *pal_buf, int pal_siz);

		/* S3TC */

		/**
		 * Convert a DXT1 image to rp_image. (big-endian)
		 * @param width Image width.
		 * @param height Image height.
		 * @param img_buf CI8 image buffer.
		 * @param img_siz Size of image data. [must be >= (w*h)/2]
		 * @return rp_image, or nullptr on error.
		 */
		static rp_image *fromDXT1_BE(int width, int height,
			const uint8_t *img_buf, int img_siz);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_IMAGEDECODER_HPP__ */

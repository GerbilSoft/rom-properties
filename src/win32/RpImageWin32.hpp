/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpImageWin32.hpp: rp_image to Win32 conversion functions.               *
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

#ifndef __ROMPROPERTIES_WIN32_RPIMAGEWIN32_HPP__
#define __ROMPROPERTIES_WIN32_RPIMAGEWIN32_HPP__

#include "libromdata/RomData.hpp"
namespace LibRomData {
	class rp_image;
}

class RpImageWin32
{
	private:
		RpImageWin32();
		~RpImageWin32();
	private:
		RP_DISABLE_COPY(RpImageWin32)

	public:
		/**
		 * Get an internal image.
		 * NOTE: The image is owned by the RomData object;
		 * caller must NOT delete it!
		 *
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return Internal image, or nullptr on error.
		 */
		static const LibRomData::rp_image *getInternalImage(
			const LibRomData::RomData *romData,
			LibRomData::RomData::ImageType imageType);

		/**
		 * Get an external image.
		 * NOTE: Caller must delete the image after use.
		 *
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return External image, or nullptr on error.
		 */
		static LibRomData::rp_image *getExternalImage(
			const LibRomData::RomData *romData,
			LibRomData::RomData::ImageType imageType);

	protected:
		/**
		 * Convert an rp_image to a HBITMAP for use as an icon mask.
		 * @param image rp_image.
		 * @return HBITMAP, or nullptr on error.
		 */
		static HBITMAP toHBITMAP_mask(const LibRomData::rp_image *image);

	public:
		/**
		 * Convert an rp_image to HBITMAP.
		 * @param image		[in] rp_image.
		 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
		 * @return HBITMAP, or nullptr on error.
		 */
		static HBITMAP toHBITMAP(const LibRomData::rp_image *image, uint32_t bgColor);

		/**
		 * Convert an rp_image to HBITMAP.
		 * This version resizes the image.
		 * @param image		[in] rp_image.
		 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
		 * @param size		[in] If non-zero, resize the image to this size.
		 * @param nearest	[in] If true, use nearest-neighbor scaling.
		 * @return HBITMAP, or nullptr on error.
		 */
		static HBITMAP toHBITMAP(const LibRomData::rp_image *image, uint32_t bgColor,
					const SIZE &size, bool nearest);

		/**
		 * Convert an rp_image to HBITMAP.
		 * This version preserves the alpha channel.
		 * @param image	[in] rp_image.
		 * @return HBITMAP, or nullptr on error.
		 */
		static HBITMAP toHBITMAP_alpha(const LibRomData::rp_image *image);

		/**
		 * Convert an rp_image to HBITMAP.
		 * This version preserves the alpha channel and resizes the image.
		 * @param image		[in] rp_image.
		 * @param size		[in] If non-zero, resize the image to this size.
		 * @param nearest	[in] If true, use nearest-neighbor scaling.
		 * @return HBITMAP, or nullptr on error.
		 */
		static HBITMAP toHBITMAP_alpha(const LibRomData::rp_image *image, const SIZE &size, bool nearest);

		/**
		 * Convert an rp_image to HICON.
		 * @param image rp_image.
		 * @return HICON, or nullptr on error.
		 */
		static HICON toHICON(const LibRomData::rp_image *image);

		/**
		 * Convert an HBITMAP to rp_image.
		 * @param hBitmap HBITMAP.
		 * @return rp_image.
		 */
		static LibRomData::rp_image *fromHBITMAP(HBITMAP hBitmap);
};

#endif /* __ROMPROPERTIES_WIN32_RPIMAGEWIN32_HPP__ */

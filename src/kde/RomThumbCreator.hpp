/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.hpp: Thumbnail creator.                                 *
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

#ifndef __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__
#define __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__

#include <kio/thumbcreator.h>
class QUrl;

// TODO: ThumbCreatorV2 on KDE4 for user configuration?
// (This was merged into ThumbCreator for KDE5.)

#include "libromdata/img/TCreateThumbnail.hpp"
namespace LibRomData {
	class rp_image;
}

class RomThumbCreator : public ThumbCreator, public LibRomData::TCreateThumbnail<QImage>
{
	public:
		virtual bool create(const QString &path, int width, int height, QImage &img) final;

	private:
		typedef ThumbCreator super;

	protected:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual QImage rpImageToImgClass(const LibRomData::rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const QImage &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual QImage getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. QImage.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(const QImage &imgClass) const final;

		/**
		 * Get an ImgClass's size.
		 * @param imgClass ImgClass object.
		 * @retrun Size.
		 */
		virtual ImgSize getImgSize(const QImage &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual QImage rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual LibRomData::rp_string proxyForUrl(const LibRomData::rp_string &url) const final;
};

#endif /* __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__ */

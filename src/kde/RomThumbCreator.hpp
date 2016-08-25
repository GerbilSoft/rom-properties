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

namespace LibRomData {
	class rp_image;
}

class RomThumbCreator : public ThumbCreator
{
	public:
		virtual bool create(const QString &path, int width, int height, QImage &img) override;

	private:
		typedef ThumbCreator super;

	protected:
		/**
		 * Convert an rp_image to QImage.
		 * TODO: Move to another file?
		 * @param image rp_image.
		 * @return QImage.
		 */
		static QImage rpToQImage(const LibRomData::rp_image *image);

		/**
		 * Download an image from a URL.
		 * @param url URL.
		 * @return QImage, or invalid QImage if an error occurred.
		 */
		QImage download(const QString &url);
};

#endif /* __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__ */

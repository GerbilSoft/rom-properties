/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.hpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include <QtCore/qglobal.h>
#include <kio/thumbcreator.h>
class QUrl;

// TODO: ThumbCreatorV2 on KDE4 for user configuration?
// (This was merged into ThumbCreator for KDE5.)

namespace LibRomData {
	class rp_image;
}

class RomThumbCreatorPrivate;
class RomThumbCreator : public ThumbCreator
{
	public:
		RomThumbCreator();
		virtual ~RomThumbCreator();

	public:
		/**
		 * Creates a thumbnail.
		 *
		 * Note that this method should not do any scaling.  The @p width and @p
		 * height parameters are provided as hints for images that are generated
		 * from non-image data (like text).
		 *
		 * @param path    The path of the file to create a preview for.  This is
		 *                always a local path.
		 * @param width   The requested preview width (see the note on scaling
		 *                above).
		 * @param height  The requested preview height (see the note on scaling
		 *                above).
		 * @param img     The QImage to store the preview in.
		 *
		 * @return @c true if a preview was successfully generated and store in @p
		 *         img, @c false otherwise.
		 */
		virtual bool create(const QString &path, int width, int height, QImage &img) override final;

		/**
		 * Returns the flags for this plugin.
		 *
		 * @return XOR'd flags values.
		 * @see Flags
		 */
		virtual Flags flags(void) const override final;

	private:
		typedef ThumbCreator super;
		RomThumbCreatorPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(RomThumbCreator)
		Q_DISABLE_COPY(RomThumbCreator)
};

#endif /* __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__ */

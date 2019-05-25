/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.hpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__
#define __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__

#include <QtCore/qglobal.h>
#include <kio/thumbcreator.h>

// TODO: ThumbCreatorV2 on KDE4 for user configuration?
// (This was merged into ThumbCreator for KDE5.)

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
		bool create(const QString &path, int width, int height, QImage &img) final;

		/**
		 * Returns the flags for this plugin.
		 *
		 * @return XOR'd flags values.
		 * @see Flags
		 */
		Flags flags(void) const final;

	private:
		typedef ThumbCreator super;
		RomThumbCreatorPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(RomThumbCreator)
		Q_DISABLE_COPY(RomThumbCreator)
};

#endif /* __ROMPROPERTIES_KDE_ROMTHUMBCREATOR_HPP__ */

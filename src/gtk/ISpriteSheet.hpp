/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * ISpriteSheet.hpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "PIMGTYPE.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"

class NOVTABLE ISpriteSheet
{
protected:
	/**
	 * Sprite sheet loader
	 * @param cols Number of columns
	 * @param rows Number of rows
	 * @param width Icon width
	 * @param height Icon height
	 */
	ISpriteSheet(int cols, int rows, int width, int height);
public:
	virtual ~ISpriteSheet() = default;

private:
	RP_DISABLE_COPY(ISpriteSheet)

protected:
	/**
	 * Get the gresource filename for a sprite sheet.
	 * @param buf		[out] Filename buffer
	 * @param size		[in] Size of buf
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return 0 on success; non-zero on error.
	 */
	virtual int getFilename(char *buf, size_t size, int width, int height, bool gray = false) const = 0;

	/**
	 * Get an icon from the sprite sheet.
	 * @param col Column
	 * @param row Row
	 * @param gray If true, load the grayscale version
	 * @return Icon, or nullptr on error. (caller must free the icon)
	 */
	PIMGTYPE getIcon(int col, int row, bool gray = false) const;

private:
	LibRpTexture::rp_image_ptr m_img, m_imgGray;
	int m_cols, m_rows;
	int m_width, m_height;
};

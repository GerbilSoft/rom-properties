/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ISpriteSheet.hpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// librptexture
#include "librptexture/img/rp_image.hpp"

class ISpriteSheet {
protected:
	/**
	 * Sprite sheet loader
	 * @param cols Number of columns
	 * @param rows Number of rows
	 * @param width Icon width
	 * @param height Icon height
	 * @param flipH If true, flip horizontally for RTL.
	 */
	ISpriteSheet(int cols, int rows, int width, int height, bool flipH = false);
public:
	virtual ~ISpriteSheet() = default;

private:
	RP_DISABLE_COPY(ISpriteSheet)

protected:
	/**
	 * Get the RT_PNG resource ID for a sprite sheet.
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return Resource ID, or nullptr on error.
	 */
	virtual LPCTSTR getResourceID(int width, int height, bool gray = false) const = 0;

private:
	/**
	 * Extract an HBITMAP sprite from an rp_image sprite sheet.
	 * Caller must delete the HBITMAP after use.
	 * @param imgSpriteSheet	[in] rp_image sprite sheet
	 * @param x			[in] X pos
	 * @param y			[in] Y pos
	 * @param width			[in] Width
	 * @param height		[in] Height
	 * @param dpi			[in,opt] DPI value.
	 * @return Sub-bitmap, or nullptr on error.
	 */
	static HBITMAP getSubBitmap(const LibRpTexture::rp_image *img, int x, int y, int w, int h, UINT dpi = 96);

protected:
	/**
	 * Get an icon from the sprite sheet.
	 * @param col Column
	 * @param row Row
	 * @param gray If true, load the grayscale version
	 * @param dpi DPI value to set in the HBITMAP
	 * @return Icon, or nullptr on error. (caller must free the icon)
	 */
	HBITMAP getIcon(int col, int row, bool gray = false, UINT dpi = 96) const;

private:
	LibRpTexture::rp_image_ptr m_img, m_imgGray;
	int m_cols, m_rows;
	int m_width, m_height;
	bool m_flipH;
};

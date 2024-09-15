/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ISpriteSheet.hpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

#include <QPixmap>

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
	 * Get the qresource filename for a sprite sheet.
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return Filename on success; empty string on error.
	 */
	virtual QString getFilename(int width, int height, bool gray = false) const = 0;

	/**
	 * Get an icon from the sprite sheet.
	 * @param col Column
	 * @param row Row
	 * @param gray If true, load the grayscale version
	 * @return Icon, or null QImage on error.
	 */
	QPixmap getIcon(int col, int row, bool gray = false) const;

private:
	QPixmap m_img, m_imgGray;
	int m_cols, m_rows;
	int m_width, m_height;
};

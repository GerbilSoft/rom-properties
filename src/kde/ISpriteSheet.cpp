/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ISpriteSheet.cpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ISpriteSheet.hpp"

/**
 * Sprite sheet loader
 * @param cols Number of columns
 * @param rows Number of rows
 * @param width Icon width
 * @param height Icon height
 */
ISpriteSheet::ISpriteSheet(int cols, int rows, int width, int height)
	: m_cols(cols)
	, m_rows(rows)
	, m_width(width)
	, m_height(height)
{}

/**
 * Get an icon from the sprite sheet.
 * @param col Column
 * @param row Row
 * @param gray If true, load the grayscale version
 * @return Icon, or null QPixmap on error.
 */
QPixmap ISpriteSheet::getIcon(int col, int row, bool gray) const
{
	assert(col >= 0);
	assert(col < m_cols);
	assert(row >= 0);
	assert(row <= m_rows);
	if (col < 0 || col >= m_cols || row < 0 || row >= m_rows) {
		// Invalid col/row.
		return {};
	}

	// Do we need to load the sprite sheet?
	QPixmap &imgSpriteSheet = gray
		? const_cast<ISpriteSheet*>(this)->m_imgGray
		: const_cast<ISpriteSheet*>(this)->m_img;
	if (imgSpriteSheet.isNull()) {
		// Load the sprite sheet.
		QString qres_filename = getFilename(m_width, m_height, gray);
		if (qres_filename.isEmpty()) {
			// Unable to get the filename.
			return {};
		}

		imgSpriteSheet.load(qres_filename);
		assert(!imgSpriteSheet.isNull());
		if (imgSpriteSheet.isNull()) {
			// Unable to load the sprite sheet.
			return {};
		}

		// Make sure the bitmap has the expected size.
		assert(imgSpriteSheet.width()  == (int)(m_width * m_cols));
		assert(imgSpriteSheet.height() == (int)(m_height * m_rows));
		if (imgSpriteSheet.width()  != (int)(m_width * m_cols) ||
		    imgSpriteSheet.height() != (int)(m_height * m_rows))
		{
			// Incorrect size. We can't use it.
			imgSpriteSheet = QPixmap();
			return {};
		}
	}

	// Extract the sub-icon.
	return imgSpriteSheet.copy(col*m_width, row*m_height, m_width, m_height);
}

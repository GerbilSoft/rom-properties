/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpriteSheet.hpp: Achievement sprite sheets loader.                   *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchSpriteSheet.hpp"

// librpbase
using LibRpBase::Achievements;

/**
 * Achievements sprite sheet
 * @param iconSize Icon size
 */
AchSpriteSheet::AchSpriteSheet(int iconSize)
	: super(Achievements::ACH_SPRITE_SHEET_COLS, Achievements::ACH_SPRITE_SHEET_ROWS, iconSize, iconSize)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);
}

/**
 * Get the gresource filename for a sprite sheet.
 * @param buf		[out] Filename buffer
 * @param size		[in] Size of buf
 * @param width		[in] Icon width
 * @param height	[in] Icon height
 * @param gray		[in] If true, load the grayscale version
 * @return 0 on success; non-zero on error.
 */
int AchSpriteSheet::getFilename(char *buf, size_t size, int width, int height, bool gray)
{
	snprintf(buf, size,
		"/com/gerbilsoft/rom-properties/ach/ach%s-%dx%d.png",
		(gray ? "-gray" : ""), width, height);
	return 0;
}

/**
 * Get an Achievements icon.
 * @param id Achievement ID
 * @param gray If true, load the grayscale version
 * @return Achievements icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE AchSpriteSheet::getIcon(Achievements::ID id, bool gray)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return nullptr;
	}

	// Determine row and column.
	const int col = ((int)id % Achievements::ACH_SPRITE_SHEET_COLS);
	const int row = ((int)id / Achievements::ACH_SPRITE_SHEET_COLS);

	// Call the superclass function.
	return super::getIcon(col, row, gray);
}

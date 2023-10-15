/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpriteSheet.hpp: Achievement sprite sheets loader.                   *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchSpriteSheet.hpp"

#include "res/resource.h"

// librpbase
using LibRpBase::Achievements;

/**
 * Achievements sprite sheet
 * @param iconSize Icon size
 * @param flipH If true, flip horizontally for RTL.
 */
AchSpriteSheet::AchSpriteSheet(int iconSize, bool flipH)
	: super(Achievements::ACH_SPRITE_SHEET_COLS, Achievements::ACH_SPRITE_SHEET_ROWS, iconSize, iconSize, flipH)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);
}

/**
 * Get the RT_PNG resource ID for a sprite sheet.
 * @param width		[in] Icon width
 * @param height	[in] Icon height
 * @param gray		[in] If true, load the grayscale version
 * @return Resource ID, or nullptr on error.
 */
LPCTSTR AchSpriteSheet::getResourceID(int width, int height, bool gray) const
{
	assert(width == height);
	assert(width == 16 || width == 24 || width == 32 || width == 64);

	UINT resourceID;
	switch (width) {
		case 16:
			resourceID = (gray ? IDP_ACH_GRAY_16x16 : IDP_ACH_16x16);
			break;
		case 24:
			resourceID = (gray ? IDP_ACH_GRAY_24x24 : IDP_ACH_24x24);
			break;
		case 32:
			resourceID = (gray ? IDP_ACH_GRAY_32x32 : IDP_ACH_32x32);
			break;
		case 64:
			resourceID = (gray ? IDP_ACH_GRAY_64x64 : IDP_ACH_64x64);
			break;
		default:
			assert(!"Invalid icon size.");
			resourceID = 0;
			break;
	}

	return MAKEINTRESOURCE(resourceID);
}

/**
 * Get an Achievements icon.
 * @param id Achievement ID
 * @param gray If true, load the grayscale version
 * @param dpi DPI value to set in the HBITMAP
 * @return Achievements icon, or nullptr on error. (caller must free the icon)
 */
HBITMAP AchSpriteSheet::getIcon(Achievements::ID id, bool gray, UINT dpi) const
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
	return super::getIcon(col, row, gray, dpi);
}

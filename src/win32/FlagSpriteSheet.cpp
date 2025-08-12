/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * FlagSpriteSheet.hpp: Flag sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FlagSpriteSheet.hpp"

#include "res/resource.h"

// librpbase
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

/**
 * Flags sprite sheet
 * @param iconSize Icon size
 * @param flipH If true, flip horizontally for RTL.
 */
FlagSpriteSheet::FlagSpriteSheet(int iconSize, bool flipH)
	: super(SystemRegion::FLAGS_SPRITE_SHEET_COLS, SystemRegion::FLAGS_SPRITE_SHEET_ROWS, iconSize, iconSize, flipH)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32);
}

/**
 * Get the RT_PNG resource ID for a sprite sheet.
 * @param width		[in] Icon width
 * @param height	[in] Icon height
 * @param gray		[in] If true, load the grayscale version
 * @return Resource ID, or nullptr on error.
 */
LPCTSTR FlagSpriteSheet::getResourceID(int width, int height, bool gray) const
{
	// NOTE: Gray is not used for flags.
	assert(width == height);
	assert(width == 16 || width == 24 || width == 32);
	assert(!gray);

	UINT resourceID;
	switch (width) {
		case 16:
			resourceID = IDP_FLAGS_16x16;
			break;
		case 24:
			resourceID = IDP_FLAGS_24x24;
			break;
		case 32:
			resourceID = IDP_FLAGS_32x32;
			break;
		default:
			assert(!"Invalid icon size.");
			resourceID = 0;
			break;
	}

	return MAKEINTRESOURCE(resourceID);
}

/**
 * Get a flag icon.
 * @param lc		[in]  Language code
 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
 * @param dpi		[in,opt] DPI value to set in the HBITMAP
 * @return Flag icon, or nullptr on error. (caller must free the icon)
 */
HBITMAP FlagSpriteSheet::getIcon(uint32_t lc, bool forcePAL, UINT dpi) const
{
	assert(lc != 0);
	if (lc == 0) {
		// Invalid language code.
		return nullptr;
	}

	// Determine row and column.
	int col, row;
	if (!SystemRegion::getFlagPosition(lc, &col, &row, forcePAL)) {
		// Found a matching icon.
		// Call the superclass function.
		return super::getIcon(col, row, false, dpi);
	}

	// No matching icon...
	return nullptr;
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * FlagSpriteSheet.hpp: Flag sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FlagSpriteSheet.hpp"

// librpbase
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

/**
 * Flags sprite sheet
 * @param iconSize Icon size
 */
FlagSpriteSheet::FlagSpriteSheet(int iconSize)
	: super(4, 4, iconSize, iconSize)
{
	// FIXME: Hard-coded flag cols/rows values...
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32);
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
int FlagSpriteSheet::getFilename(char *buf, size_t size, int width, int height, bool gray)
{
	// NOTE: Gray is not used for flags.
	RP_UNUSED(gray);
	snprintf(buf, size,
		"/com/gerbilsoft/rom-properties/flags/flags-%dx%d.png",
		width, height);
	return 0;
}

/**
 * Get a flag icon.
 * @param lc		[in]  Language code
 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
 * @return Flag icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE FlagSpriteSheet::getIcon(uint32_t lc, bool forcePAL)
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
		return super::getIcon(col, row);
	}

	// No matching icon...
	return nullptr;
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * FlagSpriteSheet.hpp: Flag sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FlagSpriteSheet.hpp"

// librpbase
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// libfmt
#include "rp-libfmt.h"

/**
 * Flags sprite sheet
 * @param iconSize Icon size
 */
FlagSpriteSheet::FlagSpriteSheet(int iconSize)
	: super(SystemRegion::FLAGS_SPRITE_SHEET_COLS, SystemRegion::FLAGS_SPRITE_SHEET_ROWS, iconSize, iconSize)
{
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
int FlagSpriteSheet::getFilename(char *buf, size_t size, int width, int height, bool gray) const
{
	// NOTE: Gray is not used for flags.
	RP_UNUSED(gray);

	assert(buf != nullptr);
	assert(size > 0);
	if (!buf || size == 0) {
		return -EINVAL;
	}

	auto result = fmt::format_to_n(buf, size-1,
		FSTR("/com/gerbilsoft/rom-properties/flags/flags-{:d}x{:d}.png"),
			width, height);
	*(result.out) = '\0';
	return 0;
}

/**
 * Get a flag icon.
 * @param lc		[in]  Language code
 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
 * @return Flag icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE FlagSpriteSheet::getIcon(uint32_t lc, bool forcePAL) const
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

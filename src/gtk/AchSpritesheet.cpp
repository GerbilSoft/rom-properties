/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpritesheet.hpp: Achievement spritesheets loader.                    *
 *                                                                         *
 * Copyright (c) 2020-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchSpritesheet.hpp"

// librpbase, librptexture
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;

namespace AchSpritesheet {

/**
 * Load the specified Achievements icon sprite sheet.
 * Caller must free it after use.
 * @param iconSize Icon size. (16, 24, 32, 64)
 * @return PIMGTYPE, or nullptr on error.
 */
PIMGTYPE load(int iconSize, bool gray)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);

	char ach_filename[64];
	snprintf(ach_filename, sizeof(ach_filename),
		"/com/gerbilsoft/rom-properties/ach/ach%s-%dx%d.png",
		(gray ? "-gray" : ""), iconSize, iconSize);
	PIMGTYPE imgAchSheet = PIMGTYPE_load_png_from_gresource(ach_filename);
	assert(imgAchSheet != nullptr);
	if (!imgAchSheet) {
		// Unable to load the achievements sprite sheet.
		return nullptr;
	}

	// Make sure the bitmap has the expected size.
	assert(PIMGTYPE_size_check(imgAchSheet,
		(int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
		(int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)));
	if (!PIMGTYPE_size_check(imgAchSheet,
	     (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
	     (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)))
	{
		// Incorrect size. We can't use it.
		PIMGTYPE_unref(imgAchSheet);
		return nullptr;
	}

	// Sprite sheet is correct.
	return imgAchSheet;
}

}

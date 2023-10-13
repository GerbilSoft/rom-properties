/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpritesheet.hpp: Achievement spritesheets loader.                    *
 *                                                                         *
 * Copyright (c) 2020-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchSpritesheet.hpp"

// librpbase
using LibRpBase::Achievements;

/**
 * Achievements spritesheet
 * @param iconSize Icon size
 */
AchSpritesheet::AchSpritesheet(int iconSize)
	: m_img(nullptr)
	, m_imgGray(nullptr)
	, m_iconSize(iconSize)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);
}

AchSpritesheet::~AchSpritesheet()
{
	if (m_img) {
		PIMGTYPE_unref(m_img);
	}
	if (m_imgGray) {
		PIMGTYPE_unref(m_imgGray);
	}
}

/**
 * Get an Achievements icon.
 * @param id Achievement ID
 * @param gray If true, load the grayscale version
 * @return Achievements icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE AchSpritesheet::getIcon(Achievements::ID id, bool gray)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return nullptr;
	}

	// Do we need to load the sprite sheet?
	PIMGTYPE &imgAchSheet = gray ? m_imgGray : m_img;
	if (!imgAchSheet) {
		// Load the sprite sheet.
		char ach_filename[64];
		snprintf(ach_filename, sizeof(ach_filename),
			"/com/gerbilsoft/rom-properties/ach/ach%s-%dx%d.png",
			(gray ? "-gray" : ""), m_iconSize, m_iconSize);
		imgAchSheet = PIMGTYPE_load_png_from_gresource(ach_filename);
		assert(imgAchSheet != nullptr);
		if (!imgAchSheet) {
			// Unable to load the achievements sprite sheet.
			return nullptr;
		}
		// Make sure the bitmap has the expected size.
		assert(PIMGTYPE_size_check(imgAchSheet,
			(int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
			(int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)));
		if (!PIMGTYPE_size_check(imgAchSheet,
		     (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_COLS),
		     (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_ROWS)))
		{
			// Incorrect size. We can't use it.
			PIMGTYPE_unref(imgAchSheet);
			imgAchSheet = nullptr;
			return nullptr;
		}
	}

	// Determine row and column.
	const int col = ((int)id % Achievements::ACH_SPRITE_SHEET_COLS);
	const int row = ((int)id / Achievements::ACH_SPRITE_SHEET_COLS);

	// Extract the sub-icon.
	PIMGTYPE subIcon = PIMGTYPE_get_subsurface(imgAchSheet, col*m_iconSize, row*m_iconSize, m_iconSize, m_iconSize);
	assert(subIcon != nullptr);
	return subIcon;
}

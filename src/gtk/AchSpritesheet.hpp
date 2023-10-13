/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpritesheet.hpp: Achievement spritesheets loader.                    *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "PIMGTYPE.hpp"

// librpbase
#include "librpbase/Achievements.hpp"

class AchSpritesheet {
public:
	/**
	 * Achievements spritesheet
	 * @param iconSize Icon size
	 */
	AchSpritesheet(int iconSize);

	~AchSpritesheet();

private:
	RP_DISABLE_COPY(AchSpritesheet)

public:
	/**
	 * Get an Achievements icon.
	 * @param id Achievement ID
	 * @param gray If true, load the grayscale version
	 * @return Achievements icon, or nullptr on error. (caller must free the icon)
	 */
	PIMGTYPE getIcon(LibRpBase::Achievements::ID id, bool gray = false);

private:
	PIMGTYPE m_img;
	PIMGTYPE m_imgGray;
	int m_iconSize;
};

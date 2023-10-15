/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpriteSheet.hpp: Achievement sprite sheets loader.                   *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// librpbase
#include "librpbase/Achievements.hpp"

#include "ISpriteSheet.hpp"
class AchSpriteSheet : public ISpriteSheet {
public:
	/**
	 * Achievements sprite sheet
	 * @param iconSize Icon size
	 * @param flipH If true, flip horizontally for RTL.
	 */
	AchSpriteSheet(int iconSize, bool flipH = false);

private:
	typedef ISpriteSheet super;
	RP_DISABLE_COPY(AchSpriteSheet)

protected:
	/**
	 * Get the RT_PNG resource ID for a sprite sheet.
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return Resource ID, or nullptr on error.
	 */
	LPCTSTR getResourceID(int width, int height, bool gray = false) const final;

public:
	/**
	 * Get an Achievements icon.
	 * @param id Achievement ID
	 * @param gray If true, load the grayscale version
	 * @param dpi DPI value to set in the HBITMAP
	 * @return Achievements icon, or nullptr on error. (caller must free the icon)
	 */
	HBITMAP getIcon(LibRpBase::Achievements::ID id, bool gray = false, UINT dpi = 96) const;
};

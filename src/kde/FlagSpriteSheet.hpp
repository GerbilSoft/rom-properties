/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * FlagSpriteSheet.hpp: Flag sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

#include "ISpriteSheet.hpp"
class FlagSpriteSheet : public ISpriteSheet
{
public:
	/**
	 * Flags sprite sheet
	 * @param iconSize Icon size
	 */
	FlagSpriteSheet(int iconSize);

private:
	typedef ISpriteSheet super;
	RP_DISABLE_COPY(FlagSpriteSheet)

protected:
	/**
	 * Get the gresource filename for a sprite sheet.
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return Filename on success; empty string on error.
	 */
	QString getFilename(int width, int height, bool gray = false) const final;

public:
	/**
	 * Get a flag icon.
	 * @param lc		[in]  Language code
	 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
	 * @return Flag icon, or nullptr on error.
	 */
	QPixmap getIcon(uint32_t lc, bool forcePAL = false) const;
};

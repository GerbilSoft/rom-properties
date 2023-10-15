/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
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
	 */
	AchSpriteSheet(int iconSize);

private:
	typedef ISpriteSheet super;
	RP_DISABLE_COPY(AchSpriteSheet)

protected:
	/**
	 * Get the gresource filename for a sprite sheet.
	 * @param buf		[out] Filename buffer
	 * @param size		[in] Size of buf
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return 0 on success; non-zero on error.
	 */
	int getFilename(char *buf, size_t size, int width, int height, bool gray = false) const final;

public:
	/**
	 * Get an Achievements icon.
	 * @param id Achievement ID
	 * @param gray If true, load the grayscale version
	 * @return Achievements icon, or nullptr on error.
	 */
	QPixmap getIcon(LibRpBase::Achievements::ID id, bool gray = false) const;
};

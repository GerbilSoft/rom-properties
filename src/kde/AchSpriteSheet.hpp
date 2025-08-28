/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * AchSpriteSheet.hpp: Achievement sprite sheets loader.                   *
 *                                                                         *
 * Copyright (c) 2020-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// librpbase
#include "librpbase/Achievements.hpp"

#include "ISpriteSheet.hpp"
class AchSpriteSheet : public ISpriteSheet
{
public:
	/**
	 * Achievements sprite sheet
	 * @param iconSize Icon size
	 */
	AchSpriteSheet(int iconSize);

private:
	typedef ISpriteSheet super;
	// FIXME: Disabling copy breaks array<AchSpriteSheet> on gcc5.
#if defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 5
	RP_DISABLE_COPY(AchSpriteSheet)
#endif /* __clang__ || !__GNUC__ || __GNUC__ > 5 */

protected:
	/**
	 * Get the qresource filename for a sprite sheet.
	 * @param width		[in] Icon width
	 * @param height	[in] Icon height
	 * @param gray		[in] If true, load the grayscale version
	 * @return Filename on success; empty string on error.
	 */
	QString getFilename(int width, int height, bool gray = false) const final;

public:
	/**
	 * Get an Achievements icon.
	 * @param id Achievement ID
	 * @param gray If true, load the grayscale version
	 * @return Achievements icon, or nullptr on error.
	 */
	QPixmap getIcon(LibRpBase::Achievements::ID id, bool gray = false) const;
};

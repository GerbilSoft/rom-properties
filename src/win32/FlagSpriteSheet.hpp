/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * FlagSpriteSheet.hpp: Flag sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

#include "ISpriteSheet.hpp"
class FlagSpriteSheet : public ISpriteSheet {
public:
	/**
	 * Flags sprite sheet
	 * @param iconSize Icon size
	 * @param flipH If true, flip horizontally for RTL.
	 */
	FlagSpriteSheet(int iconSize, bool flipH = false);

private:
	typedef ISpriteSheet super;
	RP_DISABLE_COPY(FlagSpriteSheet)

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
	* Get a flag icon.
	* @param lc		[in]  Language code
	* @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
	* @param dpi		[in,opt] DPI value to set in the HBITMAP
	* @return Flag icon, or nullptr on error. (caller must free the icon)
	*/
	HBITMAP getIcon(uint32_t lc, bool forcePAL = false, UINT dpi = 96) const;
};

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.hpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiUPackage)
ROMDATA_DECL_CTOR_DIRECTORY(WiiUPackage)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_IMGEXT()

private:
	/**
	 * Internal initialization function for the two constructors.
	 */
	void init(void);

ROMDATA_DECL_END()

}

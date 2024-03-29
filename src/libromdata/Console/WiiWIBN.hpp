/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWIBN.hpp: Nintendo Wii save file banner reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiWIBN)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()

public:
	/**
	 * Is the NoCopy flag set?
	 * @return True if set; false if not.
	 */
	bool isNoCopyFlagSet(void) const;

ROMDATA_DECL_END()

}

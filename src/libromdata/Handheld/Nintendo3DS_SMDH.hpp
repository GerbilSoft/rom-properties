/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS_SMDH.hpp: Nintendo 3DS SMDH reader.                         *
 * Handles SMDH files and SMDH sections.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

class Nintendo3DS_SMDH_Private;
ROMDATA_DECL_BEGIN(Nintendo3DS_SMDH)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

public:
	/** Special SMDH accessor functions **/

	/**
	 * Get the SMDH region code.
	 * @return SMDH region code, or 0 on error.
	 */
	uint32_t getRegionCode(void) const;

ROMDATA_DECL_END()

}

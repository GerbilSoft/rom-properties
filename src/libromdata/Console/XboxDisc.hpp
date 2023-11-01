/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxDisc.hpp: Microsoft Xbox disc image parser.                         *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "../iso_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(XboxDisc)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()

public:
	/**
	 * Is a ROM image supported by this class?
	 * @param pvd ISO-9660 Primary Volume Descriptor.
	 * @param pWave If non-zero, receives the wave number. (0 if none; non-zero otherwise.)
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static int isRomSupported_static(const ISO_Primary_Volume_Descriptor *pvd, uint8_t *pWave);

	/**
	 * Is a ROM image supported by this class?
	 * @param pvd ISO-9660 Primary Volume Descriptor.
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static inline int isRomSupported_static(const ISO_Primary_Volume_Descriptor *pvd)
	{
		return isRomSupported_static(pvd, nullptr);
	}

ROMDATA_DECL_END()

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSP.hpp: PlayStation Portable disc image reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "../iso_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(PSP)
ROMDATA_DECL_CLOSE()

public:
	/**
	 * Is a ROM image supported by this class?
	 * @param pvd ISO-9660 Primary Volume Descriptor.
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static int isRomSupported_static(const ISO_Primary_Volume_Descriptor *pvd);

ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

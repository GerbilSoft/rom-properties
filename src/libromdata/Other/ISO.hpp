/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ISO.hpp: ISO-9660 disc image parser.                                    *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

extern "C" struct _ISO_Primary_Volume_Descriptor;

namespace LibRpBase {
	class RomMetaData;
}

namespace LibRomData {

ROMDATA_DECL_BEGIN(ISO)

public:
	/**
	 * Check for a valid PVD.
	 * @param data Potential PVD. (Must be 2048 bytes)
	 * @return DiscType if valid; -1 if not.
	 */
	static int checkPVD(const uint8_t *data);

public:
	/**
	 * Add metadata properties from an ISO-9660 PVD.
	 * Convenience function for other classes.
	 * @param metaData RomMetaData object.
	 * @param pvd ISO-9660 PVD.
	 */
	static void addMetaData_PVD(LibRpBase::RomMetaData *metaData, const struct _ISO_Primary_Volume_Descriptor *pvd);

ROMDATA_DECL_METADATA()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()
ROMDATA_DECL_END()

typedef std::shared_ptr<ISO> ISOPtr;

}

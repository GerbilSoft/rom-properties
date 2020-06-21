/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationDisc.hpp: PlayStation 1 and 2 disc image reader.             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_PLAYSTATIONDISC_HPP__
#define __ROMPROPERTIES_LIBROMDATA_PLAYSTATIONDISC_HPP__

#include "librpbase/RomData.hpp"
#include "../iso_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(PlayStationDisc)
ROMDATA_DECL_CLOSE()

	public:
		/**
		 * Is a ROM image supported by this class?
		 * @param pvd ISO-9660 Primary Volume Descriptor.
		 * @return Class-specific system ID (>= 0) if supported; -1 if not.
		 */
		static int isRomSupported_static(
			const ISO_Primary_Volume_Descriptor *pvd);

ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_PLAYSTATIONDISC_HPP__ */

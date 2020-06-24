/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ISO.hpp: ISO-9660 disc image parser.                                    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_OTHER_ISO_HPP__
#define __ROMPROPERTIES_LIBROMDATA_OTHER_ISO_HPP__

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(ISO)

	public:
		/**
		* Check for a valid PVD.
		* @param data Potential PVD. (Must be 2048 bytes)
		* @return DiscType if valid; -1 if not.
		*/
		static int checkPVD(const uint8_t *data);

ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_OTHER_ISO_HPP__ */

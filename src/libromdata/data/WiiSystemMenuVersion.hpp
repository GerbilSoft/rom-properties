/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.hpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__

#include "librpbase/common.h"

namespace LibRomData {

class WiiSystemMenuVersion
{
	private:
		WiiSystemMenuVersion();
		~WiiSystemMenuVersion();
		RP_DISABLE_COPY(WiiSystemMenuVersion)

	public:
		/**
		 * Look up a Wii System Menu version.
		 * @param version Version number.
		 * @return Display version, or nullptr if not found.
		 */
		static const char *lookup(unsigned int version);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__ */

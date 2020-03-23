/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.hpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/
 
#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__

#include "common.h"

namespace LibRomData {

class NESMappers
{
	private:
		NESMappers();
		~NESMappers();
	private:
		RP_DISABLE_COPY(NESMappers)

	public:
		/**
		 * Look up an iNES mapper number.
		 * @param mapper Mapper number.
		 * @return Mapper name, or nullptr if not found.
		 */
		static const char *lookup_ines(int mapper);

		/**
		 * Convert a TNES mapper number to iNES.
		 * @param tnes_mapper TNES mapper number.
		 * @return iNES mapper number, or -1 if unknown.
		 */
		static int tnesMapperToInesMapper(int tnes_mapper);

		/**
		 * Look up an NES 2.0 submapper number.
		 * TODO: Return the "depcrecated" value?
		 * @param mapper Mapper number.
		 * @param submapper Submapper number.
		 * @return Submapper name, or nullptr if not found.
		 */
		static const char *lookup_nes2_submapper(int mapper, int submapper);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_NESMAPPERS_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.hpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/
 
#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__

#include "librpbase/common.h"

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

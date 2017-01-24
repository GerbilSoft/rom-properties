/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.hpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/
 
#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_NESMAPPERS_HPP__

#include "config.libromdata.h"
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
		static const rp_char *lookup_ines(int mapper);

		/**
		 * Convert a TNES mapper number to iNES.
		 * @param tnes_mapper TNES mapper number.
		 * @return iNES mapper number, or -1 if unknown.
		 */
		static int tnesMapperToInesMapper(int tnes_mapper);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_NESMAPPERS_HPP__ */

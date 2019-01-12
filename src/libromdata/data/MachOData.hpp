/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachOData.hpp: Mach-O executable format data.                           *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_MACHODATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_MACHODATA_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class MachOData
{
	private:
		// Static class.
		MachOData();
		~MachOData();
		RP_DISABLE_COPY(MachOData)

	public:
		/**
		 * Look up a Mach-O CPU type.
		 * @param cputype Mach-O CPU type.
		 * @return CPU type name, or nullptr if not found.
		 */
		static const char *lookup_cpu_type(uint32_t cputype);

		/**
		 * Look up a Mach-O CPU subtype.
		 * @param cputype Mach-O CPU type.
		 * @param cpusubtype Mach-O CPU subtype.
		 * @return OS ABI name, or nullptr if not found.
		 */
		static const char *lookup_cpu_subtype(uint32_t cputype, uint32_t cpusubtype);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DATA_MACHODATA_HPP__ */

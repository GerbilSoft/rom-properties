/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.hpp: Executable and Linkable Format data.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_ELFDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_ELFDATA_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class ELFData
{
	private:
		// Static class.
		ELFData();
		~ELFData();
		RP_DISABLE_COPY(ELFData)

	public:
		/**
		 * Look up an ELF machine type. (CPU)
		 * @param cpu ELF machine type.
		 * @return Machine type name, or nullptr if not found.
		 */
		static const char *lookup_cpu(uint16_t cpu);

		/**
		 * Look up an ELF OS ABI.
		 * @param cpu ELF OS ABI.
		 * @return OS ABI name, or nullptr if not found.
		 */
		static const char *lookup_osabi(uint8_t osabi);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ELFDATA_HPP__ */

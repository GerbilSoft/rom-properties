/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXEData.hpp: DOS/Windows executable data.                               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_EXEDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_EXEDATA_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class EXEData
{
	private:
		// Static class.
		EXEData();
		~EXEData();
		RP_DISABLE_COPY(EXEData)

	public:
		/**
		 * Look up a PE machine type. (CPU)
		 * @param cpu PE machine type.
		 * @return Machine type name, or nullptr if not found.
		 */
		static const char *lookup_pe_cpu(uint16_t cpu);

		/**
		 * Look up an LE machine type. (CPU)
		 * @param cpu LE machine type.
		 * @return Machine type name, or nullptr if not found.
		 */
		static const char *lookup_le_cpu(uint16_t cpu);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_EXEDATA_HPP__ */

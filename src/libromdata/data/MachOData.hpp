/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachOData.hpp: Mach-O executable format data.                           *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_MACHODATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_MACHODATA_HPP__

#include "common.h"

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

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELFData.hpp: Executable and Linkable Format data.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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
		 * @param osabi ELF OS ABI.
		 * @return OS ABI name, or nullptr if not found.
		 */
		static const char *lookup_osabi(uint8_t osabi);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_ELFDATA_HPP__ */

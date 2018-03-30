/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.hpp: Nintendo 3DS firmware data.                    *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSFIRMDATA_HPP__
#define __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSFIRMDATA_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class Nintendo3DSFirmData
{
	private:
		// Static class.
		Nintendo3DSFirmData();
		~Nintendo3DSFirmData();
		RP_DISABLE_COPY(Nintendo3DSFirmData)

	public:
		struct FirmBin_t {
			uint32_t crc;		// FIRM CRC32.
			struct {		// Kernel version.
				uint8_t major;
				uint8_t minor;
				uint8_t revision;
			} kernel;
			struct {		// System version.
				uint8_t major;
				uint8_t minor;
			} sys;
			bool isNew3DS;		// Is this New3DS?
		};

		/**
		 * Look up a Nintendo 3DS firmware binary.
		 * @param Firmware binary CRC32.
		 * @return Firmware binary data, or nullptr if not found.
		 */
		static const FirmBin_t *lookup_firmBin(uint32_t crc);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_NINTENDO3DSFIRMDATA_HPP__ */

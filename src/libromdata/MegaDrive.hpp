/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.hpp: Sega Mega Drive ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__

#include <stdint.h>
#include <string>
#include "TextFuncs.hpp"

#include "RomData.hpp"

namespace LibRomData {

class MegaDrive : public RomData
{
	public:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * Read a Sega Mega Drive ROM.
		 *
		 * A ROM file must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the ROM.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * @param file Open ROM file.
		 */
		MegaDrive(FILE *file);

	private:
		MegaDrive(const MegaDrive &);
		MegaDrive &operator=(const MegaDrive &);

	public:
		/**
		 * Detect if a ROM is supported by this class.
		 * TODO: Actually detect the type; for now, just returns true if it's supported.
		 * @param header Header data.
		 * @param size Size of header.
		 * @return True if the ROM is supported; false if it isn't.
		 */
		static bool isRomSupported(const uint8_t *header, size_t size);

	protected:
		/**
		 * Load field data.
		 * Called by RomData::fields() if the field data hasn't been loaded yet.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int loadFieldData(void) override;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__ */

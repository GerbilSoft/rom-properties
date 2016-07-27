/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataFactory.hpp: RomData factory class.                              *
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

#include "RomDataFactory.hpp"

// RomData subclasses.
#include "MegaDrive.hpp"
#include "GameCube.hpp"
#include "NintendoDS.hpp"

namespace LibRomData {

/**
 * Create a RomData class for the specified ROM file.
 *
 * NOTE: RomData::isValid() is checked before returning a
 * created RomData instance, so returned objects can be
 * assumed to be valid as long as they aren't nullptr.
 *
 * @param file ROM file.
 * @return RomData class, or nullptr if the ROM isn't supported.
 */
RomData *RomDataFactory::getInstance(FILE *file)
{
	// Read 4,096 bytes from the ROM header.
	// This should be enough to detect most systems.
	uint8_t header[4096];
	rewind(file);
	fflush(file);
	size_t count = fread(header, 1, sizeof(header), file);

	// Try to detect the ROM format.

#define CheckRomData(sys) \
	do { \
		if (sys::isRomSupported(header, sizeof(header)) > 0) { \
			RomData *romData = new sys(file); \
			if (romData->isValid()) \
				return romData; \
			\
			/* Not actually supported. */ \
			delete romData; \
		} \
	} while (0)

	CheckRomData(MegaDrive);
	CheckRomData(GameCube);
	CheckRomData(NintendoDS);

	// Not supported.
	return nullptr;
}

}

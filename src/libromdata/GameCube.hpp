/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCube.hpp: Nintendo GameCube and Wii disc image reader.              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__

#include <stdint.h>
#include <string>
#include "TextFuncs.hpp"

#include "RomData.hpp"

namespace LibRomData {

class IDiscReader;
class GameCube : public RomData
{
	public:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * Read a Nintendo GameCube or Wii disc image.
		 *
		 * A disc image must be opened by the caller. The file handle
		 * will be dup()'d and must be kept open in order to load
		 * data from the disc image.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * @param file Open disc image.
		 */
		GameCube(FILE *file);

	private:
		GameCube(const GameCube &);
		GameCube &operator=(const GameCube &);

	public:
		enum DiscType {
			DISC_UNKNOWN = 0,	// Unknown disc type.
			DISC_GCN,		// GameCube disc image.
			DISC_WII,		// Wii disc image.
			DISC_WII_WBFS,		// Wii disc image, in WBFS format.
		};

		/**
		 * Detect if a disc image is supported by this class.
		 * @param header Header data.
		 * @param size Size of header.
		 * @return DiscType if the disc image is supported; 0 if it isn't.
		 */
		static DiscType isRomSupported(const uint8_t *header, size_t size);

	private:
		// Disc type.
		DiscType m_discType;

		// Disc reader object.
		IDiscReader *m_discReader;

		/**
		 * Wii partition tables.
		 * Decoded from the actual on-disc tables.
		 */
		struct WiiPartEntry {
			uint64_t start;		// Starting address, in bytes.
			//uint64_t length;	// Length, in bytes. [TODO: Calculate this]
			uint32_t type;		// Partition type. (0 == Game, 1 == Update, 2 == Channel)
		};

		typedef std::vector<WiiPartEntry> WiiPartTable;
		WiiPartTable m_wiiMpt[4];
		bool m_wiiMptLoaded;

		/**
		 * Load the Wii partition tables.
		 * Partition tables are loaded into m_wiiMpt[].
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadWiiPartitionTables(void);

	protected:
		/**
		* Load field data.
		* Called by RomData::fields() if the field data hasn't been loaded yet.
		* @return 0 on success; negative POSIX error code on error.
		*/
		virtual int loadFieldData(void) override;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_GAMECUBE_HPP__ */

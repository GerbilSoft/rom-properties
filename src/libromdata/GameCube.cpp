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

#include "GameCube.hpp"
#include "NintendoPublishers.hpp"
#include "TextFuncs.hpp"
#include "byteswap.h"

// TODO: Move this elsewhere.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

namespace LibRomData {

// ROM fields.
// TODO: Private class?
static const struct RomFields::Desc gcn_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, nullptr},
	{_RP("Game ID"), RomFields::RFT_STRING, nullptr},
	{_RP("Publisher"), RomFields::RFT_STRING, nullptr},
	{_RP("Disc #"), RomFields::RFT_STRING, nullptr},
	{_RP("Revision"), RomFields::RFT_STRING, nullptr},

	// TODO:
	// - Partition table.
	// - System update version.
	// - Region and age ratings.
};

/**
 * GameCube/Wii disc image header.
 * This matches the disc image format exactly.
 * 
 * NOTE: Strings are NOT null-terminated!
 */
struct GCN_DiscHeader {
	union {
		char id6[6];	// Game code. (ID6)
		struct {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};

	uint8_t disc_number;		// Disc number.
	uint8_t revision;		// Revision.
	uint8_t audio_streaming;	// Audio streaming flag.
	uint8_t stream_buffer_size;	// Streaming buffer size.

	uint8_t reserved1[14];

	uint32_t magic_wii;		// Wii magic. (0x5D1C9EA3)
	uint32_t magic_gcn;		// GameCube magic. (0xC2339F3D)

	char game_title[64];		// Game title.
};

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
GameCube::GameCube(FILE *file)
	: RomData(file, gcn_fields, ARRAY_SIZE(gcn_fields))
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the disc header.
	// TODO: WBFS support.
	GCN_DiscHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	m_isValid = isRomSupported(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
}

/**
 * Detect if a disc image is supported by this class.
 * @param header Header data.
 * @param size Size of header.
 * @return DiscType if the disc image is supported; 0 if it isn't.
 */
GameCube::DiscType GameCube::isRomSupported(const uint8_t *header, size_t size)
{
	// TODO: WBFS support.

	if (size >= sizeof(GCN_DiscHeader)) {
		// Check for the magic numbers.
		const GCN_DiscHeader *gcn_header = reinterpret_cast<const GCN_DiscHeader*>(header);
		if (gcn_header->magic_wii == cpu_to_be32(0x5D1C9EA3)) {
			// Wii disc image.
			return DISC_WII;
		} else if (gcn_header->magic_gcn == cpu_to_be32(0xC2339F3D)) {
			// GameCube disc image.
			return DISC_GCN;
		}
	}

	// Not supported.
	return DISC_UNKNOWN;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCube::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	}
	if (!m_file) {
		// File isn't open.
		return -EBADF;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the disc header.
	// TODO: WBFS support.
	GCN_DiscHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header)) {
		// File isn't big enough for a GameCube/Wii header...
		return -EIO;
	}

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	m_fields->addData_string(cp1252_sjis_to_rp_string(header.game_title, sizeof(header.game_title)));

	// Game ID and publisher.
	m_fields->addData_string(ascii_to_rp_string(header.id6, sizeof(header.id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(header.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// Other fields.
	m_fields->addData_string_numeric(header.disc_number+1, RomFields::FB_DEC);
	m_fields->addData_string_numeric(header.revision, RomFields::FB_DEC, 2);

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

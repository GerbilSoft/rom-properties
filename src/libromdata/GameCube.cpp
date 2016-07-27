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

// C includes. (C++ namespace)
#include <cctype>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

// TODO: Move this elsewhere.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

namespace LibRomData {

// Wii partition table.
static const rp_char *rvl_partitions_names[] = {
	_RP("#"), _RP("Type"),
	// TODO: Start/End addresses?
};

static const struct RomFields::ListDataDesc rvl_partitions = {
	ARRAY_SIZE(rvl_partitions_names), rvl_partitions_names
};

// ROM fields.
// TODO: Private class?
static const struct RomFields::Desc gcn_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, nullptr},
	{_RP("Game ID"), RomFields::RFT_STRING, nullptr},
	{_RP("Publisher"), RomFields::RFT_STRING, nullptr},
	{_RP("Disc #"), RomFields::RFT_STRING, nullptr},
	{_RP("Revision"), RomFields::RFT_STRING, nullptr},

	// Wii partition table.
	// NOTE: Actually a table of tables, so we'll use
	// 0p0-style numbering, where the first digit is
	// the table number, and the second digit is the
	// partition number. (both start at 0)
	{_RP("Partitions"), RomFields::RFT_LISTDATA, &rvl_partitions},

	// TODO:
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
 * Wii master partition table.
 * Contains information about the (maximum of) four partition tables.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
 *
 * All fields are big-endian.
 */
struct RVL_MasterPartitionTable {
	struct {
		uint32_t count;		// Number of partitions in this table.
		uint32_t addr;		// Start address of this table, rshifted by 2.
	} table[4];
};

/**
 * Wii partition table entry.
 * Contains information about an individual partition.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
 *
 * All fields are big-endian.
 */
struct RVL_PartitionTableEntry {
	uint32_t addr;	// Start address of this partition, rshifted by 2.
	uint32_t type;	// Type of partition. (0 == Game, 1 == Update, 2 == Channel Installer, other = title ID)
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

	// Get the disc type.
	DiscType discType = isRomSupported(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
	if (discType == DISC_UNKNOWN) {
		// Unknown disc type...
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

	// Partition table. (Wii only)
	// TODO: WBFS.
	if (discType == DISC_WII) {
		RomFields::ListData *partitions = new RomFields::ListData();

		// Assuming a maximum of 128 partitions per table.
		// (This is a rather high estimate.)
		RVL_MasterPartitionTable mpt;
		RVL_PartitionTableEntry pt[1024];

		// Read the master partition table.
		// Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
		fseek(m_file, 0x40000, SEEK_SET);
		size = fread(&mpt, 1, sizeof(mpt), m_file);
		if (size == sizeof(mpt)) {
			// Process each partition table.
			vector<rp_string> data_row;	// Temporary storage for each partition entry.

			for (int i = 0; i < 4; i++) {
				uint32_t count = be32_to_cpu(mpt.table[i].count);
				if (count == 0) {
					continue;
				} else if (count > ARRAY_SIZE(pt)) {
					count = ARRAY_SIZE(pt);
				}

				// Read the individual partition table.
				// TODO: 64-bit fseek()?
				fseek(m_file, (be32_to_cpu(mpt.table[i].addr) << 2), SEEK_SET);
				size = fread(pt, sizeof(RVL_PartitionTableEntry), count, m_file);
				if (size == count) {
					// Process each partition table entry.
					partitions->data.reserve(partitions->data.size() + count);
					data_row.reserve(ARRAY_SIZE(rvl_partitions_names));
					for (int j = 0; j < (int)count; j++) {
						data_row.clear();

						// Partition number.
						char buf[16];
						int len = snprintf(buf, sizeof(buf), "%dp%d", i, j);
						if (len > (int)sizeof(buf))
							len = sizeof(buf);
						printf("buf = %s, len = %d\n", buf, len);
						data_row.push_back(len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));

						// Partition type.
						uint32_t type = be32_to_cpu(pt[j].type);
						rp_string str;
						switch (type) {
							case 0:
								str = _RP("Game");
								break;
							case 1:
								str = _RP("Update");
								break;
							case 2:
								str = _RP("Channel");
								break;
							default:
								// If all four bytes are ASCII,
								// print it as-is. (SSBB demo channel)
								// Otherwise, print the number.
								memcpy(buf, &pt[j].type, 4);
								if (isalnum(buf[0]) && isalnum(buf[1]) &&
								    isalnum(buf[2]) && isalnum(buf[3]))
								{
									// All four bytes are ASCII.
									str = ascii_to_rp_string(buf, 4);
								} else {
									// Non-ASCII data. Print the hex values instead.
									len = snprintf(buf, sizeof(buf), "%08X", be32_to_cpu(pt[j].type));
									if (len > (int)sizeof(buf))
										len = sizeof(buf);
									str = (len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));
								}
						}
						data_row.push_back(str);

						// Add the partition information to the ListData.
						partitions->data.push_back(data_row);
					}
				}
			}
		}

		// Add the partitions list data.
		m_fields->addData_listData(partitions);
	} else {
		// Add a dummy entry.
		m_fields->addData_string(nullptr);
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

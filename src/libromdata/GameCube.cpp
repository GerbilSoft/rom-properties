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

// DiscReader
#include "DiscReader.hpp"

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
	, m_discType(DISC_UNKNOWN)
	, m_discReader(nullptr)
	, m_wiiMptLoaded(false)
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
	uint8_t header[512];
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header))
		return;

	// Check if this disc image is supported.
	m_discType = isRomSupported(header, sizeof(header));
	// TODO: DiscReaderFactory?
	switch (m_discType) {
		case DISC_GCN:
		case DISC_WII:
			m_discReader = new DiscReader(m_file);
			break;
		default:
			m_discType = DISC_UNKNOWN;
			break;
	}

	m_isValid = (m_discType != DISC_UNKNOWN);
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
 * Load the Wii partition tables.
 * Partition tables are loaded into m_wiiMpt[].
 * @return 0 on success; negative POSIX error code on error.
 */
int GameCube::loadWiiPartitionTables(void)
{
	if (m_wiiMptLoaded) {
		// Partition tables have already been loaded.
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (m_discType != DISC_WII) {
		// Unsupported disc type.
		// TODO: DISC_WII_WBFS
		return -EIO;
	}

	// Clear the existing partition tables.
	for (int i = ARRAY_SIZE(m_wiiMpt)-1; i >= 0; i--) {
		m_wiiMpt[i].clear();
	}

	// Assuming a maximum of 128 partitions per table.
	// (This is a rather high estimate.)
	RVL_MasterPartitionTable mpt;
	RVL_PartitionTableEntry pt[1024];

	// Read the master partition table.
	// Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
	m_discReader->seek(0x40000);
	size_t size = m_discReader->read(&mpt, sizeof(mpt));
	if (size != sizeof(mpt)) {
		// Could not read the master partition table.
		// TODO: Return error from fread()?
		return -EIO;
	}

	// Get the size of the disc image.
	// TODO: Large File Support for 32-bit Linux and Windows.
	int64_t discSize = m_discReader->fileSize();
	if (discSize < 0) {
		// Error getting the size of the disc image.
		return -errno;
	}

	// Process each partition table.
	for (int i = 0; i < 4; i++) {
		uint32_t count = be32_to_cpu(mpt.table[i].count);
		if (count == 0) {
			continue;
		} else if (count > ARRAY_SIZE(pt)) {
			count = ARRAY_SIZE(pt);
		}

		// Read the individual partition table.
		uint64_t pt_addr = (uint64_t)(be32_to_cpu(mpt.table[i].addr)) << 2;
		const size_t ptSize = sizeof(RVL_PartitionTableEntry) * count;
		m_discReader->seek((int64_t)pt_addr);
		size = m_discReader->read(pt, ptSize);
		if (size != ptSize) {
			// Error reading the partition table entries.
			return -EIO;
		}

		// Process each partition table entry.
		m_wiiMpt[i].resize(count);
		for (int j = 0; j < (int)count; j++) {
			WiiPartEntry &entry = m_wiiMpt[i].at(j);
			entry.start = (uint64_t)(be32_to_cpu(pt[j].addr)) << 2;
			// TODO: Figure out how to calculate length?
			entry.type = be32_to_cpu(pt[j].type);
		}
	}

	// Done reading the partition tables.
	return 0;
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
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (m_discType == DISC_UNKNOWN) {
		// Unknown disc type.
		return -EIO;
	}

	// Seek to the beginning of the file.
	m_discReader->rewind();

	// Read the disc header.
	// TODO: WBFS support.
	GCN_DiscHeader header;
	size_t size = m_discReader->read(&header, sizeof(header));
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

	// Partition table. (Wii only)
	// TODO: WBFS.
	if (m_discType == DISC_WII) {
		RomFields::ListData *partitions = new RomFields::ListData();

		// Load the Wii partition tables.
		int ret = loadWiiPartitionTables();
		if (ret == 0) {
			// Wii partition tables loaded.
			// Convert them to RFT_LISTDATA for display purposes.
			vector<rp_string> data_row;     // Temporary storage for each partition entry.
			for (int i = 0; i < 4; i++) {
				const int count = (int)m_wiiMpt[i].size();
				for (int j = 0; j < count; j++) {
					const WiiPartEntry &entry = m_wiiMpt[i].at(j);
					data_row.clear();

					// Partition number.
					char buf[16];
					int len = snprintf(buf, sizeof(buf), "%dp%d", i, j);
					if (len > (int)sizeof(buf))
						len = sizeof(buf);
					data_row.push_back(len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));

					// Partition type.
					rp_string str;
					switch (entry.type) {
						case 0:
							str = _RP("Game");
							break;
						case 1:
							str = _RP("Update");
							break;
						case 2:
							str = _RP("Channel");
							break;
						default: {
							// If all four bytes are ASCII,
							// print it as-is. (SSBB demo channel)
							// Otherwise, print the number.
							// NOTE: Must be BE32 for proper display.
							uint32_t be32_type = cpu_to_be32(entry.type);
							memcpy(buf, &be32_type, 4);
							if (isalnum(buf[0]) && isalnum(buf[1]) &&
							    isalnum(buf[2]) && isalnum(buf[3]))
							{
								// All four bytes are ASCII.
								str = ascii_to_rp_string(buf, 4);
							} else {
								// Non-ASCII data. Print the hex values instead.
								len = snprintf(buf, sizeof(buf), "%08X", entry.type);
								if (len > (int)sizeof(buf))
									len = sizeof(buf);
								str = (len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));
							}
 						}
					}
					data_row.push_back(str);

					// Add the partition information to the ListData.
					partitions->data.push_back(data_row);
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

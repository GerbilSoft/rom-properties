/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gcn_structs.h: Nintendo GameCube and Wii data structures.               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GameCube/Wii disc image header.
 * This matches the disc image format exactly.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
typedef struct PACKED _GCN_DiscHeader {
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
} GCN_DiscHeader;
#pragma pack()

/**
 * Wii master partition table.
 * Contains information about the (maximum of) four partition tables.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct PACKED _RVL_MasterPartitionTable {
	struct {
		uint32_t count;		// Number of partitions in this table.
		uint32_t addr;		// Start address of this table, rshifted by 2.
	} table[4];
} RVL_MasterPartitionTable;
#pragma pack()

/**
 * Wii partition table entry.
 * Contains information about an individual partition.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partitions_information
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct PACKED _RVL_PartitionTableEntry {
	uint32_t addr;	// Start address of this partition, rshifted by 2.
	uint32_t type;	// Type of partition. (0 == Game, 1 == Update, 2 == Channel Installer, other = title ID)
} RVL_PartitionTableEntry;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__ */

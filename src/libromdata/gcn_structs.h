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
#define GCN_MAGIC 0xC2339F3D
#define WII_MAGIC 0x5D1C9EA3
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
ASSERT_STRUCT(GCN_DiscHeader, 96);

/**
 * GameCube region codes.
 * Used in bi2.bin (GameCube) and RVL_RegionSetting.
 */
enum GCN_Region_Code {
	GCN_REGION_JAPAN = 0,		// Japan / Taiwan
	GCN_REGION_USA = 1,		// USA
	GCN_REGION_PAL = 2,		// Europe / Australia
	GCN_REGION_SOUTH_KOREA = 4,	// South Korea
};

/**
 * DVD Boot Block.
 * References:
 * - http://wiibrew.org/wiki/Wii_Disc#Decrypted
 * - http://hitmen.c02.at/files/yagcd/yagcd/chap13.html
 * - http://www.gc-forever.com/wiki/index.php?title=Apploader
 *
 * All fields are big-endian.
 */
#pragma pack(1)
#define GCN_Boot_Block_ADDRESS 0x420
typedef struct PACKED _GCN_Boot_Block {
	uint32_t dol_offset;	// NOTE: 34-bit RSH2 on Wii.
	uint32_t fst_offset;	// NOTE: 34-bit RSH2 on Wii.
	uint32_t fst_size;	// FST size. (NOTE: 34-bit RSH2 on Wii.)
	uint32_t fst_max_size;	// Size of biggest additional FST.

	uint32_t fst_mem_addr;	// FST address in RAM.
	uint32_t user_pos;	// Data area start. (Might be wrong; use FST.)
	uint32_t user_len;	// Data area length. (Might be wrong; use FST.)
	uint32_t reserved;
} GCN_Boot_Block;
#pragma pack()
ASSERT_STRUCT(GCN_Boot_Block, 32);

/**
 * DVD Boot Info. (bi2.bin)
 * Reference: http://www.gc-forever.com/wiki/index.php?title=Apploader
 *
 * All fields are big-endian.
 */
#pragma pack(1)
#define GCN_Boot_Info_ADDRESS 0x440
typedef struct PACKED _GCN_Boot_Info {
	uint32_t debug_mon_size;	// Debug monitor size. [FIXME: Listed as signed?]
	uint32_t sim_mem_size;		// Simulated memory size. (bytes) [FIXME: Listed as signed?]
	uint32_t arg_offset;		// Command line arguments.
	uint32_t debug_flag;		// Debug flag. (set to 3 if using CodeWarrior on GDEV)
	uint32_t trk_location;		// Target resident kernel location.
	uint32_t trk_size;		// Size of TRK. [FIXME: Listed as signed?]
	uint32_t region_code;		// Region code. (See GCN_Region_Code.)
	uint32_t reserved1[3];
	uint32_t dol_limit;		// Maximum total size of DOL text/data sections. (0 == unlimited)
	uint32_t reserved2;
} GCN_Boot_Info;
#pragma pack()
ASSERT_STRUCT(GCN_Boot_Info, 48);

/**
 * FST entry.
 * All fields are big-endian.
 *
 * Reference: http://hitmen.c02.at/files/yagcd/yagcd/index.html#idx13.4
 */
#pragma pack(1)
typedef struct PACKED _GCN_FST_Entry {
	uint32_t file_type_name_offset;	// MSB = type; low 24 bits = name offset
	union {
		struct {
			uint32_t unused;		// Unused.
			uint32_t file_count;		// File count.
		} root_dir;
		struct {
			uint32_t parent_dir_idx;	// Parent directory index.
			uint32_t next_offset;		// Index of the next entry in the current directory.
		} dir;
		struct {
			uint32_t offset;		// File offset. (<< 2 for Wii)
			uint32_t size;			// File size.
		} file;
	};
} GCN_FST_Entry;
#pragma pack()
ASSERT_STRUCT(GCN_FST_Entry, 12);

/**
 * TGC header.
 * Used on some GameCube demo discs.
 * Reference: http://hitmen.c02.at/files/yagcd/yagcd/index.html#idx14.8
 * TODO: Check Dolphin?
 *
 * All fields are big-endian.
 */
#pragma pack(1)
#define TGC_MAGIC 0xAE0F38A2
typedef struct PACKED _GCN_TGC_Header {
	uint32_t tgc_magic;	// TGC magic.
	uint32_t reserved1;	// Unknown (usually 0x00000000)
	uint32_t header_size;	// Header size. (usually 0x8000)
	uint32_t reserved2;	// Unknown (usually 0x00100000)
	uint32_t fst_offset;	// Offset to FST inside the embedded GCM.
	uint32_t fst_size;	// FST size.
	uint32_t fst_max_size;	// Size of biggest additional FST.
	uint32_t dol_offset;	// Offset to main.dol inside the embedded GCM.
	uint32_t dol_size;	// main.dol size.
	uint32_t reserved3[2];
	uint32_t banner_offset;	// Offset to opening.bnr inside the embedded GCM.
	uint32_t banner_size;	// opening.bnr size.
	uint32_t reserved4[3];
} GCN_TGC_Header;
#pragma pack()
ASSERT_STRUCT(GCN_TGC_Header, 64);

/** Wii-specific structs. **/

// 34-bit value stored in uint32_t.
// The value must be lshifted by 2.
typedef uint32_t uint34_rshift2_t;

/**
 * Wii volume group table.
 * Contains information about the (maximum of) four volume groups.
 * References:
 * - http://wiibrew.org/wiki/Wii_Disc#Partitions_information
 * - http://blog.delroth.net/2011/06/reading-wii-discs-with-python/
 *
 * All fields are big-endian.
 */
#pragma pack(1)
#define RVL_VolumeGroupTable_ADDRESS 0x40000
typedef struct PACKED _RVL_VolumeGroupTable {
	struct {
		uint32_t count;		// Number of partitions in this volume group.
		uint34_rshift2_t addr;	// Start address of this table, rshifted by 2.
	} vg[4];
} RVL_VolumeGroupTable;
#pragma pack()

/**
 * Wii partition table entry.
 * Contains information about an individual partition.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partition_table_entry
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct PACKED _RVL_PartitionTableEntry {
	uint34_rshift2_t addr;	// Start address of this partition, rshifted by 2.
	uint32_t type;		// Type of partition. (0 == Game, 1 == Update, 2 == Channel Installer, other = title ID)
} RVL_PartitionTableEntry;
#pragma pack()

// Wii ticket constants.
#define RVL_SIGNATURE_TYPE_RSA2048 0x10001
#define RVL_COMMON_KEY_INDEX_DEFAULT 0
#define RVL_COMMON_KEY_INDEX_KOREAN 1

/**
 * Time limit structs for Wii ticket.
 * Reference: http://wiibrew.org/wiki/Ticket
 */
#pragma pack(1)
typedef struct PACKED _RVL_TimeLimit {
	uint32_t enable;	// 1 == enable; 0 == disable
	uint32_t seconds;	// Time limit, in seconds.
} RVL_TimeLimit;
#pragma pack()
ASSERT_STRUCT(RVL_TimeLimit, 8);

/**
 * Wii ticket.
 * Reference: http://wiibrew.org/wiki/Ticket
 */
#pragma pack(1)
typedef struct PACKED _RVL_Ticket {
	uint32_t signature_type;	// [0x000] Always 0x10001 for RSA-2048.
	uint8_t signature[0x100];	// [0x004] Signature.

	// The following fields are all covered by the above signature.
	uint8_t padding1[0x3C];		// [0x104] Padding. (always 0)
	char signature_issuer[0x40];	// [0x140] Signature issuer.
	uint8_t ecdh_data[0x3C];	// [0x180] ECDH data.
	uint8_t padding2[0x03];		// [0x1BC] Padding.
	uint8_t enc_title_key[0x10];	// [0x1BF] Encrypted title key.
	uint8_t unknown1;		// [0x1CF] Unknown.
	uint8_t ticket_id[0x08];	// [0x1D0] Ticket ID. (IV for title key decryption for console-specific titles.)
	uint8_t console_id[4];		// [0x1D8] Console ID.
	uint8_t title_id[8];		// [0x1DC] Title ID. (IV used for AES-CBC encryption.)
	uint8_t unknown2[2];		// [0x1E4] Unknown, mostly 0xFFFF.
	uint8_t ticket_version[2];	// [0x1E6] Ticket version.
	uint32_t permitted_titles_mask;	// [0x1E8] Permitted titles mask.
	uint32_t permit_mask;		// [0x1EC] Permit mask.
	uint8_t title_export;		// [0x1F0] Title Export allowed using PRNG key. (1 == yes, 0 == no)
	uint8_t common_key_index;	// [0x1F1] Common Key index. (0 == default, 1 == Korean)
	uint8_t unknown3[0x30];		// [0x1F2] Unknown. (VC related?)
	uint8_t content_access_perm[0x40];	// [0x222] Content access permissions. (1 bit per content)
	uint8_t padding3[2];		// [0x262] Padding. (always 0)
	RVL_TimeLimit time_limits[8];	// [0x264] Time limits.
} RVL_Ticket;
#pragma pack()
ASSERT_STRUCT(RVL_Ticket, 0x2A4);

/**
 * Wii partition header.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Partition
 */
#pragma pack(1)
typedef struct PACKED _RVL_PartitionHeader {
	RVL_Ticket ticket;			// [0x000]
	uint32_t tmd_size;			// [0x2A4] TMD size.
	uint34_rshift2_t tmd_offset;		// [0x2A8] TMD offset, rshifted by 2.
	uint32_t cert_chain_size;		// [0x2AC] Certificate chain size.
	uint34_rshift2_t cert_chain_offset;	// [0x2B0] Certificate chain offset, rshifted by 2.
	uint34_rshift2_t h3_table_offset;	// [0x2B4] H3 table offset, rshifted by 2. (Size is always 0x18000.)
	uint34_rshift2_t data_offset;		// [0x2B8] Data offset, rshifted by 2.
	uint34_rshift2_t data_size;		// [0x2BC] Data size, rshifted by 2.

	// 0x2C0
	uint8_t tmd[0x1FD40];			// TMD, variable length up to data_offset.
} RVL_PartitionHeader;
#pragma pack()
ASSERT_STRUCT(RVL_PartitionHeader, 0x20000);

/**
 * Country indexes in RVL_RegionSetting.ratings[].
 */
enum RVL_RegionSetting_RatingCountry {
	RATING_JAPAN = 0,	// CERO
	RATING_USA = 1,		// ESRB
	RATING_GERMANY = 3,	// USK
	RATING_PEGI = 4,	// PEGI
	RATING_FINLAND = 5,	// MEKU?
	RATING_PORTUGAL = 6,	// Modified PEGI
	RATING_BRITAIN = 7,	// BBFC
	RATING_AUSTRALIA = 8,	// AGCB
	RATING_SOUTH_KOREA = 9,	// GRB
};

/**
 * Region setting and age ratings.
 * Reference: http://wiibrew.org/wiki/Wii_Disc#Region_setting
 * TODO: Decode age ratings into e.g. ESRB.
 */
#pragma pack(1)
#define RVL_RegionSetting_ADDRESS 0x4E000
typedef struct PACKED _RVL_RegionSetting {
	uint32_t region_code;	// Region code. (See GCN_Region_Code.)
	uint8_t reserved[12];
	uint8_t ratings[0x10];	// Country-specific age ratings.
} RVL_RegionSetting;
#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__ */

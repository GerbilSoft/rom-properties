/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wii_structs.h: Nintendo Wii data structures.                            *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This file has Wii-specific structs only.
// For structs shared with GameCube, see gcn_structs.h.

#ifndef __ROMPROPERTIES_LIBROMDATA_WII_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_WII_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

// 34-bit value stored in uint32_t.
// The value must be lshifted by 2.
typedef uint32_t uint34_rshift2_t;

/**
 * Wii volume group table.
 * Contains information about the (maximum of) four volume groups.
 * References:
 * - https://wiibrew.org/wiki/Wii_Disc#Partitions_information
 * - http://blog.delroth.net/2011/06/reading-wii-discs-with-python/
 *
 * All fields are big-endian.
 */
#define RVL_VolumeGroupTable_ADDRESS 0x40000
typedef struct PACKED _RVL_VolumeGroupTable {
	struct {
		uint32_t count;		// Number of partitions in this volume group.
		uint34_rshift2_t addr;	// Start address of this table, rshifted by 2.
	} vg[4];
} RVL_VolumeGroupTable;

/**
 * Wii partition table entry.
 * Contains information about an individual partition.
 * Reference: https://wiibrew.org/wiki/Wii_Disc#Partition_table_entry
 *
 * All fields are big-endian.
 */
typedef struct PACKED _RVL_PartitionTableEntry {
	uint34_rshift2_t addr;	// Start address of this partition, rshifted by 2.
	uint32_t type;		// Type of partition. (0 == Game, 1 == Update, 2 == Channel Installer, other = title ID)
} RVL_PartitionTableEntry;

enum RVL_PartitionType {
	RVL_PT_GAME	= 0,
	RVL_PT_UPDATE	= 1,
	RVL_PT_CHANNEL	= 2,
};

// Wii ticket constants.
#define RVL_SIGNATURE_TYPE_RSA2048 0x10001
#define RVL_COMMON_KEY_INDEX_DEFAULT 0
#define RVL_COMMON_KEY_INDEX_KOREAN 1

/**
 * Time limit structs for Wii ticket.
 * Reference: https://wiibrew.org/wiki/Ticket
 */
typedef struct PACKED _RVL_TimeLimit {
	uint32_t enable;	// 1 == enable; 0 == disable
	uint32_t seconds;	// Time limit, in seconds.
} RVL_TimeLimit;
ASSERT_STRUCT(RVL_TimeLimit, 8);

/**
 * Title ID struct/union.
 * TODO: Verify operation on big-endian systems.
 */
typedef union PACKED _RVL_TitleID_t {
	uint64_t id;
	struct {
		uint32_t hi;
		uint32_t lo;
	};
	uint8_t u8[8];
} RVL_TitleID_t;
ASSERT_STRUCT(RVL_TitleID_t, 8);

/**
 * Wii ticket.
 * Reference: https://wiibrew.org/wiki/Ticket
 */
typedef struct PACKED _RVL_Ticket {
	uint32_t signature_type;	// [0x000] Always 0x10001 for RSA-2048.
	uint8_t signature[0x100];	// [0x004] Signature.
	uint8_t padding_sig[0x3C];	// [0x104] Padding. (always 0)

	// The following fields are all covered by the above signature.
	char signature_issuer[0x40];	// [0x140] Signature issuer.
	uint8_t ecdh_data[0x3C];	// [0x180] ECDH data.
	uint8_t padding1[0x03];		// [0x1BC] Padding.
	uint8_t enc_title_key[0x10];	// [0x1BF] Encrypted title key.
	uint8_t unknown1;		// [0x1CF] Unknown.
	uint8_t ticket_id[0x08];	// [0x1D0] Ticket ID. (IV for title key decryption for console-specific titles.)
	uint32_t console_id;		// [0x1D8] Console ID.
	RVL_TitleID_t title_id;		// [0x1DC] Title ID. (IV used for AES-CBC encryption.)
	uint8_t unknown2[2];		// [0x1E4] Unknown, mostly 0xFFFF.
	uint8_t ticket_version[2];	// [0x1E6] Ticket version.
	uint32_t permitted_titles_mask;	// [0x1E8] Permitted titles mask.
	uint32_t permit_mask;		// [0x1EC] Permit mask.
	uint8_t title_export;		// [0x1F0] Title Export allowed using PRNG key. (1 == yes, 0 == no)
	uint8_t common_key_index;	// [0x1F1] Common Key index. (0 == default, 1 == Korean)
	uint8_t unknown3[0x30];		// [0x1F2] Unknown. (VC related?)
	uint8_t content_access_perm[0x40];	// [0x222] Content access permissions. (1 bit per content)
	uint8_t padding2[2];		// [0x262] Padding. (always 0)
	RVL_TimeLimit time_limits[8];	// [0x264] Time limits.
} RVL_Ticket;
ASSERT_STRUCT(RVL_Ticket, 0x2A4);

/**
 * Wii TMD header.
 * Reference: https://wiibrew.org/wiki/Tmd_file_structure
 */
typedef struct PACKED _RVL_TMD_Header {
	uint32_t signature_type;	// [0x000] Always 0x10001 for RSA-2048.
	uint8_t signature[0x100];	// [0x004] Signature.
	uint8_t padding_sig[0x3C];	// [0x104] Padding. (always 0)

	// The following fields are all covered by the above signature.
	char signature_issuer[0x40];	// [0x140] Signature issuer.
	uint8_t version;		// [0x180] Version.
	uint8_t ca_crl_version;		// [0x181] CA CRL version.
	uint8_t signer_crl_version;	// [0x182] Signer CRL version.
	uint8_t padding1;		// [0x183]
	RVL_TitleID_t sys_version;	// [0x184] System version. (IOS title ID)
	RVL_TitleID_t title_id;		// [0x18C] Title ID.
	uint32_t title_type;		// [0x194] Title type.
	uint16_t group_id;		// [0x198] Group ID.
	uint16_t reserved1;		// [0x19A]

	// region_code and ratings are NOT valid for discs.
	// They're only valid for WiiWare.
	uint16_t region_code;		// [0x19C] Region code. (See GCN_Region_Code.)
	uint8_t ratings[0x10];		// [0x19E] Country-specific age ratings.
	uint8_t reserved3[12];		// [0x1AE]

	uint8_t ipc_mask[12];		// [0x1BA] IPC mask.
	uint8_t reserved4[18];		// [0x1C6]
	uint32_t access_rights;		// [0x1D8] Access rights. (See RVL_Access_Rights_e.)
	uint16_t title_version;		// [0x1DC] Title version.
	uint16_t nbr_cont;		// [0x1DE] Number of contents.
	uint16_t boot_index;		// [0x1E0] Boot index.
	uint8_t padding2[2];		// [0x1E2]

	// Following this header is a variable-length content table.
} RVL_TMD_Header;
ASSERT_STRUCT(RVL_TMD_Header, 0x1E4);

/**
 * Access rights.
 */
typedef enum {
	RVL_ACCESS_RIGHTS_AHBPROT	= (1U << 0),
	RVL_ACCESS_RIGHTS_DVD_VIDEO	= (1U << 1),
} RVL_Access_Rights_e;

/**
 * Wii partition header.
 * Reference: https://wiibrew.org/wiki/Wii_Disc#Partition
 */
typedef struct PACKED _RVL_PartitionHeader {
	RVL_Ticket ticket;			// [0x000]
	uint32_t tmd_size;			// [0x2A4] TMD size.
	uint34_rshift2_t tmd_offset;		// [0x2A8] TMD offset, rshifted by 2.
	uint32_t cert_chain_size;		// [0x2AC] Certificate chain size.
	uint34_rshift2_t cert_chain_offset;	// [0x2B0] Certificate chain offset, rshifted by 2.
	uint34_rshift2_t h3_table_offset;	// [0x2B4] H3 table offset, rshifted by 2. (Size is always 0x18000.)
	uint34_rshift2_t data_offset;		// [0x2B8] Data offset, rshifted by 2.
	uint34_rshift2_t data_size;		// [0x2BC] Data size, rshifted by 2.
	uint8_t tmd[0x7D40];			// [0x2C0] TMD, variable length up to data_offset.
} RVL_PartitionHeader;
ASSERT_STRUCT(RVL_PartitionHeader, 0x8000);

/**
 * Country indexes in RVL_RegionSetting.ratings[].
 */
typedef enum {
	RVL_RATING_JAPAN		= 0,	// CERO
	RVL_RATING_USA			= 1,	// ESRB
	RVL_RATING_GERMANY		= 3,	// USK
	RVL_RATING_PEGI			= 4,	// PEGI
	RVL_RATING_FINLAND		= 5,	// MEKU?
	RVL_RATING_PORTUGAL		= 6,	// Modified PEGI
	RVL_RATING_BRITAIN		= 7,	// BBFC
	RVL_RATING_AUSTRALIA		= 8,	// AGCB
	RVL_RATING_SOUTH_KOREA		= 9,	// GRB
} RVL_RegionSetting_RatingCountry;

/**
 * Region setting and age ratings.
 * Reference: https://wiibrew.org/wiki/Wii_Disc#Region_setting
 */
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

#endif /* __ROMPROPERTIES_LIBROMDATA_WII_STRUCTS_H__ */

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wii_structs.h: Nintendo Wii data structures.                            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This file has Wii-specific structs only.
// For structs shared with GameCube, see gcn_structs.h.

#pragma once

#include <stdint.h>
#include "common.h"
#include "nintendo_system_id.h"

#ifdef __cplusplus
// for uint34_rshift2_t's inline helper functions
#  include <cassert>
#  include "byteswap_rp.h"
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

// 34-bit value stored in uint32_t.
// The value must be lshifted by 2.
typedef struct _uint34_rshift2_t {
	uint32_t val;
#ifdef __cplusplus
	/**
	 * Get this uint34_rshift2_t as uint64_t.
	 * No byteswapping in this version.
	 * @return uint64_t
	 */
	uint64_t get(void) const {
		return (static_cast<uint64_t>(val) << 2);
	}

	/**
	 * Get this uint34_rshift2_t as uint64_t.
	 * Byteswapped from big-endian, if necessary.
	 * @return uint64_t
	 */
	uint64_t get_be(void) const {
		return (static_cast<uint64_t>(be32_to_cpu(val)) << 2);
	}

	/**
	 * Get this uint34_rshift2_t as off64_t.
	 * No byteswapping in this version.
	 * @return off64_t
	 */
	off64_t geto(void) const {
		return (static_cast<off64_t>(val) << 2);
	}

	/**
	 * Get this uint34_rshift2_t as off64_t.
	 * Byteswapped from big-endian, if necessary.
	 * @return uint64_t
	 */
	off64_t geto_be(void) const {
		return (static_cast<off64_t>(be32_to_cpu(val)) << 2);
	}

	/**
	 * Save a uint64_t in this uint34_rshift2_t.
	 * No byteswapping in this version.
	 * @param val34 34-bit value (2 low bits must be 0)
	 */
	void set(uint64_t val34) {
		assert((val34 & 0xFFFFFFFC00000003ULL) == 0);
		val = static_cast<uint32_t>(val34 >> 2);
	}

	/**
	 * Save a uint64_t in this uint34_rshift2_t.
	 * No byteswapping in this version.
	 * @param val34 34-bit value (2 low bits must be 0)
	 */
	void seto(off64_t val34) {
		assert((val34 & 0xFFFFFFFC00000003ULL) == 0);
		val = static_cast<uint32_t>(val34 >> 2);
	}
#endif /* __cplusplus */
} uint34_rshift2_t;

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
typedef struct _RVL_VolumeGroupTable {
	struct {
		uint32_t count;		// Number of partitions in this volume group.
		uint34_rshift2_t addr;	// Start address of this table, rshifted by 2.
	} vg[4];
} RVL_VolumeGroupTable;
ASSERT_STRUCT(RVL_VolumeGroupTable, 2*4*sizeof(uint32_t));

/**
 * Wii partition table entry.
 * Contains information about an individual partition.
 * Reference: https://wiibrew.org/wiki/Wii_Disc#Partition_table_entry
 *
 * All fields are big-endian.
 */
typedef struct _RVL_PartitionTableEntry {
	uint34_rshift2_t addr;	// Start address of this partition, rshifted by 2.
	uint32_t type;		// Type of partition. (0 == Game, 1 == Update, 2 == Channel Installer, other = title ID)
} RVL_PartitionTableEntry;
ASSERT_STRUCT(RVL_PartitionTableEntry, 2*sizeof(uint32_t));

typedef enum {
	RVL_PT_GAME	= 0,
	RVL_PT_UPDATE	= 1,
	RVL_PT_CHANNEL	= 2,
} RVL_PartitionType;

// Signature types
typedef enum {
	RVL_CERT_SIGTYPE_RSA4096_SHA1	= 0x00010000,	// RSA-4096 with SHA-1
	RVL_CERT_SIGTYPE_RSA2048_SHA1	= 0x00010001,	// RSA-2048 with SHA-1
	RVL_CERT_SIGTYPE_ECC		= 0x00010002,	// Elliptic Curve
	WUP_CERT_SIGTYPE_RSA4096_SHA256	= 0x00010003,	// RSA-4096 with SHA-256
	WUP_CERT_SIGTYPE_RSA2048_SHA256	= 0x00010004,	// RSA-2048 with SHA-256

	WUP_CERT_SIGTYPE_FLAG_DISC	= 0x00020000,	// Set for disc titles
} RVL_Cert_SigType_e;

// // Common key indexes
#define RVL_COMMON_KEY_INDEX_DEFAULT	0
#define RVL_COMMON_KEY_INDEX_KOREAN	1
#define WUP_COMMON_KEY_INDEX_vWii	2

/**
 * Time limit structs for Wii ticket.
 * Reference: https://wiibrew.org/wiki/Ticket
 */
typedef struct _RVL_TimeLimit {
	uint32_t enable;	// 1 == enable; 0 == disable
	uint32_t seconds;	// Time limit, in seconds.
} RVL_TimeLimit;
ASSERT_STRUCT(RVL_TimeLimit, 2*sizeof(uint32_t));

/**
 * Wii ticket (v0)
 * Reference: https://wiibrew.org/wiki/Ticket
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _RVL_Ticket {
	uint32_t signature_type;	// [0x000] Signature type
	uint8_t signature[0x100];	// [0x004] Signature
	uint8_t padding_sig[0x3C];	// [0x104] Padding (always 0)

	// The following fields are all covered by the above signature.
	char signature_issuer[0x40];	// [0x140] Signature issuer
	uint8_t ecdh_data[0x3C];	// [0x180] ECDH data
	uint8_t ticket_format_version;	// [0x1BC] Ticket format version (usually v0 for Wii; v1 for Wii U)
	uint8_t padding1[0x02];		// [0x1BD] Padding
	uint8_t enc_title_key[0x10];	// [0x1BF] Encrypted title key
	uint8_t unknown1;		// [0x1CF] Unknown
	uint8_t ticket_id[0x08];	// [0x1D0] Ticket ID (IV for title key decryption for console-specific titles)
	uint32_t console_id;		// [0x1D8] Console ID (0 for "any console")
	Nintendo_TitleID_BE_t title_id;	// [0x1DC] Title ID (IV used for AES-CBC encryption)
	uint8_t unknown2[2];		// [0x1E4] Unknown, mostly 0xFFFF
	uint8_t ticket_version[2];	// [0x1E6] Ticket version
	uint32_t permitted_titles_mask;	// [0x1E8] Permitted titles mask
	uint32_t permit_mask;		// [0x1EC] Permit mask
	uint8_t title_export;		// [0x1F0] Title Export allowed using PRNG key (1 == yes, 0 == no)
	uint8_t common_key_index;	// [0x1F1] Common Key index (0 == default, 1 == Korean, 2 == vWii)
	uint8_t unknown3[0x30];		// [0x1F2] Unknown. (VC related?)
	uint8_t content_access_perm[0x40];	// [0x222] Content access permissions. (1 bit per content)
	uint8_t padding2[2];		// [0x262] Padding. (always 0)
	RVL_TimeLimit time_limits[8];	// [0x264] Time limits.
} RVL_Ticket;
ASSERT_STRUCT(RVL_Ticket, 0x2A4);
#pragma pack()

/**
 * Wii U ticket (v1): v1 ticket header
 * Reference: https://wiibrew.org/wiki/Ticket
 *
 * All fields are big-endian.
 */
typedef struct _RVL_Ticket_v1_Header {
	uint16_t header_version;		// [0x2A4] v1 header version (usually 1)
	uint16_t size_header;			// [0x2A6] Size of the v1 header
	uint32_t size_data;			// [0x2A8] Size of the v1 data (including header) [usually 0xAC or 172]
	uint32_t offset_section_headers;	// [0x2AC] Offset to section headers, relative to the beginning of this header
	uint16_t num_section_headers;		// [0x2B0] Number of section headers
	uint16_t size_section_header;		// [0x2B2] Size of each section header [0x14?]
	uint32_t flags;				// [0x2B4] Flags (TODO)
} RVL_Ticket_v1_Header;
ASSERT_STRUCT(RVL_Ticket_v1_Header, 0x14);

/**
 * Wii U ticket (v1): section header
 * Reference: https://wiibrew.org/wiki/Ticket
 *
 * All fields are big-endian.
 */
typedef struct _RVL_Ticket_v1_Section_Header {
	uint32_t offset;		// [0x000] Offset to the records
	uint32_t num_records;		// [0x004] Number of records
	uint32_t size_of_each;		// [0x008] Size of each record in this section
	uint32_t size_of_section;	// [0x00C] Size of this section
	uint16_t section_type;		// [0x010] Section type
	uint16_t flags;			// [0x012] Flags (TODO)
} RVL_Ticket_v1_Section_Header;
ASSERT_STRUCT(RVL_Ticket_v1_Section_Header, 0x14);

/**
 * Wii U ticket (v1)
 * Also supported by Wii IOS56 and later.
 * Reference: http://wiibrew.org/wiki/Ticket
 *
 * All fields are big-endian.
 */
typedef struct _RVL_Ticket_V1 {
	RVL_Ticket v0;					// [0x000] Same as RVL_Ticket v0; v0.ticket_format_version == 1
	RVL_Ticket_v1_Header v1;			// [0x2A4] RVL Ticket v1 header

	union {
		RVL_Ticket_v1_Section_Header sections[7];	// [0x2B8] Up to 7 section headers fit in 848 bytes
								//         TODO: Larger v1 tickets?
		uint8_t section_data[152];			// [0x2B8] Section header data
	};
} RVL_Ticket_V1;
ASSERT_STRUCT(RVL_Ticket_V1, 0x350);

/**
 * Wii TMD header
 * References:
 * - https://wiibrew.org/wiki/Title_metadata
 * - https://wiiubrew.org/wiki/Title_metadata
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _RVL_TMD_Header {
	uint32_t signature_type;	// [0x000] Signature type
	uint8_t signature[0x100];	// [0x004] Signature
	uint8_t padding_sig[0x3C];	// [0x104] Padding (always 0)

	// The following fields are all covered by the above signature.
	char signature_issuer[0x40];	// [0x140] Signature issuer
	uint8_t tmd_format_version;	// [0x180] TMD format version (v0 for Wii; v1 for Wii U)
	uint8_t ca_crl_version;		// [0x181] CA CRL version
	uint8_t signer_crl_version;	// [0x182] Signer CRL version
	uint8_t padding1;		// [0x183]
	Nintendo_TitleID_BE_t sys_version;	// [0x184] System version [IOS(U) title ID]
	Nintendo_TitleID_BE_t title_id;		// [0x18C] Title ID
	uint32_t title_type;		// [0x194] Title type
	uint16_t group_id;		// [0x198] Group ID
	uint16_t reserved1;		// [0x19A]

	// region_code and ratings are NOT valid for discs.
	// They're only valid for WiiWare.
	uint16_t region_code;		// [0x19C] Region code (See GCN_Region_Code)
	uint8_t ratings[0x10];		// [0x19E] Country-specific age ratings
	uint8_t reserved3[12];		// [0x1AE]

	uint8_t ipc_mask[12];		// [0x1BA] IPC mask
	uint8_t reserved4[18];		// [0x1C6]
	uint32_t access_rights;		// [0x1D8] Access rights (See RVL_Access_Rights_e)
	uint16_t title_version;		// [0x1DC] Title version
	uint16_t nbr_cont;		// [0x1DE] Number of contents
	uint16_t boot_index;		// [0x1E0] Boot index
	uint8_t padding2[2];		// [0x1E2]

	// Following this header is:
	// - v0: Content table (length indicated by nbr_cont)
	// - v1: CMD group header
} RVL_TMD_Header;
ASSERT_STRUCT(RVL_TMD_Header, 0x1E4);
#pragma pack()

/**
 * Access rights.
 */
typedef enum {
	RVL_ACCESS_RIGHTS_AHBPROT	= (1U << 0),
	RVL_ACCESS_RIGHTS_DVD_VIDEO	= (1U << 1),
} RVL_Access_Rights_e;

/**
 * Wii content entry (Stored after the TMD) (v0)
 * Reference: https://wiibrew.org/wiki/Title_metadata
 *
 * All fields are big-endian.
 */
#pragma pack(1)
typedef struct RP_PACKED _RVL_Content_Entry {
	uint32_t content_id;		// [0x000] Content ID
	uint16_t index;			// [0x004] Index
	uint16_t type;			// [0x006] Type (see RVL_Content_Type_e)
	uint64_t size;			// [0x008] Size
	uint8_t sha1_hash[20];		// [0x010] SHA-1 hash of the content (installed) or H3 table (disc).
} RVL_Content_Entry;
ASSERT_STRUCT(RVL_Content_Entry, 36);
#pragma pack()

/**
 * Content type (bitfield)
 */
typedef enum {
	RVL_CONTENT_TYPE_DEFAULT	= 0x0001,
	RVL_CONTENT_TYPE_UNKNOWN_0x04	= 0x0004,

	// Used for DLC titles.
	RVL_CONTENT_TYPE_DATA		= 0x0008,
	RVL_CONTENT_TYPE_UNKNOWN_0x10	= 0x0010,

	// Seems to be used for WFS titles.
	RVL_CONTENT_TYPE_MAYBE_WFS	= 0x0020,
	RVL_CONTENT_TYPE_UNKNOWN_CT	= 0x0040,

	// Seen in CoD:MW3 DLC.
	RVL_CONTENT_TYPE_UNKNOWN_0x4000	= 0x4000,

	// Seen in Pop Europe (WiiWare).
	RVL_CONTENT_TYPE_UNKNOWN_0x8000	= 0x8000,
} RVL_Content_Type_e;

/**
 * Wii partition header.
 * Reference: https://wiibrew.org/wiki/Wii_Disc#Partition
 */
typedef struct _RVL_PartitionHeader {
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
typedef struct _RVL_RegionSetting {
	uint32_t region_code;	// Region code. (See GCN_Region_Code.)
	uint32_t disc_count;	// Number of game discs.
	uint8_t reserved[8];
	uint8_t ratings[0x10];	// Country-specific age ratings.
} RVL_RegionSetting;
ASSERT_STRUCT(RVL_RegionSetting, 32);

#ifdef __cplusplus
}
#endif

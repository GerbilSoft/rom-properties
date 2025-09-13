/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nfp_structs.h: Nintendo amiibo data structures.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://www.3dbrew.org/wiki/Amiibo
 * - https://www.reddit.com/r/amiibo/comments/38hwbm/nfc_character_identification_my_findings_on_the/
 * - https://www.nxp.com/documents/data_sheet/NTAG213_215_216.pdf
 */

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Amiibo binary file sizes.
 */
typedef enum {
	NFP_FILE_STANDARD	= 540,
	NFP_FILE_NO_PW		= 532,
	NFP_FILE_EXTENDED	= 572,
} NFP_File_Size;

/**
 * NTAG215 structure for Nintendo Figurine Platform.
 * Reference: https://www.3dbrew.org/wiki/Amiibo
 *
 * Page size: 4 bytes
 * Page count: 135 pages (540 bytes)
 * Data pages: 126 pages (504 bytes)
 * All fields are in big-endian.
 *
 * Comments: [0xPG,RO] or [0xPG,RW]
 * PG = page number.
 * RO = read-only
 * RW = read/write
 */
#define NFP_LOCK_HEADER 0x0FE0U
#define NFP_CAP_CONTAINER 0xF110FFEEU
#define NFP_CFG0 0x00000004U
#define NFP_CFG1 0x5F000000U
typedef struct _NFP_Data_t {
	/** NTAG215 header **/
	uint8_t serial[9];		// [0x00,RO] NTAG215 serial number.
	uint8_t int_u8;			// [0x02,RO] "Internal" u8 value
	uint16_t lock_header;		// [0x02,RO] Lock bytes. Must match: 0x0FE0
	uint32_t cap_container;		// [0x03,RO] Must match: 0xF1 0x10 0xFF 0xEE

	/** User data area **/

	uint8_t hmac_counter[4];	// [0x04,RW] Some counter used with HMAC.
	uint8_t crypt_data[32];		// [0x05,RW] Encryption data.
	uint8_t sha256_hash_1[32];	// [0x0D,RO] SHA256(-HMAC?) hash of something.
					// First 0x18 bytes of this hash is section3 in the encrypted buffer.

	// Character identification (page 0x15, raw offset 0x54)
	uint32_t char_id;		// [0x15,RO] Character identification.
	union {
		uint32_t u32;
		uint8_t u8[4];
	} amiibo_id;			// [0x16,RO] amiibo series identification.
	uint8_t unknown1[4];		// [0x17,RO]
	uint8_t sha256_hash_2[32];	// [0x18,RO] SHA256(-HMAC?) hash of something.

	uint8_t sha256_hash_data[32];	// [0x20,RW] SHA256-HMAC hash over 0x1DF bytes.
					// First 3 bytes are the last 3 bytes of [0x04,RW].
					// Remaining is first 0x1DC bytes of plaintext data.

	uint8_t section1[0x114];	// [0x28,RW] section1 of encrypted data.
	uint8_t section2[0x54];		// [0x6D,RW] section2 of encrypted data.

	/** NTAG215 footer **/
	uint8_t lock_footer[4];		// [0x82,RO] NTAG215 dynamic lock bytes.
					// First 3 bytes must match: 0x01 0x00 0x0F
	uint32_t cfg0;			// [0x83,RO] NTAG215 CFG0. Must match: 0x00000004
	uint32_t cfg1;			// [0x84,RO] NTAG215 CFG1. Must match: 0x5F000000

	uint8_t pwd[4];			// [0x85,RO]
	uint8_t pack[2];		// [0x86,RO]
	uint8_t rfui[2];		// [0x87,RO]

	// Extra data present in extended dumps.
	// TODO: What is this data?
	uint8_t extended[32];
} NFP_Data_t;
ASSERT_STRUCT(NFP_Data_t, 572);

/**
 * amiibo type. (low byte of char_id)
 */
typedef enum {
	NFP_TYPE_FIGURINE	= 0x00,
	NFP_TYPE_CARD		= 0x01,
	NFP_TYPE_YARN		= 0x02,
	NFP_TYPE_BAND		= 0x03,
	NFP_TYPE_WOODEN_BLOCK	= 0x04,
} NFP_Type_t;

#ifdef __cplusplus
}
#endif

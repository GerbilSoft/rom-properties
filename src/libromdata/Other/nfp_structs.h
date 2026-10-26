/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nfp_structs.h: Nintendo amiibo data structures.                         *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
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
 * Amiibo binary file sizes
 */
typedef enum {
	// Amiibo v2 (NTAG215)
	NFP_FILE_STANDARD	= 540,
	NFP_FILE_NO_PW		= 532,
	NFP_FILE_EXTENDED	= 572,

	// Super Nintendo World band (NT3H2111)
	NFP_SNW_STANDARD	= 944,
	NFP_SNW_EXTENDED	= 1024,

	// Amiibo v3 (NT3H2211)
	NFP_V3_STANDARD		= 1968,
	NFP_V3_EXTENDED		= 2048,

	NFP_MAX_SIZE		= NFP_V3_EXTENDED
} NFP_File_Size;

/**
 * Common user data structure for both NTAG215 and NT3H2111/NT3H2211 amiibo.
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
typedef struct _NFP_User_Data_t {
	/** User data area (page 0x04) **/
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
} NFP_User_Data_t;
ASSERT_STRUCT(NFP_User_Data_t, 504);

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
#define NFP_NTAG215_LOCK_HEADER 0x0FE0U
#define NFP_NTAG215_CAP_CONTAINER 0xF110FFEEU
#define NTP_NTAG215_LOCK_FOOTER 0x01000FBDU
#define NFP_NTAG215_CFG0 0x00000004U
#define NFP_NTAG215_CFG1 0x5F000000U
typedef struct _NFP_NTAG215_Data_t {
	/** NTAG215 header **/
	uint8_t uid[9];			// [0x00,RO] NTAG215 unique ID (includes two check bytes, BCC0/BCC1)
	uint8_t int_u8;			// [0x02,RO] "Internal" u8 value
	uint16_t lock_header;		// [0x02,RO] Lock bytes. Must match: 0x0FE0
	uint32_t cap_container;		// [0x03,RO] Must match: 0xF1 0x10 0xFF 0xEE

	/** User data area (page 0x04) **/
	NFP_User_Data_t user_data;

	/** NTAG215 footer **/
	uint8_t lock_footer[4];		// [0x82,RO] NTAG215 dynamic lock bytes.
					// First 3 bytes must match: 0x01 0x00 0x0F (fourth is always 0xBD?)
	uint32_t cfg0;			// [0x83,RO] NTAG215 CFG0. Must match: 0x00000004
	uint32_t cfg1;			// [0x84,RO] NTAG215 CFG1. Must match: 0x5F000000

	uint8_t pwd[4];			// [0x85,RO]
	uint8_t pack[2];		// [0x86,RO]
	uint8_t rfui[2];		// [0x87,RO]

	// Extra data present in extended dumps.
	// TODO: What is this data?
	uint8_t extended[32];
} NFP_NTAG215_Data_t;
ASSERT_STRUCT(NFP_NTAG215_Data_t, NFP_FILE_EXTENDED);

/**
 * NT3H2111/NT3H2211 structure for Nintendo Figurine Platform.
 * NOTE: 2111 and 2211 have the same structures in the first 1 KB.
 * 2211 has a second 1 KB that's all user memory.
 *
 * Page size: 4 bytes
 * Page count: 256 pages (1024 bytes) or 512 pages (2048 bytes)
 * Data pages: 236 pages (944 bytes) or 492 pages (1968 bytes)
 * All fields are in big-endian.
 *
 * Comments: [0xPG,RO] or [0xPG,RW]
 * PG = page number.
 * RO = read-only
 * RW = read/write
 */
#define NFP_NT3H2111_LOCK_HEADER 0x0FE0U
#define NFP_NT3H2111_CAP_CONTAINER 0xF110FFEEU
#define NTP_NT3H2111_LOCK_FOOTER 0x01011F00U
#define NTP_NT3H2211_LOCK_FOOTER 0x0100FF00U
#define NFP_NT3H2111_CFG0 0x0100F848U
#define NFP_NT3H2111_CFG1 0x08010000U
typedef struct _NFP_NT3H2111_Data_t {
	/** NT3H2111 header **/
	uint8_t uid[7];			// [0x00,RO] NT3H2111 unique ID (no check bytes!)
	uint8_t int_u8[3];		// [0x01/0x02,RO] "Internal" u8 values
	uint16_t lock_header;		// [0x02,RO] Lock bytes. Must match: 0x0FE0
	uint32_t cap_container;		// [0x03,RO] Must match: 0xF1 0x10 0xFF 0xEE

	/** User data area (page 0x04) **/
	NFP_User_Data_t user_data;
	uint8_t section3[384];		// [0x82,RW] assuming extra data for NT3H2111/NT3H2211...

	/** NT3H2111 footer **/
	uint8_t lock_footer[4];		// [0xE2,RO] NT3H2111 dynamic lock bytes.
					// First 3 bytes must match:
					// - NT3H2111: 0x01 0x01 0x1F
					// - NT3H2211: 0x01 0x00 0xFF
	uint32_t rfu_pwd[5];		// [0xE3,RO] RFU/PWD stuff

	uint32_t cfg0;			// [0xE8,RO] NT3H2111 CFG0. Must match: 0x0100F848
	uint32_t cfg1;			// [0xE9,RO] NT3H2111 CFG1. Must match: 0x08010000

	uint32_t nak0[2];		// [0xEA,RO] Invalid access - returns NAK
	uint32_t session[2];		// [0xEC,RO] Session registers
	uint32_t nak1[18];		// [0xEE,RO] Invalid access - returns NAK
} NFP_NT3H2111_Data_t;
ASSERT_STRUCT(NFP_NT3H2111_Data_t, NFP_SNW_EXTENDED);

/**
 * amiibo type (low byte of char_id)
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

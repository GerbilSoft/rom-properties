/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ique_player_structs.h: iQue Player data structures.                     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// .cmd files are always 10,668 (0x29AC) bytes.
#define IQUE_PLAYER_CMD_FILESIZE 10668
// .dat (ticket) files are always 11,084 (0x2B4C) bytes.
#define IQUE_PLAYER_DAT_FILESIZE 11084

/**
 * iQue Player .cmd header
 * References:
 * - https://github.com/simontime/iQueCMD/blob/master/Program.cs
 * - http://www.iquebrew.org/index.php?title=CMD
 *
 * All fields are in big-endian.
 */
#define IQUE_PLAYER_MAGIC "CAM"
typedef struct _iQuePlayer_contentDesc {
	uint32_t eeprom_rdram_addr;		// [0x000] EEPROM RDRAM address
	uint32_t eeprom_rdram_size;		// [0x004] EEPROM RDRAM size
	uint32_t flash_rdram_addr;		// [0x008] Flash RDRAM address
	uint32_t flash_rdram_size;		// [0x00C] Flash RDRAM size
	uint32_t sram_rdram_addr;		// [0x010] SRAM RDRAM address
	uint32_t sram_rdram_size;		// [0x014] SRAM RDRAM size
	uint32_t controller_pak_addr[4];	// [0x018] Controller pak addresses
	uint32_t controller_pak_size;		// [0x028] Controller pak size
	uint32_t osRomBase;			// [0x02C] osRomBase
	uint32_t osTvType;			// [0x030] osTvType
	uint32_t osMemSize;			// [0x034] osMemSize
	uint32_t unknown1[2];			// [0x038]

	char magic[3];				// [0x040] "CAM" (magic)
	uint8_t u0x_file_count;			// [0x043] Number of .u0x files
	uint16_t thumb_image_size;		// [0x044] Thumb image size (max 0x4000)
						//         Decompressed size must be 0x1880.
	uint16_t title_image_size;		// [0x048] Title image size (max 0x10000)

	// Following the .cmd header are the two images, both DEFLATE-compressed:
	// - Thumbnail image: 56x56, RGBA5551
	// - Title image: 184x24, IA8
} iQuePlayer_contentDesc;
ASSERT_STRUCT(iQuePlayer_contentDesc, 0x48);

// Image sizes.
#define IQUE_PLAYER_THUMB_W 56
#define IQUE_PLAYER_THUMB_H 56
#define IQUE_PLAYER_THUMB_SIZE (IQUE_PLAYER_THUMB_W * IQUE_PLAYER_THUMB_H * 2)
#define IQUE_PLAYER_TITLE_W 184
#define IQUE_PLAYER_TITLE_H 24
#define IQUE_PLAYER_TITLE_SIZE (IQUE_PLAYER_TITLE_W * IQUE_PLAYER_TITLE_H * 2)

/**
 * Content metadata header
 * Located at 0x2800 in the .cmd file.
 *
 * Reference: http://www.iquebrew.org/index.php?title=CMD
 *
 * All fields are in big-endian.
 */
#define IQUE_PLAYER_BBCONTENTMETADATAHEAD_ADDRESS 0x2800
typedef struct _iQuePlayer_BbContentMetaDataHead {
	uint32_t unusedPadding;		// [0x000]
	uint32_t caCrlVersion;		// [0x004]
	uint32_t cpCrlVersion;		// [0x008]
	uint32_t size;			// [0x00C] Size of the application.
	uint32_t descFlags;		// [0x010]
	uint8_t commonCmdIv[16];	// [0x014] IV used to encrypt title key (using common key)
	uint8_t hash[20];		// [0x024] SHA-1 hash of the application plaintext.
	uint8_t iv[16];			// [0x038] Content IV.
	uint32_t execFlags;		// [0x048]
	uint32_t hwAccessRights;	// [0x04C] See IQUE_PLAYER_hwAccessRights_e.
	uint32_t secureKernelRights;	// [0x050] Secure kernel calls. (bitfield, 1=allowed)
	uint32_t bbid;			// [0x054] If non-zero, limited to specific console.
	char issuer[64];		// [0x058] Certificate used to sign CMD.
	uint32_t content_id;		// [0x098] Content ID.
	uint8_t key[16];		// [0x09C] Encrypted title key.
	uint8_t rsa2048_sig[256];	// [0x0AC] RSA-2048 signature. If key[] is encrypted twice
					// for non-SA, then this is *before* the second encryption.
} iQuePlayer_BbContentMetaDataHead;
ASSERT_STRUCT(iQuePlayer_BbContentMetaDataHead, 0x1AC);

/**
 * Hardware access rights
 */
typedef enum {
	IQUE_PLAYER_HW_PI_BUFFER	= (1U << 0),
	IQUE_PLAYER_HW_NAND_FLASH	= (1U << 1),
	IQUE_PLAYER_HW_MEMORY_MAPPER	= (1U << 2),
	IQUE_PLAYER_HW_AES_ENGINE	= (1U << 3),
	IQUE_PLAYER_HW_NEW_PI_DMA	= (1U << 4),
	IQUE_PLAYER_HW_GPIO		= (1U << 5),
	IQUE_PLAYER_HW_EXT_IO		= (1U << 6),
	IQUE_PLAYER_HW_NEW_PI_ERR	= (1U << 7),
	IQUE_PLAYER_HW_USB		= (1U << 8),
	IQUE_PLAYER_HW_SK_RAM		= (1U << 9),
} iQuePlayer_hwAccessRights_e;

/**
 * Ticket header
 * Located after the content metadata in .dat files.
 *
 * Reference: http://www.iquebrew.org/index.php?title=Ticket
 *
 * All fields are in big-endian.
 */
#define IQUE_PLAYER_BBTICKETHEAD_ADDRESS 0x29AC
typedef struct _iQuePlayer_BbTicketHead {
	uint32_t bbId;			// [0x29AC] Console ID.
	uint16_t tid;			// [0x29B0] Ticket ID. (if bit 15 is set, this is a trial ticket)
	uint16_t code;			// [0x29B2] Trial limitation. (See IQUE_PLAYER_TrialLimitation_e.)
	uint16_t limit;			// [0x29B4] Number of minutes, or number of launches,
					//          before limit is exceeded.
	uint16_t reserved1;		// [0x29B6]
	uint32_t tsCrlVersion;		// [0x29B8] Ticket CRL version
	uint8_t cmdIv[16];		// [0x29BC] Title key IV; IV used to re-encrypt title key (with ECDH key)
	uint8_t serverKey[64];		// [0x29CC] ECC public key used to derive unique title key encryption key
	char issuer[64];		// [0x2A0C] Certificate used to sign the ticket.
	uint8_t ticketSign[256];	// [0x2A4C] RSA-2048 signature over CMD *and* above ticket data.
} iQuePlayer_BbTicketHead;
ASSERT_STRUCT(iQuePlayer_BbTicketHead, 0x2B4C - IQUE_PLAYER_BBTICKETHEAD_ADDRESS);

/**
 * Trial limitations
 */
typedef enum {
	IQUE_PLAYER_TRIAL_TIME_0	= 0,	// Time-based limitation
	IQUE_PLAYER_TRIAL_LAUNCHES	= 1,	// Number of launches
	IQUE_PLAYER_TRIAL_TIME_2	= 2,	// Time-based limitation
} iQuePlayer_TrialLimitation_e;

#ifdef __cplusplus
}
#endif

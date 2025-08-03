/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wii_banner.h: Nintendo Wii banner structures.                           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "common.h"

// for Title ID
#include "wii_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WIBN (Wii Banner)
 * Reference: https://wiibrew.org/wiki/Savegame_Files
 * NOTE: This may be located at one of two places:
 * - 0x0000: banner.bin extracted via SaveGame Manager GX
 * - 0x0020: Savegame extracted via Wii System Menu
 */

// Magic numbers
#define BANNER_WIBN_MAGIC		'WIBN'		/* 'WIBN' */
#define BANNER_WIBN_ADDRESS_RAW		0x0000		/* banner.bin from SaveGame Manager GX */
#define BANNER_WIBN_ADDRESS_ENCRYPTED	0x0020		/* extracted from Wii System Menu */

// Flags.
#define BANNER_WIBN_FLAGS_NOCOPY	0x01
#define BANNER_WIBN_FLAGS_ICON_BOUNCE	0x10

// Banner size.
#define BANNER_WIBN_IMAGE_W 192
#define BANNER_WIBN_IMAGE_H 64

// Icon size.
#define BANNER_WIBN_ICON_W 48
#define BANNER_WIBN_ICON_H 48

// Struct size.
#define BANNER_WIBN_IMAGE_SIZE (BANNER_WIBN_IMAGE_W * BANNER_WIBN_IMAGE_H * 2)
#define BANNER_WIBN_ICON_SIZE (BANNER_WIBN_ICON_W * BANNER_WIBN_ICON_H * 2)
#define BANNER_WIBN_STRUCT_SIZE (sizeof(Wii_WIBN_Header_t) + BANNER_WIBN_IMAGE_SIZE)
#define BANNER_WIBN_STRUCT_SIZE_ICONS(icons) \
	(BANNER_WIBN_STRUCT_SIZE + ((icons)*BANNER_WIBN_ICON_SIZE))

/**
 * Wii save game banner header.
 * Reference: https://wiibrew.org/wiki/Savegame_Files#Banner
 *
 * All fields are in big-endian.
 */
#define WII_WIBN_MAGIC 'WIBN'
typedef struct _Wii_WIBN_Header_t {
	uint32_t magic;			// [0x000] 'WIBN'
	uint32_t flags;			// [0x004] Flags. (See Wii_WIBN_Flags_e.)
	uint16_t iconspeed;		// [0x008] Similar to GCN.
	uint8_t reserved[22];		// [0x00A]
	char16_t gameTitle[32];		// [0x020] Game title. (UTF-16 BE)
	char16_t gameSubTitle[32];	// [0x060] Game subtitle. (UTF-16 BE)
} Wii_WIBN_Header_t;
ASSERT_STRUCT(Wii_WIBN_Header_t, 160);

/**
 * Wii save game flags.
 */
typedef enum {
	WII_WIBN_FLAG_NO_COPY		= (1U << 0),	// Cannot copy from NAND normally.
	WII_WIBN_FLAG_ICON_BOUNCE	= (1U << 4),	// Icon animation "bounces" instead of looping.
} Wii_WIBN_Flags_e;

// IMET magic number
#define WII_IMET_MAGIC 'IMET'

/**
 * IMET (Wii opening.bnr header)
 * This contains the game title.
 * Reference: https://wiibrew.org/wiki/Opening.bnr#banner.bin_and_icon.bin
 *
 * NOTE: This does not include the 64 or 128 bytes of data
 * that may show up before Wii_IMET_t.
 *
 * All fields are in big-endian.
 */
typedef struct _Wii_IMET_t {
	uint32_t magic;		// "IMET"
	uint32_t hashsize;	// Hash length
	uint32_t unknown;
	uint32_t sizes[3];	// icon.bin, banner.bin, sound.bin
	uint32_t flag1;

	// Titles. (UTF-16BE)
	// - Index 0: Language: JP,EN,DE,FR,ES,IT,NL,xx,xx,KO
	// - Index 1: Line
	// - Index 2: Character
	char16_t names[10][2][21];

	uint8_t zeroes2[588];
	uint8_t md5[16];	// MD5 of 0 to 'hashsize' in the header.
				// This field is all 0 when calculating.
} Wii_IMET_t;
ASSERT_STRUCT(Wii_IMET_t, 1472);

/**
 * IMET from NAND titles.
 *
 * This includes an extra header with the build string
 * and the builder, plus 64 zero bytes.
 *
 * All fields are in big-endian.
 */
typedef struct _Wii_IMET_NAND_t {
	char build_string[0x30];	// [0x000] Build string
	char builder[0x10];		// [0x030] Builder
	uint8_t padding[0x40];		// [0x040] Padding (all zeroes)
	Wii_IMET_t imet;		// [0x080] IMET header
} Wii_IMET_NAND_t;
ASSERT_STRUCT(Wii_IMET_NAND_t, 1472+128);

/**
 * IMET from disc titles.
 *
 * This includes 64 zero bytes.
 *
 * All fields are in big-endian.
 */
typedef struct _Wii_IMET_Disc_t {
	uint8_t padding[0x40];		// [0x000] Padding (all zeroes)
	Wii_IMET_t imet;		// [0x040] IMET header
} Wii_IMET_Disc_t;
ASSERT_STRUCT(Wii_IMET_Disc_t, 1472+64);

// Wii languages. (Maps to IMET indexes.)
typedef enum {
	WII_LANG_JAPANESE	= 0,
	WII_LANG_ENGLISH	= 1,
	WII_LANG_GERMAN		= 2,
	WII_LANG_FRENCH		= 3,
	WII_LANG_SPANISH	= 4,
	WII_LANG_ITALIAN	= 5,
	WII_LANG_DUTCH		= 6,
	// 7 and 8 are unknown. (Chinese?)
	WII_LANG_KOREAN		= 9,

	WII_LANG_MAX
} Wii_Language_ID;

/**
 * Wii save game main header.
 * This header is always encrypted.
 * Reference: https://wiibrew.org/wiki/Savegame_Files#Main_header
 *
 * All fields are in big-endian.
 */
typedef struct _Wii_SaveGame_Header_t {
	Nintendo_TitleID_BE_t savegame_id;	// [0x000] Savegame ID (title ID)
	uint32_t banner_size;		// [0x008] Size of banner+icons, with header. (max 0xF0A0)
	uint8_t permissions;		// [0x00C] Permissions (See Wii_SaveGame_Perm_e.)
	uint8_t unknown1;		// [0x00D]
	uint8_t md5_header[16];		// [0x00E] MD5 of plaintext header, with md5 blanker applied
	uint8_t unknown2[2];		// [0x01E]
} Wii_SaveGame_Header_t;
ASSERT_STRUCT(Wii_SaveGame_Header_t, 32);

/**
 * Wii save game permissions.
 * Similar to Unix permissions, except there's no Execute bit.
 */
typedef enum {
	Wii_SaveGame_Perm_User_Read	= 0x20,
	Wii_SaveGame_Perm_User_Write	= 0x10,
	Wii_SaveGame_Perm_Group_Read	= 0x08,
	Wii_SaveGame_Perm_Group_Write	= 0x04,
	Wii_SaveGame_Perm_Other_Read	= 0x02,
	Wii_SaveGame_Perm_Other_Write	= 0x01,

	Wii_SaveGame_Perm_Mask_User	= 0x30,
	Wii_SaveGame_Perm_Mask_Group	= 0x0C,
	Wii_SaveGame_Perm_Mask_Other	= 0x03,
} Wii_SaveGame_Perm_e;

/**
 * Wii save game Bk (backup) header.
 * This header is always unencrypted.
 * Reference: https://wiibrew.org/wiki/Savegame_Files#Bk_.28.22BacKup.22.29_Header
 *
 * All fields are in big-endian.
 */
#define WII_BK_SIZE 0x70
#define WII_BK_MAGIC 'Bk'
#define WII_BK_VERSION 0x0001
typedef struct _Wii_Bk_Header_t {
	union {
		struct {
			uint32_t size;		// [0x000] Size of the header. (0x070)
			uint16_t magic;		// [0x004] Magic. ('Bk')
			uint16_t version;	// [0x006] Version. (0x0001)
		};
		uint8_t full_magic[0x008];	// [0x000] 8-byte magic.
	};
	uint32_t ng_id;		// [0x008] NG id
	uint32_t num_files;	// [0x00C] Number of files.
	uint32_t size_files;	// [0x010] Size of files.
	uint32_t unknown1[2];	// [0x014]
	uint32_t total_size;	// [0x01C] Total size.
	uint8_t unknown2[64];	// [0x020]
	uint32_t unknown3;	// [0x060]
	char id4[4];		// [0x064] Game ID.
	uint8_t wii_mac[6];	// [0x068] MAC address of the originating Wii.
	uint8_t unknown4[2];	// [0x06E]
	uint8_t padding[16];	// [0x070] 64-byte alignment.
} Wii_Bk_Header_t;
ASSERT_STRUCT(Wii_Bk_Header_t, 0x70+0x10);

/**
 * Wii save game file header.
 * Reference: https://wiibrew.org/wiki/Savegame_Files#File_Header
 *
 * All fields are in big-endian.
 */
#define WII_SAVEGAME_FILEHEADER_MAGIC 0x03ADF17E
typedef struct _Wii_SaveGame_FileHeader_t {
	uint32_t magic;		// [0x000] Magic. (0x03ADF17E)
	uint32_t size;		// [0x004] Size of file.
	uint8_t permissions;	// [0x008] Permissions. (???)
	uint8_t attributes;	// [0x009] Attributes. (???)
	uint8_t type;		// [0x00A] Type. (1 == file, 2 == directory)
	char filename[0x45];	// [0x00B] Filename. (NULL-terminated)
	uint8_t iv[16];		// [0x050] IV for file decryption.
	uint8_t unknown[0x20];	// [0x060]
} Wii_SaveGame_FileHeader_t;
ASSERT_STRUCT(Wii_SaveGame_FileHeader_t, 0x80);

#ifdef __cplusplus
}
#endif

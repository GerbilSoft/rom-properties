/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nintendo_system_id.h: Nintendo system IDs.                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>

#include "common.h"
#include "librpbyteswap/byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Title ID struct/union.
 * Big-endian version for Wii and TMDs.
 *
 * TODO: Verify operation on big-endian systems.
 */
typedef union _Nintendo_TitleID_BE_t {
	uint64_t id;	// original TID is big-endian
	struct {
		union {
			uint32_t hi;
			struct {
				uint16_t sysID;
				uint16_t catID;
			};
		};
		uint32_t lo;
	};
	uint8_t u8[8];
} Nintendo_TitleID_BE_t;
ASSERT_STRUCT(Nintendo_TitleID_BE_t, sizeof(uint64_t));

/**
 * Title ID struct/union.
 * Big-endian version for DSi and 3DS (excluding TMD).
 *
 * TODO: Verify operation on big-endian systems.
 */
typedef union _Nintendo_TitleID_LE_t {
	uint64_t id;	// original TID is little-endian
	struct {
		uint32_t lo;
		union {
			uint32_t hi;
			struct {
				uint16_t catID;
				uint16_t sysID;
			};
		};
	};
	uint8_t u8[8];
} Nintendo_TitleID_LE_t;
ASSERT_STRUCT(Nintendo_TitleID_LE_t, sizeof(uint64_t));

/**
 * Nintendo Title ID: System IDs.
 * The system ID is the high 16 bits of the 64-bit title ID.
 *
 * This format originated with Routefree/BroadOn,
 * and was used on all Nintendo systems starting with Wii.
 *
 * Nintendo Switch drops it in favor of a new schema.
 */
typedef enum {
	NINTENDO_SYSID_BROADON	= 0,	// BroadOn
	NINTENDO_SYSID_RVL	= 1,	// Wii
	NINTENDO_SYSID_NC	= 2,	// GBA NetCard
	NINTENDO_SYSID_TWL	= 3,	// DSi
	NINTENDO_SYSID_CTR	= 4,	// 3DS
	NINTENDO_SYSID_WUP	= 5,	// Wii U
	NINTENDO_SYSID_vWii	= 7,	// vWii
} Nintendo_SysID_e;

/**
 * Nintendo Title ID: BroadOn: Category IDs.
 * For SysID == 0.
 */
typedef enum {
	NINTENDO_CATID_BROADON_RVL	= 1,	// Wii IOS
	NINTENDO_CATID_BROADON_NC	= 2,	// NetCard
	NINTENDO_CATID_BROADON_WUP	= 7,	// vWii IOS
} Nintendo_CatID_IOS_e;

/**
 * Nintendo Title ID: Wii: Category IDs.
 * For SysID == 1 (Wii) and SysID == 7 (vWii).
 */
typedef enum {
	NINTENDO_CATID_RVL_DISC			= 0,	// Disc title
	NINTENDO_CATID_RVL_DOWNLOADED		= 1,	// Downloaded channel
	NINTENDO_CATID_RVL_SYSTEM		= 2,	// System channel
	NINTENDO_CATID_RVL_DISC_WITH_CHANNEL	= 4,	// Disc title with an installable channel
	NINTENDO_CATID_RVL_DLC			= 5,	// DLC
	NINTENDO_CATID_RVL_HIDDEN		= 8,	// Hidden (DVDX, rgnsel, EULA, etc.)
} Nintendo_CatID_RVL_e;

/**
 * Nintendo Title ID: DSi: Category IDs.
 * For SysID == 3.
 */
typedef enum {
	NINTENDO_CATID_TWL_CARTRIDGE		= 0x00,
	NINTENDO_CATID_TWL_DSiWARE		= 0x04,
	NINTENDO_CATID_TWL_SYSTEM_FUN_TOOL	= 0x05,
	NINTENDO_CATID_TWL_NONEXEC_DATA		= 0x0F,
	NINTENDO_CATID_TWL_SYSTEM_BASE_TOOL	= 0x15,
	NINTENDO_CATID_TWL_SYSTEM_MENU		= 0x17,
} Nintendo_CatID_TWL_e;

/**
 * Nintendo Title ID: 3DS: Category IDs.
 * For SysID == 4.
 */
typedef enum {
	// Applications
	NINTENDO_CATID_CTR_APP		= 0x00,		// Application (cart and eShop)
	NINTENDO_CATID_CTR_DLPCHILD	= 0x01,		// Download Play child
	NINTENDO_CATID_CTR_DEMO		= 0x02,		// Demo
	NINTENDO_CATID_CTR_UPDATE	= 0x0E,		// Update
	NINTENDO_CATID_CTR_DLC		= 0x8C,		// DLC

	// System
	NINTENDO_CATID_CTR_SYSAPP	= 0x10,		// System application
	NINTENDO_CATID_CTR_SYSDATA	= 0x1B,		// System data archive
	NINTENDO_CATID_CTR_SYSAPPLET	= 0x30,		// System applet
	NINTENDO_CATID_CTR_SHAREDDATA	= 0x9B,		// Shared data
	NINTENDO_CATID_CTR_SYSDATAVER	= 0xDB,		// System data (mostly version info)
	NINTENDO_CATID_CTR_SYSMODULE	= 0x130,	// System modules
	NINTENDO_CATID_CTR_FIRM		= 0x138,	// System firmware

	// DSi mode
	NINTENDO_CATID_CTR_TWL_FLAG		= 0x8000,	// Flag for DSiWare on 3DS
	NINTENDO_CATID_CTR_TWL_DSiWARE		= NINTENDO_CATID_CTR_TWL_FLAG | NINTENDO_CATID_TWL_DSiWARE,
	NINTENDO_CATID_CTR_TWL_SYSTEM_FUN_TOOL	= NINTENDO_CATID_CTR_TWL_FLAG | NINTENDO_CATID_TWL_SYSTEM_FUN_TOOL,
	NINTENDO_CATID_CTR_TWL_NONEXEC_DATA	= NINTENDO_CATID_CTR_TWL_FLAG | NINTENDO_CATID_TWL_NONEXEC_DATA,
} Nintendo_CatID_CTR_e;

/**
 * Nintendo Title ID: Wii U: Category IDs.
 * For SysID == 5.
 */
typedef enum {
	// Applications
	NINTENDO_CATID_WUP_APP		= 0x00,		// Application (disc and eShop)
	NINTENDO_CATID_WUP_DEMO		= 0x02,		// Demo
	NINTENDO_CATID_WUP_DLC		= 0x0C,		// DLC
	NINTENDO_CATID_WUP_UPDATE	= 0x0E,		// Update

	// System
	NINTENDO_CATID_WUP_SYSAPP	= 0x10,		// System application
	NINTENDO_CATID_WUP_SYSDATA	= 0x1B,		// System data archive
	NINTENDO_CATID_WUP_SYSAPPLET	= 0x30,		// System applet
	NINTENDO_CATID_WUP_SYSUPDATE	= 0x0E,		// System update
} Nintendo_CatID_WUP_e;

#ifdef __cplusplus
}
#endif

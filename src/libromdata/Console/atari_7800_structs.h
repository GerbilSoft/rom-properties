/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * atari_7800_structs.h: Atari 7800 ROM image data structures.             *
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

/**
 * Atari 7800 ROM image file header.
 * NOTE: This is an emulator header; not part of the actual cartridge.
 * Reference: http://7800.8bitdev.org/index.php/A78_Header_Specification
 *
 * All fields are in big-endian.
 */
// NOTE: A78 v3 explicitly uses NULLs, but older files might use spaces.
// We won't check the padding bytes.
#define ATARI_7800_A78_MAGIC		"ATARI7800"
#define ATARI_7800_A78_END_MAGIC	"ACTUAL CART DATA STARTS HERE"
#pragma pack(1)
typedef struct RP_PACKED _Atari_A78Header {
	uint8_t version;		// [0x000] Header version
	char magic[9];			// [0x001] Magic: ATARI_7800_A78_MAGIC
	char magic_padding[7];		// [0x00A] Magic: Padding (NULL for v3)
	char title[32];			// [0x011] Title (ASCII, NULL-terminated)
	uint32_t rom_size;		// [0x031] ROM size, without header
	uint16_t cart_type;		// [0x035] Cartridge type (see Atari_A78_CartType_e)
	uint8_t control_types[2];	// [0x037] Controller 1 and 2 types (see Atari_A78_ControllerType_e)
	uint8_t tv_type;		// [0x039] TV type (see Atari_A78_TVType_e)
	uint8_t save_device;		// [0x03A] Save device (see Atari_A78_SaveDevice_e)
	uint8_t reserved1[4];		// [0x03B]
	uint8_t passthru;		// [0x03F] Slot passthrough device (see Atari_A78_PassThru_e)

	uint8_t reserved2[36];		// [0x040]
	char end_magic[28];		// [0x064] Header end magic-text: ATARI_7800_A78_END_MAGIC
} Atari_A78Header;
ASSERT_STRUCT(Atari_A78Header, 0x80);
#pragma pack()

/**
 * Atari 7800: Cartridge type (bitfield)
 */
typedef enum {
	ATARI_A78_CartType_POKEY_x4000			= (1U <<  0),
	ATARI_A78_CartType_SuperGame_BankSwitched	= (1U <<  1),
	ATARI_A78_CartType_SuperGame_RAM_x4000		= (1U <<  2),
	ATARI_A78_CartType_ROM_x4000			= (1U <<  3),
	ATARI_A78_CartType_Bank6_x4000			= (1U <<  4),
	ATARI_A78_CartType_Banked_RAM			= (1U <<  5),
	ATARI_A78_CartType_POKEY_x450			= (1U <<  6),
	ATARI_A78_CartType_MirrorRAM_x4000		= (1U <<  7),
	ATARI_A78_CartType_ActiVision_Banking		= (1U <<  8),
	ATARI_A78_CartType_Absolute_Banking		= (1U <<  9),
	ATARI_A78_CartType_POKEY_x440			= (1U << 10),
	ATARI_A78_CartType_YM2151_x460_x461		= (1U << 11),
	ATARI_A78_CartType_SOUPER			= (1U << 12),
	ATARI_A78_CartType_Banksets			= (1U << 13),
	ATARI_A78_CartType_Halt_Banked_RAM		= (1U << 14),
	ATARI_A78_CartType_POKEY_x800			= (1U << 15),
} Atari_A78_CartType_e;

/**
 * Atari 7800: Controller type
 */
typedef enum {
	ATARI_A78_ControllerType_None			=  0,
	ATARI_A78_ControllerType_Joystick_7800		=  1,
	ATARI_A78_ControllerType_LightGun		=  2,
	ATARI_A78_ControllerType_Paddle			=  3,
	ATARI_A78_ControllerType_Trakball		=  4,
	ATARI_A78_ControllerType_Joystick_2600		=  5,
	ATARI_A78_ControllerType_Driving_2600		=  6,
	ATARI_A78_ControllerType_Keyboard_2600		=  7,
	ATARI_A78_ControllerType_Mouse_ST		=  8,
	ATARI_A78_ControllerType_Mouse_Amiga		=  9,
	ATARI_A78_ControllerType_AtariVox_SaveKey	= 10,
	ATARI_A78_ControllerType_SNES2Atari		= 11,
} Atari_A78_ControllerType_e;

/**
 * Atari 7800: TV type (bitfield)
 */
typedef enum {
	ATARI_A78_TVType_Format_NTSC		= (0U << 0),
	ATARI_A78_TVType_Format_PAL		= (1U << 0),
	ATARI_A78_TVType_Format_Mask		= (1U << 0),

	ATARI_A78_TVType_Artifacts_Composite	= (0U << 1),
	ATARI_A78_TVType_Artifacts_Component	= (1U << 1),
	ATARI_A78_TVType_Artifacts_Mask		= (1U << 1),
} Atari_A78_TVType_e;

/**
 * Atari 7800: Save device (bitfield)
 */
typedef enum {
	ATARI_A78_SaveDevice_HSC		= (1U << 0),
	ATARI_A78_SaveDevice_AtariVox_SaveKey	= (1U << 1),
} Atari_A78_SaveDevice_e;

/**
 * Atari 7800: Slot passthrough device (bitfield)
 */
typedef enum {
	ATARI_A78_PassThru_XM	= (1U << 0),
} Atari_A78_PassThru_e;

#ifdef __cplusplus
}
#endif

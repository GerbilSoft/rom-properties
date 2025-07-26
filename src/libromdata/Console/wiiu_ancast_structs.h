/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * wiiu_ancast_structs.h: Wii U "Ancast" image structures.                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://wiiubrew.org/wiki/Ancast_image

#pragma once

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// "Toucan" image magic number (BE32)
#define WIIU_TOUCAN_HEADER_MAGIC 0xFD9B5B7A

// "Ancast" image magic number (BE32)
#define WIIU_ANCAST_HEADER_MAGIC 0xEFA282D9

/**
 * Wii U "Ancast" image header: Signature common fields
 * All fields are in big-endian.
 */
typedef struct _WiiU_Ancast_Header_SigCommon_t {
	uint32_t magic;			// [0x000] Magic number
	uint32_t null_0;		// [0x004] NULL
	uint32_t sig_offset;		// [0x008] Signature offset (usually 0x20)
	uint32_t null_1;		// [0x00C] NULL
	uint32_t null_2[4];		// [0x010] NULL
	uint32_t sig_type;		// [0x020] Signature type (see WiiU_Ancast_SigType_e)
} WiiU_Ancast_Header_SigCommon_t;
ASSERT_STRUCT(WiiU_Ancast_Header_SigCommon_t, 0x24);

/**
 * Wii U "Ancast" image header: ARM
 * All fields are in big-endian.
 */
typedef struct _WiiU_Ancast_Header_ARM_t {
	uint32_t magic;			// [0x000] Magic number
	uint32_t null_0;		// [0x004] NULL
	uint32_t sig_offset;		// [0x008] Signature offset (usually 0x20)
	uint32_t null_1;		// [0x00C] NULL
	uint32_t null_2[4];		// [0x010] NULL
	uint32_t sig_type;		// [0x020] Signature type (see WiiU_Ancast_SigType_e)
					//         (should be 2 aka RSA-2048)

	union {
		uint8_t u8[0x100];
		uint32_t u32[0x100/4];
	} signature;			// [0x024] RSA-2048 signature
	uint8_t padding0[0x7C];		// [0x124] Padding (NULL)

	uint16_t null_3;		// [0x1A0] NULL
	uint8_t null_4;			// [0x1A2] NULL
	uint8_t null_5;			// [0x1A3] NULL

	uint32_t target_device;		// [0x1A4] Target device (see WiiU_Target_Device_e)
	uint32_t console_type;		// [0x1A8] Console type (see WiiU_Console_Type_e)
	uint32_t body_size;		// [0x1AC] Ancast image body size
	uint8_t body_hash[20];		// [0x1B0] Ancast image body hash (SHA-1)
	uint32_t version;		// [0x1C4] Version (usually 2)
	uint8_t padding1[0x38];		// [0x1C8] Padding (NULL)
} WiiU_Ancast_Header_ARM_t;
ASSERT_STRUCT(WiiU_Ancast_Header_ARM_t, 0x200);

/**
 * Wii U "Ancast" image header: PowerPC
 * All fields are in big-endian.
 */
typedef struct _WiiU_Ancast_Header_PPC_t {
	uint32_t magic;			// [0x000] Magic number
	uint32_t null_0;		// [0x004] NULL
	uint32_t sig_offset;		// [0x008] Signature offset (usually 0x20)
	uint32_t null_1;		// [0x00C] NULL
	uint32_t null_2[4];		// [0x010] NULL
	uint32_t sig_type;		// [0x020] Signature type (see WiiU_Ancast_SigType_e)
					//         (should be 1 aka ECDSA)

	union {
		uint8_t u8[0x38];
		uint32_t u32[0x38/4];
	} signature;			// [0x024] ECDSA signature
	uint8_t padding0[0x44];		// [0x05C] Padding (NULL)

	uint16_t null_3;		// [0x0A0] NULL
	uint8_t null_4;			// [0x0A2] NULL
	uint8_t null_5;			// [0x0A3] NULL

	uint32_t target_device;		// [0x0A4] Target device (see WiiU_Target_Device_e)
	uint32_t console_type;		// [0x0A8] Console type (see WiiU_Console_Type_e)
	uint32_t body_size;		// [0x0AC] Ancast image body size
	uint8_t body_hash[20];		// [0x0B0] Ancast image body hash (SHA-1)
	uint8_t padding1[0x3C];		// [0x0C4] Padding (NULL)
} WiiU_Ancast_Header_PPC_t;
ASSERT_STRUCT(WiiU_Ancast_Header_PPC_t, 0x100);

/**
 * Wii U "Ancast" image: Signature type
 */
typedef enum {
	WIIU_ANCAST_SIGTYPE_ECDSA	= 0x01,	// ECDSA (PPC images)
	WIIU_ANCAST_SIGTYPE_RSA2048	= 0x02,	// RSA-2048 (ARM images)
} WiiU_Ancast_SigType_e;

/**
 * Wii U "Ancast" image: Target device
 */
typedef enum {
	// PowerPC
	WIIU_ANCAST_TARGET_DEVICE_PPC_WIIU	= 0x11,		// Wii U image
	WIIU_ANCAST_TARGET_DEVICE_PPC_VWII_12	= 0x12,		// "Unknown" vWii image
	WIIU_ANCAST_TARGET_DEVICE_PPC_VWII	= 0x13,		// vWii image
	WIIU_ANCAST_TARGET_DEVICE_PPC_SPECIAL	= 0x14,		// "Special" image

	// ARM
	WIIU_ANCAST_TARGET_DEVICE_ARM_NAND	= 0x21,		// ARM images: NAND boot
	WIIU_ANCAST_TARGET_DEVICE_ARM_SD	= 0x22,		// ARM images: SD boot
} WiiU_Target_Device_e;

/**
 * Wii U "Ancast" image: Console type
 */
typedef enum {
	WIIU_ANCAST_CONSOLE_TYPE_DEVEL	= 1,	// Console type: Development
	WIIU_ANCAST_CONSOLE_TYPE_PROD	= 2,	// Console type: Production
} WiiU_Console_Type_e;

#ifdef __cplusplus
}
#endif

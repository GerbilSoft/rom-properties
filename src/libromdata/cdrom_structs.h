/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cdrom_structs.h: CD-ROM data structures.                                *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * References:
 * - https://github.com/qeedquan/ecm/blob/master/format.txt
 * - https://github.com/Karlson2k/libcdio-k2k/blob/master/include/cdio/sector.h
 * - https://problemkaputt.de/psx-spx.htm#cdromxasubheaderfilechannelinterleave
 */

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CDROM_FRAMES_PER_SEC	75
#define CDROM_SECS_PER_MIN	60

/**
 * MSF address.
 * Each value is encoded as BCD.
 */
// Some compilers pad this structure to a multiple of 4 bytes
#pragma pack(1)
typedef struct PACKED _CDROM_MSF_t {
	uint8_t min;
	uint8_t sec;	// 60
	uint8_t frame;	// 75
} CDROM_MSF_t;
ASSERT_STRUCT(CDROM_MSF_t, 3);
#pragma pack()

/**
 * Convert an MSF address to LBA.
 * Removes the 150-block lead in.
 * @param msf MSF.
 * @return LBA.
 */
static FORCEINLINE unsigned int cdrom_msf_to_lba(const CDROM_MSF_t *msf)
{
	// NOTE: Not verifying BCD here.
	unsigned int lba = ((msf->frame >> 4) * 10) | (msf->frame & 0x0F);
	lba += ((((msf->sec >> 4) * 10) | (msf->sec & 0x0F)) * CDROM_FRAMES_PER_SEC);
	lba += ((((msf->min >> 4) * 10) | (msf->min & 0x0F)) * CDROM_FRAMES_PER_SEC * CDROM_SECS_PER_MIN);
	return lba - 150;
}

/**
 * CD-ROM Mode 2 XA subheader struct.
 * NOTE: Subheader only has four significant bytes. These bytes
 * are duplicated for reliability.
 */
typedef union _CDROM_Mode2_XA_Subheader {
	struct {
		uint8_t file_number;	// [0x000] File number (0x00-0xFF)
		uint8_t channel_number;	// [0x001] Channel number (0x00-0x1F)
		uint8_t submode;	// [0x002] Submode (see CDROM_Mode2_XA_Submode_Flags_e)
		uint8_t codinginfo;	// [0x003] Coding info (see CDROM_Mode2_XA_CodingInfo_Flags_e)
	};
	uint8_t data[2][4];		// Raw data. [0][x] is primary, [1][x] is duplicate.
} CDROM_Mode2_XA_Subheader;
ASSERT_STRUCT(CDROM_Mode2_XA_Subheader, 8);

/**
 * CD-ROM Mode 2 XA submode flags.
 */
typedef enum {
	CDROM_MODE2_XA_SUBMODE_EOR	= (1U << 0),	// End of Record

	// Only one of these three flags may be set to indicate the sector type.
	CDROM_MODE2_XA_SUBMODE_VIDEO		= (1U << 1),	// Video sector
	CDROM_MODE2_XA_SUBMODE_AUDIO		= (1U << 2),	// Audio sector
	CDROM_MODE2_XA_SUBMODE_DATA		= (1U << 3),	// Data sector
	CDROM_MODE2_XA_SUBMODE_TYPE_MASK	= (7U << 1),

	CDROM_MODE2_XA_SUBMODE_TRIGGER		= (1U << 4),
	CDROM_MODE2_XA_SUBMODE_FORM2		= (1U << 5),	// If set, uses Form2.
	CDROM_MODE2_XA_SUBMODE_REAL_TIME	= (1U << 6),
	CDROM_MODE2_XA_SUBMODE_EOF		= (1U << 7),	// End of File
} CDROM_Mode2_Submode_Flags_e;

/**
 * CD-ROM Mode 2 XA coding info flags. (ADPCM sectors only)
 */
typedef enum {
	// Mono/Stereo (2-bit field)
	CDROM_MODE2_XA_CODINGINFO_MONO		= (0U << 0),	// Mono
	CDROM_MODE2_XA_CODINGINFO_STEREO	= (1U << 0),	// Stereo
	CDROM_MODE2_XA_CODINGINFO_RSV1		= (2U << 0),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_RSV2		= (3U << 0),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_MASK		= (3U << 0),

	// Sample rate (2-bit field)
	CDROM_MODE2_XA_CODINGINFO_RATE_37800	= (0U << 2),	// 37,800 Hz
	CDROM_MODE2_XA_CODINGINFO_RATE_18900	= (1U << 2),	// 18,900 Hz
	CDROM_MODE2_XA_CODINGINFO_RATE_RSV1	= (2U << 2),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_RATE_RSV2	= (3U << 2),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_RATE_MASK	= (3U << 2),

	// Bits per sample (2-bit field)
	CDROM_MODE2_XA_CODINGINFO_BPS_4		= (0U << 4),	// 4 bits per sample (standard)
	CDROM_MODE2_XA_CODINGINFO_BPS_8		= (1U << 4),	// 8 bits per sample
	CDROM_MODE2_XA_CODINGINFO_BPS_RSV1	= (2U << 4),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_BPS_RSV2	= (3U << 4),	// Reserved
	CDROM_MODE2_XA_CODINGINFO_BPS_MASK	= (3U << 4),

	CDROM_MODE2_XA_CODINGINFO_EMPHASIS	= (1U << 6),	// 0=Normal, 1=Emphasis

	CDROM_MODE2_XA_CODINGINFO_RSV7		= (1U << 7),	// Reserved
} CDROM_Mode2_XA_CodingInfo_Flags_e;

/**
 * 2352-byte sector format.
 */
typedef struct _CDROM_2352_Sector_t {
	uint8_t sync[12];	// 00 FF FF FF FF FF FF FF FF FF FF 00
	CDROM_MSF_t msf;	// Sector address.
	uint8_t mode;		// Sector mode.

	// Sector data formats.
	union {
		struct {
			uint8_t data[2048];
			uint8_t edc[4];
			uint8_t zero[8];
			uint8_t ecc[276];
		} m1;
		struct {
			uint8_t data[2336];
		} m2;
		struct {
			CDROM_Mode2_XA_Subheader sub;
			uint8_t data[2048];
			uint8_t edc[4];
			uint8_t ecc[276];
		} m2xa_f1;
		struct {
			CDROM_Mode2_XA_Subheader sub;
			uint8_t data[2324];
			uint8_t edc[4];
		} m2xa_f2;
	};
} CDROM_2352_Sector_t;
ASSERT_STRUCT(CDROM_2352_Sector_t, 2352);

/**
 * Get the start of the user data section of a raw CD-ROM sector.
 *
 * NOTES:
 * - Assuming 2048 bytes of user data.
 * - If XA data support is needed, the caller should check for Mode 2 manually.
 * - The return value from audio sectors is undefined and may correlate
 *   to either Mode 1 or Mode 2, depending on the audio data.
 * - Empty sectors ("Mode 0") will act like Mode 1.
 * @param sector Raw CD-ROM sector.
 * @return Start of user data section.
 */
static inline constexpr const uint8_t *cdromSectorDataPtr(const CDROM_2352_Sector_t *sector)
{
	return (unlikely(sector->mode == 2))
		? sector->m2xa_f1.data
		: sector->m1.data;
}

#ifdef __cplusplus
}
#endif

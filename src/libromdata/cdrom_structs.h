/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * cdrom_structs.h: CD-ROM data structures.                                *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CDROM_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_CDROM_STRUCTS_H__

/**
 * References:
 * - https://github.com/qeedquan/ecm/blob/master/format.txt
 * - https://github.com/Karlson2k/libcdio-k2k/blob/master/include/cdio/sector.h
 */

#include <stdint.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

#define CDROM_FRAMES_PER_SEC	75
#define CDROM_SECS_PER_MIN	60

/**
 * MSF address.
 * Each value is encoded as BCD.
 */
typedef struct PACKED _CDROM_MSF_t {
	uint8_t min;
	uint8_t sec;	// 60
	uint8_t frame;	// 75
} CDROM_MSF_t;
ASSERT_STRUCT(CDROM_MSF_t, 3);

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
 * 2352-byte sector format.
 */
typedef struct PACKED _CDROM_2352_Sector_t {
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
			uint8_t sub[8];
			uint8_t data[2048];
			uint8_t edc[4];
			uint8_t ecc[276];
		} m2xa_f1;
		struct {
			uint8_t sub[8];
			uint8_t data[2324];
			uint8_t spare[4];
		} m2xa_f2;
	};
} CDROM_2352_Sector_t;
ASSERT_STRUCT(CDROM_2352_Sector_t, 2352);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_CDROM_STRUCTS_H__ */

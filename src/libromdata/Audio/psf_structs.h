/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * psf_structs.h: PSF audio data structures.                               *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

// References:
// - http://fileformats.archiveteam.org/wiki/Portable_Sound_Format
// - http://wiki.neillcorlett.com/PSFFormat
//   - ARCHIVED: https://web.archive.org/web/20100610021754/http://wiki.neillcorlett.com/PSFFormat
// - http://wiki.neillcorlett.com/PSFTagFormat
//   - ARCHIVED: https://web.archive.org/web/20100510040327/http://wiki.neillcorlett.com:80/PSFTagFormat

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_PSF_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_PSF_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * Portable Sound Format.
 * All fields are little-endian.
 */
#define PSF_MAGIC "PSF"
#define PSF_TAG_MAGIC "[TAG]"
typedef struct PACKED _PSF_Header {
	char magic[3];			// [0x000] "PSF"
	uint8_t version;		// [0x003] Version. Identifies the system. (See PSF_Version_e.)
	uint32_t reserved_size;		// [0x004] Size of reserved area. (R)
	uint32_t compressed_prg_length;	// [0x008] Compressed program length. (N)
	uint32_t compressed_prg_crc32;	// [0x00C] CRC32 of compressed program data.
} PSF_Header;
ASSERT_STRUCT(PSF_Header, 16);

/**
 * PSF: Version. (System)
 */
typedef enum {
	PSF_VERSION_PLAYSTATION		= 0x01,	// PSF1
	PSF_VERSION_PLAYSTATION_2	= 0x02,	// PSF2
	PSF_VERSION_SATURN		= 0x11,	// SSF
	PSF_VERSION_DREAMCAST		= 0x12,	// DSF
	PSF_VERSION_MEGA_DRIVE		= 0x13,
	PSF_VERSION_N64			= 0x21,	// USF
	PSF_VERSION_GBA			= 0x22,	// GSF
	PSF_VERSION_SNES		= 0x23,	// SNSF
	PSF_VERSION_QSOUND		= 0x41,	// QSF
} PSF_Version_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_PSF_STRUCTS_H__ */

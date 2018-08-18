/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nsf_structs.h: SID audio data structures.                               *
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
// - https://www.hvsc.c64.org/download/C64Music/DOCUMENTS/SID_file_format.txt

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_SID_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_SID_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * PlaySID file format. (Commodore 64)
 * All fields are big-endian.
 *
 * NOTE: Field names match the documentation from HVSC:
 * https://www.hvsc.c64.org/download/C64Music/DOCUMENTS/SID_file_format.txt
 */
#define PSID_MAGIC 'PSID'
#define RSID_MAGIC 'RSID'
typedef struct PACKED _SID_Header {
	/** Begin PSID v1 header **/
	uint32_t magic;			// [0x000] 'PSID' or 'RSID'
	uint16_t version;		// [0x004] Version.
	uint16_t dataOffset;		// [0x006] Data offset.
	uint16_t loadAddress;		// [0x008] Load address.
	uint16_t initAddress;		// [0x00A] Init address.
	uint16_t playAddress;		// [0x00C] Play address.
	uint16_t songs;			// [0x00E] Number of songs.
	uint16_t startSong;		// [0x010] Default song number.
	uint32_t speed;			// [0x012] Speed. (one bit per track; up to 32 tracks)
					// - 0-bit: uses VBlank interrupt (50 Hz PAL; 60 Hz NTSC)
					// - 1-bit: uses CIA 1 timer interrupt (default 60 Hz)

	// Tag fields. (ASCII; might not be NULL-terminated.)
	char name[32];			// [0x016] Name.
	char author[32];		// [0x036] Author.
	char copyright[32];		// [0x056] Copyright, aka "released".
	/** End PSID v1 header **/

	// TODO: PSID v2+
} SID_Header;
ASSERT_STRUCT(SID_Header, 118);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_SID_STRUCTS_H__ */

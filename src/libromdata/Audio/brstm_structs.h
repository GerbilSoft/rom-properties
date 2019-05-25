/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * brstm_structs.h: Nintendo Wii BRSTM audio data structures.              *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_BRSTM_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_BRSTM_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * BRSTM chunk information.
 * Endianness depends on the byte-order mark.
 */
typedef struct _BRSTM_ChunkInfo {
	uint32_t offset;	// [0x000] Offset
	uint32_t size;		// [0x004] Size
} BRSTM_ChunkInfo;
ASSERT_STRUCT(BRSTM_ChunkInfo, 8);

/**
 * BRSTM header.
 * This matches the BRSTM header format exactly.
 * Reference: https://wiibrew.org/wiki/BRSTM_file
 *
 * Offsets in the BRSTM header are absolute addresses.
 * (aka: relative to the start of the BRSTM header)
 *
 * Endianness depends on the byte-order mark.
 */
#define BRSTM_MAGIC 'RSTM'
#define BRSTM_BOM_HOST 0xFEFF	// UTF-8 BOM; matches host-endian.
#define BRSTM_BOM_SWAP 0xFFFE	// UTF-8 BOM; matches swapped-endian.
typedef struct PACKED _BRSTM_Header {
	uint32_t magic;		// [0x000] 'RSTM'
	uint16_t bom;		// [0x004] Byte-order mark
	uint8_t version_major;	// [0x006] Major version (1)
	uint8_t version_minor;	// [0x007] Minor version (0)
	uint32_t file_size;	// [0x008] Size of the whole file
	uint16_t header_size;	// [0x00C] Header size (Usually 0x40; must be at least 0x28)

	// Chunks
	// If a chunk offset or size is 0, it doesn't exist.
	// HEAD and DATA chunks must exist.
	// ADPC chunk is optional.
	uint16_t chunk_count;	// [0x00E] Number of chunks
	BRSTM_ChunkInfo head;	// [0x010] HAED chunk
	BRSTM_ChunkInfo adpc;	// [0x018] ADPC chunk
	BRSTM_ChunkInfo data;	// [0x020] DATA chunk

	// There's usually 24 bytes of padding here,
	// but we'll leave that out.
} BRSTM_Header;
ASSERT_STRUCT(BRSTM_Header, 0x28);

/**
 * HEAD chunk header.
 * This contains offsets to the various HEAD chunk parts.
 *
 * Offsets in the HEAD chunk are relative to HEAD+0x008.
 *
 * Endianness depends on the byte-order mark.
 */
#define BRSTM_HEAD_MAGIC 'HEAD'
#define BRSTM_HEAD_MARKER 0x01000000
typedef struct PACKED _BRSTM_HEAD_Header {
	uint32_t magic;		// [0x000] 'HEAD'
	uint32_t size;		// [0x004] Size of entire HEAD section.
	uint32_t marker1;	// [0x008] Marker? (0x01000000)
	uint32_t head1_offset;	// [0x00C] HEAD chunk, part 1
	uint32_t marker2;	// [0x008] Marker? (0x01000000)
	uint32_t head2_offset;	// [0x00C] HEAD chunk, part 2
	uint32_t marker3;	// [0x008] Marker? (0x01000000)
	uint32_t head3_offset;	// [0x00C] HEAD chunk, part 3
} BRSTM_HEAD_Header;
ASSERT_STRUCT(BRSTM_HEAD_Header, 32);

/**
 * HEAD chunk, part 1.
 * This is the only HEAD chunk with useful metadata,
 * so we're not including others.
 *
 * Endianness depends on the byte-order mark.
 */
typedef struct PACKED _BRSTM_HEAD_Chunk1 {
	uint8_t codec;				// [0x000] See BRSTM_Codec_e
	uint8_t loop_flag;			// [0x001] Loop flag
	uint8_t channel_count;			// [0x002] Number of channels
	uint8_t padding1;			// [0x003] Padding? (0x00)
	uint16_t sample_rate;			// [0x004] Sample rate
	uint8_t padding2[2];			// [0x006] Padding? (0x00)
	uint32_t loop_start;			// [0x008] Loop start, in samples
	uint32_t sample_count;			// [0x00C] Total sample count
	uint32_t adpcm_offset;			// [0x010] Absolute offset to the beginning of the ADPCM data
	uint32_t adpcm_block_count;		// [0x014] Total number of interlaced blocks in ADPCM data,
						//         including the final block.
	uint32_t block_size;			// [0x018] Block size, in bytes
	uint32_t samples_per_block;		// [0x01C] Samples per block
	uint32_t final_block_size;		// [0x020] Size of the final block (without padding), in bytes
	uint32_t final_block_samples;		// [0x024] Number of samples in the final block
	uint32_t final_block_size_pad;		// [0x028] Size of the final block (with padding), in bytes
	uint32_t adpc_samples_per_entry;	// [0x02C] Samples per entry in the ADPC table
	uint32_t adpc_bytes_per_entry;		// [0x030] Bytes per entry in the ADPC table
} BRSTM_HEAD_Chunk1;
ASSERT_STRUCT(BRSTM_HEAD_Chunk1, 0x34);

/**
 * BRSTM codecs.
 */
typedef enum {
	BRSTM_CODEC_PCM_S8	= 0,	// Signed 8-bit PCM
	BRSTM_CODEC_PCM_S16	= 1,	// Signed 16-bit PCM
	BRSTM_CODEC_ADPCM_THP	= 2,	// 4-bit ADPCM
} BRSTM_Codec_e;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_BRSTM_STRUCTS_H__ */

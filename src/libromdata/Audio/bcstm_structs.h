/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * bcstm_structs.h: Nintendo 3DS BCSTM audio data structures.              *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_AUDIO_BCSTM_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_AUDIO_BCSTM_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * BCSTM reference.
 * Indicates the block type.
 *
 * Endianness depends on the byte-order mark.
 */
typedef struct PACKED _BCSTM_Reference {
	uint16_t type_id;	// [0x000] Type ID (See BCSTM_Block_Type_e)
	uint16_t padding;	// [0x002] Padding
	uint32_t offset;	// [0x004] Offset (~0 for "null")
} BCSTM_Reference;
ASSERT_STRUCT(BCSTM_Reference, 8);

/**
 * BCSTM block types.
 */
typedef enum {
	BCSTM_BLOCK_TYPE_BYTE_TABLE	= 0x0100,
	BCSTM_BLOCK_TYPE_REF_TABLE	= 0x0101,
	BCSTM_BLOCK_TYPE_ADPCM_DSP_INFO	= 0x0300,
	BCSTM_BLOCK_TYPE_ADPCM_IMA_INFO	= 0x0301,
	BCSTM_BLOCK_TYPE_SAMPLE_DATA	= 0x1F00,
	BCSTM_BLOCK_TYPE_INFO_BLOCK	= 0x4000,
	BCSTM_BLOCK_TYPE_SEEK_BLOCK	= 0x4001,
	BCSTM_BLOCK_TYPE_DATA_BLOCK	= 0x4002,
	BCSTM_BLOCK_TYPE_STREAM_INFO	= 0x4100,
	BCSTM_BLOCK_TYPE_TRACK_INFO	= 0x4101,
	BCSTM_BLOCK_TYPE_CHANNEL_INFO	= 0x4102,
} BCSTM_Block_Type_e;

/**
 * BCSTM sized reference.
 * Indicates the block type and offset.
 *
 * Endianness depends on the byte-order mark.
 */
typedef struct PACKED _BCSTM_SizedRef {
	BCSTM_Reference ref;	// [0x000] Reference
	uint32_t size;		// [0x008] Size
} BCSTM_SizedRef;
ASSERT_STRUCT(BCSTM_SizedRef, 12);

/**
 * BCSTM header.
 * This matches the BCSTM header format exactly.
 * Reference: https://www.3dbrew.org/wiki/BCSTM
 *
 * Offsets in the BCSTM header are absolute addresses.
 * (aka: relative to the start of the BCSTM header)
 *
 * Endianness depends on the byte-order mark.
 */
#define BCSTM_MAGIC 'CSTM'
#define BCSTM_BOM_HOST 0xFEFF	// UTF-8 BOM; matches host-endian.
#define BCSTM_BOM_SWAP 0xFFFE	// UTF-8 BOM; matches swapped-endian.
#define BCSTM_VERSION 0x02000000
typedef struct PACKED _BCSTM_Header {
	uint32_t magic;		// [0x000] 'CSTM'
	uint16_t bom;		// [0x004] Byte-order mark
	uint16_t header_size;	// [0x006] Header size (0x40 due to Info Block alignment)
	uint32_t version;	// [0x008] Version (0x02000000)
	uint32_t file_size;	// [0x00C] Size of the whole file
	uint16_t block_count;	// [0x010] Number of blocks (3)
	uint8_t reserved[2];	// [0x012]

	// Sized References
	// Offsets are relative to the start of the file.
	BCSTM_SizedRef info;	// [0x014] Info block
	BCSTM_SizedRef seek;	// [0x020] Seek block
	BCSTM_SizedRef data;	// [0x02C] Data block
} BCSTM_Header;
ASSERT_STRUCT(BCSTM_Header, 0x38);

/**
 * Stream Info struct.
 * This struct is fully contained within the INFO block.
 *
 * Endianness depends on the byte-order mark.
 */
typedef struct PACKED _BCSTM_Stream_Info {
	uint8_t codec;			// [0x000] Codec (See BCSTM_Codec_e)
					//         (listed as Encoding on 3dbrew)
	uint8_t loop_flag;		// [0x001] Loop flag
	uint8_t channel_count;		// [0x002] Channel count
	uint8_t padding;		// [0x003]
	uint32_t sample_rate;		// [0x004] Sample rate
	uint32_t loop_start;		// [0x008] Loop start frame
	uint32_t loop_end;		// [0x00C] Loop end frame

	uint32_t sample_block_count;	// [0x010] Sample block count
	uint32_t sample_block_size;	// [0x014] Sample block size
	uint32_t sample_block_sample_count;	// [0x018] Sample block sample count

	uint32_t last_sample_block_size;	// [0x01C] Last sample block size
	uint32_t last_sample_block_sample_count; // [0x20] Last sample block sample count
	uint32_t last_sample_block_padded_size;	// [0x024] Last sample block padded size

	uint32_t seek_data_size;		// [0x028] Seek data size
	uint32_t seek_interval_sample_count;	// [0x02C] Seek interval count
	BCSTM_Reference sample_data;		// [0x030] Sample data reference
						//         (relative to Data Block field)
} BCSTM_Stream_Info;
ASSERT_STRUCT(BCSTM_Stream_Info, 0x38);

/**
 * BCSTM codecs.
 */
typedef enum {
	BCSTM_CODEC_PCM_S8	= 0,	// Signed 8-bit PCM
	BCSTM_CODEC_PCM_S16	= 1,	// Signed 16-bit PCM
	BCSTM_CODEC_ADPCM_DSP	= 2,	// DSP ADPCM
	BCSTM_CODEC_ADPCM_IMA	= 3,	// IMA ADPCM
} BCSTM_Codec_e;

/**
 * INFO block.
 * This contains references to other fields.
 *
 * Note that the full block is not included, since
 * the track and channel info tables are variable-length.
 *
 * Endianness depends on the byte-order mark.
 */
#define BCSTM_INFO_MAGIC 'INFO'
typedef struct PACKED _BCSTM_INFO_Block {
	uint32_t magic;				// [0x000] 'INFO'
	uint32_t size;				// [0x004] Size of the info blokc.
	BCSTM_Reference stream_info_ref;	// [0x008] Stream Info
	BCSTM_Reference track_info_ref;		// [0x010] Track Info
	BCSTM_Reference channel_info_ref;	// [0x018] Channel Info

	// Stream Info.
	// NOTE: This is fully contained within the INFO block,
	// even though there's a reference field listed above.
	BCSTM_Stream_Info stream_info;		// [0x020] Stream Info

	// The remainder of the INFO block is variable-length.
} BCSTM_INFO_Block;
ASSERT_STRUCT(BCSTM_INFO_Block, 0x58);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_AUDIO_BCSTM_STRUCTS_H__ */

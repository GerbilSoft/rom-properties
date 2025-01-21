/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * scsi_ata_cmd.h: SCSI/ATA wrapper commands.                              *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://www.smartmontools.org/static/doxygen/scsiata_8cpp_source.html
 * - https://wisesciencewise.wordpress.com/2017/07/13/reading-and-writing-using-ata-pass-through-command/
 * - https://www.t10.org/ftp/t10/document.04/04-262r8.pdf
 */

#pragma once

#include "scsi_protocol.h"
#include "ata_protocol.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: All fields must be byteswapped to big-endian before
// sending the commands.

/** ATA PASS THROUGH(16) **/
// This command uses 48-bit LBA addressing.

typedef struct RP_PACKED _SCSI_CDB_ATA_PASS_THROUGH_16 {
	uint8_t OpCode;		/* ATA PASS THROUGH(16) (0x85) */
	uint8_t ATA_Flags0;	/* multiple_count, protocol, extend */
	uint8_t ATA_Flags1;	/* offline, ck_cond, t_dir, byte_block, t_length */

	// ATA command
	struct {
		uint16_t Feature;
		uint16_t Sector_Count;
		uint16_t LBA_low;
		uint16_t LBA_mid;
		uint16_t LBA_high;
		uint8_t Device;
		uint8_t Command;
	} ata;

	uint8_t Control;
} SCSI_CDB_ATA_PASS_THROUGH_16;

/** ATA PASS THROUGH(12) **/
// This command uses 28-bit LBA addressing.

typedef struct RP_PACKED SCSI_CDB_ATA_PASS_THROUGH_12 {
	uint8_t OpCode;		/* ATA PASS THROUGH(12) (0xA1) (clashes with MMC BLANK) */
	uint8_t ATA_Flags0;	/* multiple_count, protocol, extend */
	uint8_t ATA_Flags1;	/* offline, ck_cond, t_dir, byte_block, t_length */

	// ATA command
	struct {
		uint8_t Feature;
		uint8_t Sector_Count;
		uint8_t LBA_low;
		uint8_t LBA_mid;
		uint8_t LBA_high;
		uint8_t Device;
		uint8_t Command;
	} ata;

	uint8_t Reserved;
	uint8_t Control;
} SCSI_CDB_ATA_PASS_THROUGH_12;

/**
 * ata_flags0 macro.
 * @param multiple_count	pow2 of number of sectors transferred per DRQ data block
 * @param protocol		ATA protocol
 * @param extend		If 1, this is a 48-bit ATA command; otherwise, 28-bit.
 */
#define ATA_FLAGS0(multiple_count, protocol, extend) \
	((((multiple_count) & 0x7) << 5) | \
	 (((protocol) & 0xF) << 1) | \
	 ((extend) & 1))

/**
 * ata_flags1 macro.
 * TODO: off_line field?
 * @param chk_cond	1 = read register(s) back
 * @param t_dir		0 = to device; 1 = from device
 * @param byte_block	0 = bytes; 1 = 512-byte blocks
 * @param t_length	0 = no data transferred; 1 = FEATURE; 2 = SECTOR_COUNT; 3 = STPSIU
 */
#define ATA_FLAGS1(chk_cond, t_dir, byte_block, t_length) \
	((((chk_cond) & 1) << 5) | \
	 (((t_dir) & 1) << 3) | \
	 (((byte_block) & 1) << 2) | \
	 ((t_length) & 3))

#define T_DIR_OUT		0
#define T_DIR_IN		1

#define LEN_BYTES		0
#define LEN_BLOCKS		1

#define T_LENGTH_NONE		0
#define T_LENGTH_FEATURE	1
#define T_LENGTH_SECTOR_COUNT	2
#define T_LENGTH_STPSIU		3

#ifdef __cplusplus
}
#endif

#pragma pack()

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * ata_protocol.h: ATA protocol definitions.                               *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://www.smartmontools.org/static/doxygen/scsiata_8cpp_source.html
 * - http://www.t13.org/documents/uploadeddocuments/docs2006/d1699r3f-ata8-acs.pdf
 */

#pragma once

#include <stdint.h>

/* ATA structs are defined on the byte-level, so we must
 * prevent the compiler from adding alignment padding. */
#if !defined(PACKED)
# if defined(__GNUC__)
#  define PACKED __attribute__((packed))
# else
#  define PACKED
# endif /* defined(__GNUC__) */
#endif /* !defined(PACKED) */

// TODO: Remove packing?
#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ATA command struct.
 *
 * For 28-bit addressing, use the low 8 bits of each
 * LBA field, plus the low 4 bits of device.
 *
 * For 48-bit addressing, use all 16 bits of each
 * LBA field and don't use the device field.
 */
typedef struct PACKED _ATA_CMD {
	uint16_t Feature;	// [0x000] Feature
	uint16_t Sector_Count;	// [0x002] Sector count
	uint16_t LBA_low;	// [0x004] LBA low
	uint16_t LBA_mid;	// [0x006] LBA mid
	uint16_t LBA_high;	// [0x008] LBA high
	uint8_t Device;		// [0x00A] Device
	uint8_t Command;	// [0x00B] Command
} ATA_CMD;

/** ATA protocols **/

#define ATA_PROTOCOL_HARD_RESET		0x00
#define ATA_PROTOCOL_SRST		0x01
#define ATA_PROTOCOL_RESERVED		0x02
#define ATA_PROTOCOL_NON_DATA		0x03
#define ATA_PROTOCOL_PIO_DATA_IN	0x04
#define ATA_PROTOCOL_PIO_DATA_OUT	0x05
#define ATA_PROTOCOL_DMA		0x06
#define ATA_PROTOCOL_DMA_QUEUED		0x07
#define ATA_PROTOCOL_DEVICE_DIAGNOSTIC	0x08
#define ATA_PROTOCOL_DEVICE_RESET	0x09
#define ATA_PROTOCOL_UDMA_DATA_IN	0x0A
#define ATA_PROTOCOL_UDMA_DATA_OUT	0x0B
#define ATA_PROTOCOL_FPDMA		0x0C
#define ATA_PROTOCOL_RETURN_RESPONSE	0x0F

/** ATA commands **/

#define ATA_CMD_READ_DMA_EXT			0x25
#define ATA_CMD_WRITE_DMA_EXT			0x35

// IDENTIFY DEVICE (PIO Data-In)
#define ATA_CMD_IDENTIFY_DEVICE			0xEC
#define ATA_PROTO_IDENTIFY_DEVICE		ATA_PROTOCOL_PIO_DATA_IN

// IDENTIFY DEVICE response.
// NOTE: All offsets are in WORDs.
// NOTE: ATA strings use ASCII encoding and are byteswapped.
// NOTE: Some fields have been converted to uint32_t or uint64_t
// to make it easier to access. These must be swapped from native
// format before use.
typedef struct PACKED _ATA_RESP_IDENTIFY_DEVICE {
	// TODO: Find obsolete info.

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t config;		// 0
	uint16_t obsolete_001;		// 1
	uint16_t specific_config;	// 2
	uint16_t obsolete_003;		// 3
	uint16_t retired_004[2];	// 4-5
	uint16_t obsolete_006;		// 6

	/** IDENTIFY DEVICE only **/
	uint16_t compact_flash_007[2];	// 7-8: Reserved for CompactFlash.

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t obsolete_009;		// 9
	char serial_number[20];		// 10-19: Serial number (ATA string)
					//        NOTE: Usually right-aligned.
	uint16_t retired_020[2];	// 20-21
	uint16_t obsolete_22;		// 22
	char firmware_revision[8];	// 23-26: Firmware revision (ATA string)
	char model_number[40];		// 27-46: Model number (ATA string)

	/** IDENTIFY DEVICE only **/
	uint16_t max_sectors_per_drq;	// 47: 15:8 80h
					//      7:0 00h = Reserved
					//          01h-FFh = Maximum number of
					//          logical sectros that shall
					//          be transferred per DRQ data
					//          block on READ/WRITE MULTIPLE
					//          commands.
	uint16_t tcg;			// 48: Trusted Computing feature set

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t capabilities_049;	// 49: Capabilities
	uint16_t capabilities_050;	// 50: Capabilities
	uint16_t obsolete_051[2];	// 51-52
	uint16_t feature_64_70_88;	// 53: 15:8 Reserved for e06114
					//        2 == 1 if word 88 is valid
					//        1 == 1 if words 70:64 are valid
					//        0 == obsolete

	/** IDENTIFY DEVICE only **/
	uint16_t obsolete_054[5];	// 54-58
	uint16_t sectors_per_drq;	// 59: 15:9 Reserved
					//        8 1 = Multiple sector setting is valid
					//      7:0 Current setting for number of
					//          logical sectors transferred per
					//          DRQ data block on READ/WRITE
					//          MULTIPLE commands.
	uint32_t total_sectors;		// 60-61: Total number of user addressable
					//        logical sectors. (28-bit)

	/** IDENTIFY PACKET DEVICE only **/
	uint16_t dmadir;		// 62: DMA direction

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t mdma_modes;		// 63: Multiword DMA mode capability
					//     and selection.
	uint16_t pio_modes;		// 64: PIO modes supported
	uint16_t mdma_time_min;		// 65: Minimum MDMA transfer cycle time
					//     per word, in nanoseconds.
	uint16_t mdma_time_rec;		// 66: Manufacturer's recommended MDMA
					//     transfer cycle time, in nanoseconds.
	uint16_t pio_time_noflow_min;	// 67: Minimum PIO transfer cycle time
					//     without flow control, in nanoseconds.
	uint16_t pio_time_iordy_min;	// 68: Minimum PIO transfer cycle time
					//     with IODRY flow control, in nanoseconds.
	uint16_t reserved_069[2];	// 69-70
	uint16_t reserved_packet[4];	// 71-74: Reserved for IDENTIFY PACKET DEVICE.

	/** IDENTIFY DEVICE only **/
	uint16_t queue_depth;		// 75: Queue depth

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t sata_capabilities;	// 76: SATA capabilities
	uint16_t reserved_sata_077;	// 77: Reserved for Serial ATA
	uint16_t sata_supported;	// 78: SATA features supported
	uint16_t sata_enabled;		// 79: SATA features enabled
	uint16_t major_revision;	// 80: Major revision number
	uint16_t minor_revision;	// 81: Minor revision number
	uint16_t cmd_sets_support[6];	// 82-87: Command sets supported
	uint16_t udma_modes;		// 88: Ultra DMA modes
	uint16_t sec_erase_time;	// 89: Time required for security erase
					//     unit completion.
	uint16_t enh_sec_erase_time;	// 90: Time required for Enhanced security
					//     erase completion.

	/** IDENTIFY DEVICE only **/
	uint16_t apm_value;		// 91: Current APM value

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t master_password_id;	// 92: Master Password identifier
	uint16_t hw_reset_result;	// 93: Hardware reset result
	uint16_t acoustic_mgmt;		// 94: Acoustic management value

	/** IDENTIFY DEVICE only **/
	uint16_t stream_min_req_size;	// 95: Stream minimum request size
	uint16_t stream_transfer_time_dma;	// 96: Streaming transfer time (DMA)
	uint16_t stream_access_latency;		// 97: Streaming access latency
						//     (DMA and PIO)
	uint16_t stream_perf_granularity[2];	// 98-99: Streaming performance
						//        granularity
	uint64_t total_sectors_48;		// 100-103: Total number of sectors
						//          in 48-bit LBA mode.
	uint16_t stream_transfer_time_pio;	// 104: Streaming transfer time (PIO)
	uint16_t reserved_105;			// 105
	uint16_t logical_sector_size_info;	// 106: Logical sector size info
	uint16_t iso7779_delay;		// 107: Inter-seek delay for ISO-7779
					//      acoustic testing, in microseconds.

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t unique_id[4];		// 108-111: NAA, OUI, unique ID
	uint16_t wwn_128_ext[4];	// 112-115: Reserved for WWN 128-bit extension.
	uint16_t incits_tr_37_2004;	// 116: Reserved for INCITS TR-37-2004.
	uint16_t logical_sector_size[2];	// 117-118: Logical sector size,
						//          in words.
	uint16_t cmd_sets_support2[2];	// 119-120: Command sets supported
	uint16_t reserved_121[4];	// 121-124

	/** IDENTIFY PACKET DEVICE only **/
	uint16_t atapi_byte_count;	// 125

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t reserved_126[2];	// 126-127

	uint16_t security_status;	// 128: Security status
	uint16_t vendor_specific[31];	// 129-159: Vendor specific

	/** IDENTIFY DEVICE only **/
	uint16_t cfa_power_mode_1;	// 160: CFA power mode 1
	uint16_t compact_flash_161[15];	// 161-175: Reserved for CompactFlash.
	char media_serial_number[60];	// 176-205: Current media serial number.
					//          (ATA string)
	uint16_t sct_command_transport;	// 206: SCT Command Transport
	uint16_t ce_ata_207[2];		// 207-208: Reserved for CE-ATA.
	uint16_t logical_block_align;	// 209: Alignment of logical blocks within
					//      a larger physical block.
	uint16_t wrv_sector_count_m3[2];	// 210-211: Write-Read-Verify
						// sector count. (Mode 3 only)
	uint16_t verify_sector_count_m2[2];	// 212-213: Verify sector count.
						// (Mode 2 only)
	uint16_t nv_cache_caps;		// 214: NV cache capabilities
	uint16_t nv_cache_size[2];	// 215-216: NV cache size, in logical blocks.
	uint16_t nv_read_speed;		// 217: NV cache read speed, in MB/s.
	uint16_t nv_write_speed;	// 218: NV cache write speed, in MB/s.
	uint16_t nv_options;		// 219: NV cache options
	uint16_t wrv_current_mode;	// 220: Write-Read-Verify feature set
					//      current mode.
	uint16_t reserved_221;		// 221
	uint16_t transport_major_rev;	// 222: Transport Major revision number
	uint16_t transport_minor_rev;	// 223: Transport Minor revision number

	/** IDENTIFY DEVICE and IDENTIFY PACKET DEVICE **/
	uint16_t ce_ata_224[10];	// 224-233: Reserved for CE-ATA.

	/** IDENTIFY DEVICE only **/
	uint16_t min_blocks_ucode;	// 234: Minimum number of 512-byte units
					//      per DOWNLOAD MICROCODE command
					//      mode 3.
	uint16_t max_blocks_ucode;	// 235: Maximum number of 512-byte units
					//      per DOWNLOAD MICROCODE command
					//      mode 3.

	uint16_t reserved[19];		// 236-254
	uint16_t integrity;		// 255: Integrity word
					//      15:8 Checksum
					//       7:0 Signature (0xA5)
					// If the signature is present, the checksum
					// will be the two's complement sum of all
					// bytes in words 0-254 as well as the
					// signature byte in this word. If the
					// checksum is correct, then the sum of all
					// 512 bytes will be zero.
} ATA_RESP_IDENTIFY_DEVICE;
// FIXME: Do ASSERT_STRUCT().
//ASSERT_STRUCT(ATA_RESP_IDENTIFY_DEVICE, 512);

// IDENTIFY PACKET DEVICE (PIO Data-In)
#define ATA_CMD_IDENTIFY_PACKET_DEVICE		0xA1
#define ATA_PROTO_IDENTIFY_PACKET_DEVICE	ATA_PROTOCOL_PIO_DATA_IN

// IDENTIFY PACKET DEVICE response uses the same structure as IDENTIFY DEVICE,
// though some fields may be slightly different.
typedef struct _ATA_RESP_IDENTIFY_DEVICE ATA_RESP_IDENTIFY_PACKET_DEVICE;

#ifdef __cplusplus
}
#endif

#pragma pack()

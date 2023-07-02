/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * scsi_protocol.h: SCSI protocol definitions.                             *
 *                                                                         *
 * Copyright (c) 2013-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://en.wikipedia.org/wiki/SCSI_command
 * - http://www.t10.org/lists/2op.htm
 * - http://www.elhvb.com/mobokive/eprm/eprmx/12501.htm
 * - http://www.t10.org/lists/2sensekey.htm
 * - http://www.pioneerelectronics.com/pio/pe/images/portal/cit_3424/31636562SCSI-2RefManV31.pdf
 * - http://www.13thmonkey.org/documentation/SCSI/mmc-r10a.pdf
 * - http://www.13thmonkey.org/documentation/SCSI/mmc4r05.pdf
 * - http://www.13thmonkey.org/documentation/SCSI/x3_304_1997.pdf
 * - http://t10.org/ftp/t10/document.05/05-344r0.pdf
 *
 * Individual commands:
 * - https://en.wikipedia.org/wiki/SCSI_Read_Commands
 * - https://en.wikipedia.org/wiki/SCSI_Read_Capacity_Command
 * - https://en.wikipedia.org/wiki/SCSI_Start_Stop_Unit_Command
 * - https://en.wikipedia.org/wiki/SCSI_Inquiry_Command
 * - https://en.wikipedia.org/wiki/SCSI_Peripheral_Device_Type
 * - ftp://ftp.t10.org/t10/document.97/97-263r0.pdf
 * - https://en.wikipedia.org/wiki/SCSI_Test_Unit_Ready_Command
 * - https://en.wikipedia.org/wiki/SCSI_Status_Code
 * - https://en.wikipedia.org/wiki/SCSI_Request_Sense_Command
 * - https://en.wikipedia.org/wiki/Key_Code_Qualifier
 */

#pragma once

#include <stdint.h>

/* SCSI structs are defined on the byte-level, so we must
 * prevent the compiler from adding alignment padding. */
#if !defined(PACKED)
# if defined(__GNUC__)
#  define PACKED __attribute__((packed))
# else
#  define PACKED
# endif /* defined(__GNUC__) */
#endif /* !defined(PACKED) */

#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 *                         SCSI operation codes                         *
 ************************************************************************/

/**
 * Notes:
 * - SPC:   SCSI-3 Primary Commands
 * - SPC-2: SCSI-3 Primary Commands - 2
 * - SBC:   SCSI-3 Block Commands
 * - MMC:   SCSI-3 Multimedia Commands
 */
 
/** ALL device types. **/
#define SCSI_OP_TEST_UNIT_READY			0x00
#define SCSI_OP_REQUEST_SENSE			0x03
#define SCSI_OP_INQUIRY				0x12
#define SCSI_MODE_SELECT_6			0x15
#define SCSI_OP_COPY				0x18	/* SPC */
#define SCSI_MODE_SENSE_6			0x1A
#define SCSI_OP_RECEIVE_DIAGNOSTIC_RESULTS	0x1C
#define SCSI_OP_SEND_DIAGNOSTIC			0x1D
#define SCSI_OP_COPY_AND_VERIFY			0x3A	/* SPC */
#define SCSI_OP_WRITE_BUFFER			0x3B
#define SCSI_OP_READ_BUFFER			0x3C
#define SCSI_OP_CHANGE_DEFINITION		0x40
#define SCSI_OP_LOG_SELECT			0x4C
#define SCSI_OP_LOG_SENSE			0x4D
#define SCSI_OP_MODE_SELECT_10			0x55
#define SCSI_OP_MODE_SENSE_10			0x5A

/** Direct Access devices. */
#define SCSI_OP_REZERO_UNIT			0x01	/* SBC */
#define SCSI_OP_FORMAT_UNIT			0x04
#define SCSI_OP_REASSIGN_BLOCKS			0x07
#define SCSI_OP_READ_6				0x08
#define SCSI_OP_WRITE_6				0x0A
#define SCSI_OP_SEEK_6				0x0B	/* SBC */
#define SCSI_OP_RELEASE_6			0x17	/* SPC-2 */
#define SCSI_OP_START_STOP_UNIT			0x1B
#define SCSI_OP_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1E
#define SCSI_OP_READ_CAPACITY_10		0x25
#define SCSI_OP_READ_10				0x28
#define SCSI_OP_WRITE_10			0x2A
#define SCSI_OP_SEEK_10				0x2B	/* SBC */
#define SCSI_OP_WRITE_AND_VERIFY_10		0x2E
#define SCSI_OP_VERIFY_10			0x2F
#define SCSI_OP_SEARCH_DATA_HIGH_10		0x30	/* SBC */ /* OBSOLETE */
#define SCSI_OP_SEARCH_DATA_EQUAL_10		0x31	/* SBC */ /* OBSOLETE */
#define SCSI_OP_SEARCH_DATA_LOW_10		0x32	/* SBC */ /* OBSOLETE */
#define SCSI_OP_SET_LIMITS_10			0x33	/* SBC */ /* OBSOLETE */
#define SCSI_OP_PREFETCH_10			0x34
#define SCSI_OP_SYNCHRONIZE_CACHE_10		0x35
#define SCSI_OP_LOCK_UNLOCK_CACHE_10		0x36	/* SBC */
#define SCSI_OP_READ_DEFECT_DATA_10		0x37
#define SCSI_OP_COMPARE				0x39	/* SPC */
#define SCSI_OP_READ_LONG_10			0x3E
#define SCSI_OP_WRITE_LONG_10			0x3F
#define SCSI_OP_WRITE_SAME_10			0x41
#define SCSI_OP_RELEASE_10			0x57	/* SPC-2 */
#define SCSI_OP_ATA_PASS_THROUGH_16		0x85
#define SCSI_OP_READ_16				0x88
#define SCSI_OP_WRITE_16			0x8A
#define SCSI_OP_WRITE_AND_VERIFY_16		0x8E
#define SCSI_OP_VERIFY_16			0x8F
#define SCSI_OP_PREFETCH_16			0x90
#define SCSI_OP_SYNCHRONIZE_CACHE_16		0x91
#define SCSI_OP_LOCK_UNLOCK_CACHE_16		0x92	/* SBC */
#define SCSI_OP_WRITE_SAME_16			0x93
#define SCSI_OP_ATA_PASS_THROUGH_12		0xA1	/* clashes with MMC BLANK */
#define SCSI_OP_READ_12				0xA8
#define SCSI_OP_WRITE_12			0xAA
#define SCSI_OP_SET_LIMITS_12			0xB3	/* SBC */
#define SCSI_OP_READ_DEFECT_DATA_12		0xB7

/** Sequential Access devices. **/
#define SCSI_OP_READ_BLOCK_LIMITS		0x05
#define SCSI_OP_READ_REVERSE_6			0x0F
#define SCSI_OP_WRITE_FILEMARKS_6		0x10
#define SCSI_OP_SPACE_6				0x11
#define SCSI_OP_VERIFY_6			0x13
#define SCSI_OP_RECOVER_BUFFERED_DATA		0x14
#define SCSI_OP_RESERVE_6			0x16
#define SCSI_OP_ERASE_6				0x19
#define SCSI_OP_LOAD_UNLOAD			0x1B
#define SCSI_OP_LOCATE_10			0x2B
#define SCSI_OP_ERASE_10			0x2C
#define SCSI_OP_READ_POSITION			0x34
#define SCSI_OP_RESERVE_10			0x56
#define SCSI_OP_WRITE_FILEMARKS_16		0x10
#define SCSI_OP_READ_REVERSE_16			0x81
#define SCSI_OP_SPACE_16			0x91
#define SCSI_OP_LOCATE_16			0x92
#define SCSI_OP_ERASE_16			0x93
#define SCSI_OP_ERASE_12			0xAC

/** Printer devices. **/
#define SCSI_OP_PRINT				0x0A
#define SCSI_OP_SLEW_AND_PRINT			0x0B
#define SCSI_OP_SYNCHRONIZE_BUFFER		0x10
#define SCSI_OP_STOP_PRINT			0x1B

/** Processor devices. **/
#define SCSI_OP_RECEIVE				0x08
#define SCSI_OP_SEND				0x0A

/** Write-Once devices. **/
#define SCSI_OP_MEDIUM_SCAN			0x38
#define SCSI_OP_SEARCH_DATA_HIGH_12		0xB0	/* SBC */ /* OBSOLETE */
#define SCSI_OP_SEARCH_DATA_EQUAL_12		0xB1	/* SBC */ /* OBSOLETE */
#define SCSI_OP_SEARCH_DATA_LOW_12		0xB2	/* SBC */ /* OBSOLETE */

/** CD-ROM devices. **/
#define SCSI_OP_READ_SUBCHANNEL			0x42
#define SCSI_OP_READ_TOC			0x43
#define SCSI_OP_READ_TOC_PMA_ATIP		0x43	/* full command name */
#define SCSI_OP_READ_HEADER			0x44
#define SCSI_OP_PLAY_AUDIO_10			0x45
#define SCSI_OP_PLAY_AUDIO_MSF			0x47
#define SCSI_OP_PLAY_AUDIO_TRACK_INDEX		0x48	/* not listed on t10.org */
#define SCSI_OP_PLAY_TRACK_RELATIVE_10		0x49	/* not listed on t10.org */
#define SCSI_OP_PAUSE_RESUME			0x4B
#define SCSI_OP_STOP_PLAY_SCAN			0x4E
#define SCSI_OP_PLAY_AUDIO_12			0xA5
#define SCSI_OP_PLAY_TRACK_RELATIVE_12		0xA9	/* not listed on t10.org */
#define SCSI_OP_READ_CD_MSF			0xB9	/* MSF addressing */
#define SCSI_OP_SET_CD_SPEED			0xBB
#define SCSI_OP_MECHANISM_STATUS		0xBD
#define SCSI_OP_READ_CD				0xBE	/* LBA addressing */

/** CD-ROM devices: MultiMedia Commands. **/
#define SCSI_OP_GET_CONFIGURATION		0x46
#define SCSI_OP_GET_EVENT_STATUS_NOTIFICATION	0x4A
#define SCSI_OP_READ_DISC_INFORMATION		0x51
#define SCSI_OP_READ_TRACK_INFORMATION		0x52
#define SCSI_OP_RESERVE_TRACK			0x53
#define SCSI_OP_SEND_OPC_INFORMATION		0x54
#define SCSI_OP_REPAIR_TRACK			0x58
#define SCSI_OP_CLOSE_TRACK_SESSION		0x5B
#define SCSI_OP_READ_BUFFER_CAPACITY		0x5C
#define SCSI_OP_SEND_CUE_SHEET			0x5D
#define SCSI_OP_BLANK				0xA1	/* clashes with ATA PASS-THROUGH(12) */
#define SCSI_OP_SEND_KEY			0xA3
#define SCSI_OP_REPORT_KEY			0xA4
#define SCSI_OP_LOAD_UNLOAD_CD			0xA6	/* multi-disc changers */
#define SCSI_OP_LOAD_UNLOAD_CD_DVD		0xA6	/* multi-disc changers */
#define SCSI_OP_SET_READ_AHEAD			0xA7
#define SCSI_OP_GET_PERFORMANCE			0xAC
#define SCSI_OP_READ_DVD_STRUCTURE		0xAD
#define SCSI_OP_SET_STREAMING			0xB6
#define SCSI_OP_MMC_SCAN			0xBA	/* name collision with SCSI_OP_SCAN (0x1B) */
#define SCSI_OP_SEND_DVD_STRUCTURE		0xBF

/** Scanner devices. **/
#define SCSI_OP_SCAN				0x1B
#define SCSI_OP_SET_WINDOW			0x24
#define SCSI_OP_GET_WINDOW			0x25
#define SCSI_OP_OBJECT_POSITION			0x31
#define SCSI_OP_GET_DATA_BUFFER_STATUS		0x34

/** Optical Memory devices. **/
#define SCSI_OP_UPDATE_BLOCK			0x3D

/** Medium Changer devices. **/
#define SCSI_OP_INITIALIZE_ELEMENT_STATUS	0x07
#define SCSI_OP_POSITION_TO_ELEMENT		0x2B
#define SCSI_OP_EXCHANGE_MEDIUM			0xA6
#define SCSI_OP_REQUEST_VOLUME_ELEMENT_ADDRESS	0xB5
#define SCSI_OP_SEND_VOLUME_TAG			0xB6

/** Communication devices. **/
#define SCSI_OP_GET_MESSAGE_6			0x08
#define SCSI_OP_SEND_MESSAGE_6			0x0A
#define SCSI_OP_GET_MESSAGE_10			0x28
#define SCSI_OP_SEND_MESSAGE_10			0x2A
#define SCSI_OP_GET_MESSAGE_12			0xA8
#define SCSI_OP_SEND_MESSAGE_12			0xAA

/** SERVICE ACTION IN(16) **/
#define SCSI_OP_SERVICE_ACTION_IN_16		0x9E
/** SERVICE ACTION IN(16) opcodes **/
#define SCSI_SAIN_OP_READ_CAPACITY_16		0x10
#define SCSI_SAIN_OP_READ_LONG_16		0x11

/************************************************************************
 *                          SCSI status codes                           *
 ************************************************************************/

#define SCSI_STATUS_OK				0x00
#define SCSI_STATUS_CHECK_CONDITION		0x02
#define SCSI_STATUS_CONDITION_MET		0x04
#define SCSI_STATUS_BUSY			0x08
#define SCSI_STATUS_INTERMEDIATE		0x10
#define SCSI_STATUS_INTERMEDIATE_CONDITION_MET	0x14
#define SCSI_STATUS_RESERVATION_CONFLICT	0x18
#define SCSI_STATUS_COMMAND_TERMINATED		0x22
#define SCSI_STATUS_QUEUE_FULL			0x28
#define SCSI_STATUS_ACA_ACTIVE			0x30
#define SCSI_STATUS_TASK_ABORTED		0x40

/************************************************************************
 *                    SCSI command descriptor blocks                    *
 ************************************************************************/

/** REQUEST SENSE (0x03) **/

typedef struct PACKED _SCSI_CDB_REQUEST_SENSE {
	uint8_t OpCode;		/* REQUEST SENSE (0x03) */
	uint8_t LUN;		/* filled in by OS */
	uint8_t Reserved[2];
	uint8_t AllocLen;
	uint8_t Control;
} SCSI_CDB_REQUEST_SENSE;

typedef struct PACKED _SCSI_RESP_REQUEST_SENSE {
	uint8_t ErrorCode;		/* Error code. (0x70 or 0x71) */
	uint8_t SegmentNum;		/* Segment number. (obsolete) */
	uint8_t SenseKey;		/* Sense key, plus other bits. */
	uint8_t InfoBytes[4];		/* Information. */
	uint8_t AddSenseLen;		/* Additional sense length. (n-7) */
	uint8_t ComSpecInfo[4];		/* Command-specific information. */
	uint8_t AddSenseCode;		/* Additional sense code. */
	uint8_t AddSenseQual;		/* Additional sense code qualifier. */
	uint8_t FRUCode;		/* Field replaceable unit code. */
	uint8_t SenKeySpec[3];		/* Sense-key specific. */
	// NOTE: MSVC doesn't really like 0-byte arrays.
	//uint8_t AddSenseBytes[0];	/* Additional sense bytes. */
} SCSI_RESP_REQUEST_SENSE;

/** SCSI error code macros. (From udev) **/
#define ERRCODE(s)	((((s)[2] & 0x0FU) << 16) | ((s)[12] << 8) | ((s)[13]))
#define SK(errcode)	(((errcode) >> 16) & 0xFU)
#define ASC(errcode)	(((errcode) >> 8) & 0xFFU)
#define ASCQ(errcode)	((errcode) & 0xFFU)

/* REQUEST SENSE error codes. */
#define SCSI_ERR_REQUEST_SENSE_CURRENT	0x70	/* Current errors, fixed format. */
#define SCSI_ERR_REQUEST_SENSE_DEFERRED	0x71	/* Deferred errors, fixed format. */
#define SCSI_ERR_REQUEST_SENSE_CURRENT_DESC	0x72	/* Current errors, descriptor format. */
#define SCSI_ERR_REQUEST_SENSE_DEFERRED_DESC	0x73	/* Deferred errors, descriptor format. */

/* REQUEST SENSE bit definitions. */
#define SCSI_BIT_REQUEST_SENSE_ERRORCODE_VALID	(1U << 7)	/* If set, INFORMATION is defined. */
#define SCSI_BIT_REQUEST_SENSE_KEY_FILEMARK	(1U << 7)	/* Reached a filemark. */
#define SCSI_BIT_REQUEST_SENSE_KEY_EOM		(1U << 6)	/* End of medium. */
#define SCSI_BIT_REQUEST_SENSE_KEY_ILI		(1U << 5)	/* Incorrect length indicator. */

/* REQUEST SENSE sense key definitons. */
#define SCSI_SENSE_KEY_NO_SENSE			0x0
#define SCSI_SENSE_KEY_RECOVERED_ERROR		0x1
#define SCSI_SENSE_KEY_NOT_READY		0x2
#define SCSI_SENSE_KEY_MEDIUM_ERROR		0x3
#define SCSI_SENSE_KEY_HARDWARE_ERROR		0x4
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST		0x5
#define SCSI_SENSE_KEY_UNIT_ATTENTION		0x6
#define SCSI_SENSE_KEY_DATA_PROTECT		0x7
#define SCSI_SENSE_KEY_BLANK_CHECK		0x8
#define SCSI_SENSE_KEY_VENDOR_SPECIFIC		0x9 /* Sometimes indicates Firmware Error */
#define SCSI_SENSE_KEY_COPY_ABORTED		0xA
#define SCSI_SENSE_KEY_ABORTED_COMMAND		0xB
#define SCSI_SENSE_KEY_EQUAL			0xC /* Search. (not listed on t10.org) */
#define SCSI_SENSE_KEY_VOLUME_OVERFLOW		0xD
#define SCSI_SENSE_KEY_MISCOMPARE		0xE /* Search. */
#define SCSI_SENSE_KEY_COMPLETED		0xF

/** TEST UNIT READY (0x00) **/

typedef struct PACKED _SCSI_CDB_TEST_UNIT_READY {
	uint8_t OpCode;		/* TEST UNIT READY (0x00) */
	uint8_t LUN;		/* filled in by OS */
	uint8_t Reserved[3];
	uint8_t Control;
} SCSI_CDB_TEST_UNIT_READY;

/** INQUIRY (0x12) **/

typedef struct PACKED _SCSI_CDB_INQUIRY {
	uint8_t OpCode;		/* INQUIRY (0x12) */
	uint8_t EVPD;
	uint8_t PageCode;
	uint16_t AllocLen;	/* (BE16) */
	uint8_t Control;
} SCSI_CDB_INQUIRY;

/* INQUIRY response for Standard Inquiry Data. (EVPD == 0, PageCode == 0) */
typedef struct PACKED _SCSI_RESP_INQUIRY_STD {
	uint8_t PeripheralDeviceType;	/* High 3 bits == qualifier; low 5 bits == type */
	uint8_t RMB_DeviceTypeModifier;
	uint8_t Version;
	uint8_t ResponseDataFormat;
	uint8_t AdditionalLength;
	uint8_t Reserved1[2];
	uint8_t Flags;
	char vendor_id[8];
	char product_id[16];
	char product_revision_level[4];
	char VendorSpecific[20];
	uint8_t Reserved2[40];
} SCSI_RESP_INQUIRY_STD;

/* Peripheral device types. (PeripheralDeviceType & 0x1F) */
#define SCSI_DEVICE_TYPE_DASD		0x00	/* Direct-access block device, e.g. HDD. */
#define SCSI_DEVICE_TYPE_SEQD		0x01	/* Sequential-access device, e.g. tape. */
#define SCSI_DEVICE_TYPE_PRINTER	0x02	/* Printer. */
#define SCSI_DEVICE_TYPE_PROCESSOR	0x03	/* Processor device. */
#define SCSI_DEVICE_TYPE_WORM		0x04	/* Write-once media. (NOT USED FOR CD-R!) */
#define SCSI_DEVICE_TYPE_CDROM		0x05	/* CD-ROM device. Also includes DVD and BD. */
#define SCSI_DEVICE_TYPE_SCANNER	0x06	/* Scanner. */
#define SCSI_DEVICE_TYPE_OPTICAL	0x07	/* Optical memory device. (NOT USED FOR CD-R!) */
#define SCSI_DEVICE_TYPE_JUKEBOX	0x08	/* Medium changer, e.g. CD jukebox. */
#define SCSI_DEVICE_TYPE_COMM		0x09	/* Communications device. */
#define SCSI_DEVICE_TYPE_STORAGE	0x0C	/* Storage array controller device, e.g. RAID. */
#define SCSI_DEVICE_TYPE_ENCLOSURE	0x0D	/* Enclosure services device. */
#define SCSI_DEVICE_TYPE_SDASD		0x0E	/* Simplied direct-access device. */
#define SCSI_DEVICE_TYPE_OPTICAL_CARD	0x0F	/* Optical card reader/writer device. */
#define SCSI_DEVICE_TYPE_EXPANDER	0x10	/* Bridging expander. */
#define SCSI_DEVICE_TYPE_OBJECT		0x11	/* Object-based storage device. */
#define SCSI_DEVICE_TYPE_AUTOMATION	0x12	/* Automation/Drive interface. */
#define SCSI_DEVICE_TYPE_SECURITY	0x13	/* Security manager device. */
#define SCSI_DEVICE_TYPE_SIMPLE_MMC	0x14	/* Simplified MMC device. */
#define SCSI_DEVICE_TYPE_KNOWN_LUN	0x1E	/* Well-known logical unit. */
#define SCSI_DEVICE_TYPE_UNKNOWN	0x1F	/* Unknown or no device type. */

/* ANSI version numbers. */
#define SCSI_VERSION_ANSI_MAYBE		0x0	/* Unknown if device is using an ANSI-approved standard. */
#define SCSI_VERSION_ANSI_SCSI1		0x1	/* SCSI-1 (ANSI X3.131-1986) */
#define SCSI_VERSION_ANSI_SCSI2		0x2	/* SCSI-2 */

/* Flags. */
#define SCSI_BIT_INQUIRY_FLAGS_RELADR		(1U << 7)	/* 1 == device supports RelAdr mode. */
#define SCSI_BIT_INQUIRY_FLAGS_WBUS32		(1U << 6)	/* 1 == device supports 32-bit mode. */
#define SCSI_BIT_INQUIRY_FLAGS_WBUS16		(1U << 5)	/* 1 == device supports 16-bit mode. */
#define SCSI_BIT_INQUIRY_FLAGS_SYNC		(1U << 4)	/* 1 == supports synchronous transfers. */
#define SCSI_BIT_INQUIRY_FLAGS_LINKED		(1U << 3)	/* 1 == supports linked commands. */
#define SCSI_BIT_INQUIRY_FLAGS_CMDQUE		(1U << 1)	/* 1 == supports Tagged Command Queueing. */
#define SCSI_BIT_INQUIRY_FLAGS_SFTRE		(1U << 0)	/* 1 == supports Soft Reset. */

/** READ CAPACITY(10) (0x25) **/

typedef struct PACKED _SCSI_CDB_READ_CAPACITY_10 {
	uint8_t OpCode;		/* READ CAPACITY(10) (0x25) */
	uint8_t RelAdr;		/* 1 == relative to LBA */
	uint32_t LBA;		/* (BE32) */
	uint8_t Reserved[2];
	uint8_t PMI;		/* 1 == get last LBA that doesn't incur substantial delay */
	uint8_t Control;
} SCSI_CDB_READ_CAPACITY_10;

/* READ CAPACITY(10) response. */
typedef struct PACKED _SCSI_RESP_READ_CAPACITY_10 {
	uint32_t LBA;		/* (BE32) Highest LBA number. */
	uint32_t BlockLen;	/* (BE32) Block length, in bytes. */
} SCSI_RESP_READ_CAPACITY_10;

/** READ CAPACITY(16) [SERVICE ACTION IN(16)] (0x9E) **/

typedef struct PACKED _SCSI_CDB_READ_CAPACITY_16 {
	uint8_t OpCode;		/* SERVICE ACTION IN(16) (0x9E) */
	uint8_t SAIn_OpCode;	/* READ CAPACITY(16) SAIn OpCode (0x10) */
	uint64_t LBA;		/* (BE64) */
	uint32_t AllocLen;	/* (BE32) */
	uint8_t Reserved;
	uint8_t Control;
} SCSI_CDB_READ_CAPACITY_16;

/* READ CAPACITY(16) response. */
typedef struct PACKED _SCSI_RESP_READ_CAPACITY_16 {
	uint64_t LBA;		/* (BE64) Highest LBA number. */
	uint32_t BlockLen;	/* (BE32) Block length, in bytes. */
	uint8_t Flags;		/* Bit 1 == RTO_EN; Bit 0 == PROT_EN */
	uint8_t Reserved[19];
} SCSI_RESP_READ_CAPACITY_16;

/** READ(10) (0x28) **/

typedef struct PACKED _SCSI_CDB_READ_10
{
	uint8_t OpCode;		/* READ(10) (0x28) */
	uint8_t Flags;
	uint32_t LBA;		/* (BE32) Starting LBA. */
	uint8_t Reserved;
	uint16_t TransferLen;	/* (BE16) Transfer length, in blocks. */
	uint8_t Control;
} SCSI_CDB_READ_10;

/* Flags. */
#define SCSI_BIT_READ_10_FUA		(1U << 3)	/* Force Unit Access */
#define SCSI_BIT_READ_10_DPO		(1U << 4)	/* Disable Page Out */

/** READ TOC (0x43) [CD-ROM] **/

typedef struct PACKED _SCSI_CDB_READ_TOC
{
	uint8_t OpCode;		/* READ TOC (0x43) */
	uint8_t MSF;		/* 0 == LBA; 2 == MSF */
	uint8_t Format;		/* TODO: Check MMC docs for this again... */
	uint8_t Reserved[3];
	uint8_t TrackSessionNumber;
	uint16_t AllocLen;	/* (BE16) */
	uint8_t Control;	/* High 2 bits indicate "Format"? (Pioneer vendor-specific op...) */
} SCSI_CDB_READ_TOC;

/* MSF flags. */
#define SCSI_BIT_READ_TOC_MSF_LBA	(0U << 2)	/* 0 == reads TOC in LBA format. */
#define SCSI_BIT_READ_TOC_MSF_MSF	(1U << 2)	/* 1 == reads TOC in MSF format. */

/* TOC: Track entry. */
typedef struct PACKED _SCSI_CDROM_TOC_TRACK
{
	uint8_t rsvd1;
	uint8_t ControlADR;	/* Track type. */
	uint8_t TrackNumber;
	uint8_t rsvd2;
	uint32_t StartAddress;	/* (BE32) */
} SCSI_CDROM_TOC_TRACK;

/* Table of Contents. */
typedef struct PACKED _SCSI_CDROM_TOC
{
	uint8_t FirstTrackNumber;
	uint8_t LastTrackNumber;
	SCSI_CDROM_TOC_TRACK Tracks[100];
} SCSI_CDROM_TOC;

/* READ TOC response. */
typedef struct PACKED _SCSI_RESP_READ_TOC
{
	uint16_t DataLen;	/* (BE16) */
	SCSI_CDROM_TOC toc;
} SCSI_RESP_READ_TOC;

/** GET CONFIGURATION (0x46) [MMC] **/

typedef struct PACKED _SCSI_CDB_GET_CONFIGURATION {
	uint8_t OpCode;			/* GET CONFIGURATION (0x46) */
	uint8_t RT;			/* Type of Feature Descriptors to receive. */
	uint16_t StartingFeatureNumber;	/* (BE16) Index of first feature to receive. */
	uint8_t Reserved[3];
	uint16_t AllocLen;		/* (BE16) */
	uint8_t Control;
} SCSI_CDB_GET_CONFIGURATION;

/* Requested Type of feature descriptors. */
#define SCSI_GET_CONFIGURATION_RT_ALL		0x0
#define SCSI_GET_CONFIGURATION_RT_HEADER_ONLY	0x1
#define SCSI_GET_CONFIGURATION_RT_ONE		0x2

/* GET CONFIGURATION response header. */
typedef struct PACKED _SCSI_RESP_GET_CONFIGURATION_HEADER {
	uint32_t DataLength;		/* (BE32) Total length of data returned. */
	uint8_t Reserved[2];
	uint16_t CurrentProfile;	/* (BE16) Current Feature Profile. */
} SCSI_RESP_GET_CONFIGURATION_HEADER;

/* Feature codes. */
#define SCSI_MMC_FEATURE_PROFILE_LIST		0x0000
#define SCSI_MMC_FEATURE_CORE			0x0001
#define SCSI_MMC_FEATURE_MORPHING		0x0002
#define SCSI_MMC_FEATURE_REMOVABLE_MEDIUM	0x0003
#define SCSI_MMC_FEATURE_RANDOM_READABLE	0x0010
#define SCSI_MMC_FEATURE_MULTIREAD		0x001D
#define SCSI_MMC_FEATURE_CD_READ		0x001E
#define SCSI_MMC_FEATURE_DVD_READ		0x001F
#define SCSI_MMC_FEATURE_RANDOM_WRITABLE	0x0020
#define SCSI_MMC_FEATURE_INC_STRM_WRITABLE	0x0021
#define SCSI_MMC_FEATURE_SECTOR_ERASABLE	0x0022
#define SCSI_MMC_FEATURE_FORMATTABLE		0x0023
#define SCSI_MMC_FEATURE_DEFECT_MANAGEMENT	0x0024
#define SCSI_MMC_FEATURE_WRITE_ONCE		0x0025
#define SCSI_MMC_FEATURE_RESTRICTED_OVERWRITE	0x0026
#define SCSI_MMC_FEATURE_CD_TRACK_AT_ONCE	0x002D
#define SCSI_MMC_FEATURE_CD_MASTERING		0x002E
#define SCSI_MMC_FEATURE_DVD_R_WRITE		0x002F
#define SCSI_MMC_FEATURE_POWER_MANAGEMENT	0x0100
#define SCSI_MMC_FEATURE_SMART			0x0101
#define SCSI_MMC_FEATURE_EMBEDDED_CHANGER	0x0102
#define SCSI_MMC_FEATURE_CD_AUDIO_ANALOG_PLAY	0x0103
#define SCSI_MMC_FEATURE_MICROCODE_UPGRADE	0x0104
#define SCSI_MMC_FEATURE_TIME_OUT		0x0105
#define SCSI_MMC_FEATURE_DVD_CSS		0x0106
#define SCSI_MMC_FEATURE_REAL_TIME_STREAMING	0x0107
#define SCSI_MMC_FEATURE_LUN_SERIAL_NUMBER	0x0108

/* Feature profiles. (Feature 0x0000) */
#define SCSI_MMC_PROFILE_NONREMOVABLE_DISK	0x0001
#define SCSI_MMC_PROFILE_REMOVABLE_DISK		0x0002
#define SCSI_MMC_PROFILE_MO_ERASABLE		0x0003
#define SCSI_MMC_PROFILE_MO_WRITE_ONCE		0x0004
#define SCSI_MMC_PROFILE_AS_MO			0x0005
#define SCSI_MMC_PROFILE_CDROM			0x0008
#define SCSI_MMC_PROFILE_CD_R			0x0009
#define SCSI_MMC_PROFILE_CD_RW			0x000A
#define SCSI_MMC_PROFILE_DVDROM			0x0010
#define SCSI_MMC_PROFILE_DVD_R			0x0011
#define SCSI_MMC_PROFILE_DVD_RAM		0x0012
#define SCSI_MMC_PROFILE_DVD_RW_RO		0x0013	/* restricted overwrite */
#define SCSI_MMC_PROFILE_DVD_RW_SEQ		0x0014	/* sequential overwrite */
#define SCSI_MMC_PROFILE_DVD_R_DL_SEQ		0x0015	/* sequential recording */
#define SCSI_MMC_PROFILE_DVD_R_DL_JUMP		0x0016	/* layer-jump recording */
#define SCSI_MMC_PROFILE_DVD_RW_DL		0x0017
#define SCSI_MMC_PROFILE_DVD_PLUS_RW		0x001A
#define SCSI_MMC_PROFILE_DVD_PLUS_R		0x001B
#define SCSI_MMC_PROFILE_DDCDROM		0x0020
#define SCSI_MMC_PROFILE_DDCD_R			0x0021
#define SCSI_MMC_PROFILE_DDCD_RW		0x0022
#define SCSI_MMC_PROFILE_DVD_PLUS_RW_DL		0x002A
#define SCSI_MMC_PROFILE_DVD_PLUS_R_DL		0x002B
#define SCSI_MMC_PROFILE_BDROM			0x0040
#define SCSI_MMC_PROFILE_BD_R_SEQ		0x0041	/* sequential recording */
#define SCSI_MMC_PROFILE_BD_R_RND		0x0042	/* random recording */
#define SCSI_MMC_PROFILE_BD_RE			0x0043
#define SCSI_MMC_PROFILE_HDDVD			0x0050
#define SCSI_MMC_PROFILE_HDDVD_R		0x0051
#define SCSI_MMC_PROFILE_HDDVD_RAM		0x0052
#define SCSI_MMC_PROFILE_HDDVD_RW		0x0053
#define SCSI_MMC_PROFILE_HDDVD_R_DL		0x0058
#define SCSI_MMC_PROFILE_HDDVD_RW_DL		0x005A

/** READ DISC INFORMATION (0x51) [MMC] **/

typedef struct PACKED _SCSI_CDB_READ_DISC_INFORMATION {
	uint8_t OpCode;			/* READ DISC INFORMATION (0x51) */
	uint8_t DataType;		/* Data type to read. */
	uint8_t Reserved[5];
	uint16_t AllocLen;		/* (BE16) */
	uint8_t Control;
} SCSI_CDB_READ_DISC_INFORMATION;

/* Data Types for READ DISC INFORMATION. */
#define SCSI_READ_DISC_INFORMATION_DATATYPE_STANDARD	0x00
#define SCSI_READ_DISC_INFORMATION_DATATYPE_TRACK	0x01
#define SCSI_READ_DISC_INFORMATION_DATATYPE_POW		0x02

/* READ DISC INFORMATION response: Standard disc information. */
typedef struct PACKED _SCSI_RESP_READ_DISC_INFORMATION_STANDARD {
	uint16_t DiscInfoLength;		/* (BE16) */
	uint8_t DiscStatusFlags;
	uint8_t FirstTrackNumber;
	uint8_t NumSessionsLSB;
	uint8_t FirstTrackNumberInLastSessionLSB;
	uint8_t LastTrackNumberInLastSessionLSB;
	uint8_t ValidFlags;
	uint8_t DiscType;
	uint8_t NumSessionsMSB;
	uint8_t FirstTrackNumberInLastSessionMSB;
	uint8_t LastTrackNumberInLastSessionMSB;
	uint32_t DiscIdentification;		/* (BE32) */
	uint32_t LastSessionLeadInStartLBA;	/* (BE32) */
	uint32_t DiscBarCode;			/* (BE32) */
	uint8_t DiscApplicationCode;

	/* OPC tables. */
	uint8_t NumOPCTables;
	// NOTE: MSVC doesn't really like 0-byte arrays.
	//uint8_t OPCTableEntries[0];
} SCSI_RESP_READ_DISC_INFORMATION_STANDARD;

/* READ DISC INFORMATION response: Track resources information. */
typedef struct PACKED _SCSI_RESP_READ_DISC_INFORMATION_TRACK {
	uint16_t DiscInfoLength;			/* (BE16) == 10 */
	uint8_t DiscInfoDataType;			/* == (0x1U << 5) */
	uint8_t Reserved;
	uint16_t MaxPossibleNumberOfTracks;		/* (BE16) */
	uint16_t NumberOfAssignedTracks;		/* (BE16) */
	uint16_t MaxPossibleNumberOfAppendableTracks;	/* (BE16) */
	uint16_t CurrentNumberOfAppendableTracks;	/* (BE16) */
} SCSI_RESP_READ_DISC_INFORMATION_TRACK;

/* READ DISC INFORMATION response: POW Resources information. */
typedef struct PACKED _SCSI_RESP_READ_DISC_INFORMATION_POW {
	uint16_t DiscInfoLength;			/* (BE16) == 14 */
	uint8_t DiscInfoDataType;			/* == (0x2U << 5) */
	uint8_t Reserved;
	uint32_t RemainingPOWReplacements;		/* (BE32) */
	uint32_t RemainingPOWReallocationMapEntries;	/* (BE32) */
	uint32_t NumberOfRemainingPOWUpdates;		/* (BE32) */
} SCSI_RESP_READ_DISC_INFORMATION_POW;

/** READ CD (0xBE) **/

typedef struct PACKED _SCSI_CDB_READ_CD {
	uint8_t OpCode;			/* READ CD (0xBE) */
	uint8_t SectorType;		/* Expected sector type. */
	uint32_t StartingLBA;		/* (BE32) Starting LBA. */
	uint8_t TransferLen[3];		/* (BE24) Transfer length, in blocks. */
	uint8_t Flags;
	uint8_t Subchannels;		/* Subchannel selection bits. */
	uint8_t Control;
} SCSI_CDB_READ_CD;

/* Expected sector types. */
#define SCSI_READ_CD_SECTORTYPE_RELADR	(1U << 0)	/* 1 == relative addressing */
#define SCSI_READ_CD_SECTORTYPE_DAP	(1U << 1)	/* 1 == enable error correction for audio */
#define SCSI_READ_CD_SECTORTYPE_ANY	(0x0U << 2)	/* Any type allowed. */
#define SCSI_READ_CD_SECTORTYPE_CDDA	(0x1U << 2)	/* CD audio. */
#define SCSI_READ_CD_SECTORTYPE_M1F1	(0x2U << 2)	/* Yellow Book: 2048-byte user data */
#define SCSI_READ_CD_SECTORTYPE_M1F2	(0x3U << 2)	/* Yellow Book: 2336-byte user data */
#define SCSI_READ_CD_SECTORTYPE_M2F1	(0x4U << 2)	/* Green Book: 2048-byte user data */
#define SCSI_READ_CD_SECTORTYPE_M2F2	(0x5U << 2)	/* Green Book: 2324-byte user data */

/* Flags. */
#define SCSI_BIT_READ_CD_FLAGS_C2_ERROR		(1U << 1)	/* Read C2 error flags. */
#define SCSI_BIT_READ_CD_FLAGS_C2_BLOCK_ERROR	(1U << 2)	/* Read C2 and block error flags. */
#define SCSI_BIT_READ_CD_FLAGS_EDC_ECC		(1U << 3)	/* Read EDC & ECC data. */
#define SCSI_BIT_READ_CD_FLAGS_USER_DATA	(1U << 4)	/* Read user data. */
#define SCSI_BIT_READ_CD_FLAGS_HEADER		(1U << 5)	/* M1/F1 4-byte header. */
#define SCSI_BIT_READ_CD_FLAGS_SUBHEADER	(1U << 6)	/* M2 subheader. */
#define SCSI_BIT_READ_CD_FLAGS_SYNCH_FIELD	(1U << 7)	/* Synchronization field. */

/* Subchannel selection. */
#define SCSI_READ_CD_SUBCHANNELS_NONE	0x0	/* No subchannel data. */
#define SCSI_READ_CD_SUBCHANNELS_RAW	0x1	/* RAW subchannel data. (OBSOLETE) */
#define SCSI_READ_CD_SUBCHANNELS_Q	0x2	/* Q subchannel only. */
#define SCSI_READ_CD_SUBCHANNELS_R_W	0x4	/* R-W subchannels. */

/** READ CD MSF (0xB9) **/

typedef struct PACKED _SCSI_CDB_READ_CD_MSF {
	uint8_t OpCode;			/* READ CD (0xBE) */
	uint8_t SectorType;		/* Expected sector type. */
	uint8_t Reserved;
	struct {
		uint8_t M;
		uint8_t S;
		uint8_t F;
	} Start;			/* Starting MSF. */
	struct {
		uint8_t M;
		uint8_t S;
		uint8_t F;
	} End;				/* Ending MSF, inclusive. */
	uint8_t Flags;
	uint8_t Subchannels;		/* Subchannel selection bits. */
	uint8_t Control;
} SCSI_CDB_READ_CD_MSF;

#ifdef __cplusplus
}
#endif

#pragma pack()

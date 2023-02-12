/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * device.cpp: Extra functions for devices.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "device.hpp"

// NOTE: We can't check in CMake beacuse RP_OS_SCSI_SUPPORTED
// is checked in librpbase, not rpcli.
#ifdef RP_OS_SCSI_SUPPORTED

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpfile/RpFile.hpp"
#include "librptext/conversion.hpp"
#include "librptext/printf.hpp"
using namespace LibRpText;
using LibRpFile::RpFile;

// SCSI and ATA protocols.
#include "librpfile/scsi/scsi_protocol.h"
#include "librpfile/scsi/ata_protocol.h"

// C++ includes.
#include <iomanip>
#include <iostream>

class StreamStateSaver {
	std::ios &stream;	// Stream being adjusted.
	std::ios state;		// Copy of original flags.
public:
	explicit StreamStateSaver(std::ios &stream)
		: stream(stream)
		, state(nullptr)
	{
		// Save the stream's state.
		state.copyfmt(stream);
	}

	~StreamStateSaver()
	{
		// Restore the stream's state.
		stream.copyfmt(state);
	}

	inline void restore(void)
	{
		// Restore the stream's state.
		stream.copyfmt(state);
	}
};

/** ScsiInquiry **/

ScsiInquiry::ScsiInquiry(RpFile *file)
	: file(file)
{ }

std::ostream& operator<<(std::ostream& os, const ScsiInquiry& si)
{
	SCSI_RESP_INQUIRY_STD resp;
	int ret = si.file->scsi_inquiry(&resp);
	if (ret != 0) {
		// TODO: Decode the error.
		os << "-- " << rp_sprintf(C_("rpcli", "SCSI INQUIRY failed: %08X"),
			static_cast<unsigned int>(ret)) << '\n';
		return os;
	}

	// SCSI device information.
	// TODO: Trim spaces?
	// TODO: i18n?
	StreamStateSaver state(os);
	os << "-- SCSI INQUIRY data for: " << si.file->filename() << '\n';

	static const char *const pdt_tbl[0x20] = {
		"Direct-access block device",		// 0x00
		"Sequential-access device",		// 0x01
		"Printer",				// 0x02
		"Processor",				// 0x03
		"Write-once media",			// 0x04
		"CD/DVD/BD-ROM",			// 0x05
		"Scanner",				// 0x06
		"Optical memory device",		// 0x07
		"Medium changer",			// 0x08
		"Communications device",		// 0x09
		nullptr,				// 0x0A
		nullptr,				// 0x0B
		"Storage array controller device",	// 0x0C
		"Enclosure services device",		// 0x0D
		"Simplified direct-access device",	// 0x0E
		"Optical card reader/writer",		// 0x0F
		"Bridge controller",			// 0x10
		"Object-based storage device",		// 0x11
		"Automation/Drive interface",		// 0x12
		"Security manager device",		// 0x13
		"Simplified MMC device",		// 0x14
		nullptr, nullptr, nullptr,		// 0x15-0x17
		nullptr, nullptr, nullptr,		// 0x18-0x1A
		nullptr, nullptr, nullptr,		// 0x1B-0x1D
		"Well-known logical unit",		// 0x1E
		"Unknown or no device type",		// 0x1F
	};
	os << "Peripheral device type: ";
	const char *const pdt = pdt_tbl[resp.PeripheralDeviceType & 0x1F];
	os << (pdt ? pdt : rp_sprintf("0x%02X",
		static_cast<unsigned int>(resp.PeripheralDeviceType) & 0x1F)) << '\n';

	os << "Peripheral qualifier:   ";
	static const char *const pq_tbl[8] = {
		"Connected",		// 000b
		"Not connected",	// 001b
		"010b",			// 010b
		"Not supported",	// 011b
		"100b", "101b",		// 100b,101b
		"110b", "111b",		// 110b,111b
	};
	os << pq_tbl[resp.PeripheralDeviceType >> 5] << '\n';

	os << "Removable media:        " << (resp.RMB_DeviceTypeModifier & 0x80 ? "Yes" : "No") << '\n';

	os << "SCSI version:           ";
	if (resp.Version <= 0x07) {
		static const char ver_tbl[8][8] = {
			"Any",		// 0x00
			"SCSI-1",	// 0x01
			"SCSI-2",	// 0x02
			"SPC",		// 0x03
			"SPC-2",	// 0x04
			"SPC-3",	// 0x05
			"SPC-4",	// 0x06
			"SPC-5",	// 0x07
		};
		os << ver_tbl[resp.Version];
	} else {
		os << rp_sprintf("0x%02X", resp.Version);
	}
	os << '\n';

	// TODO: ResponseDataFormat and high bits?
	// TODO: Verify AdditionalLength field?
	// TODO: Flags.
	// TODO: Trim spaces.
	os << "Vendor ID:              " << latin1_to_utf8(resp.vendor_id, sizeof(resp.vendor_id)) << '\n';
	os << "Product ID:             " << latin1_to_utf8(resp.product_id, sizeof(resp.product_id)) << '\n';
	os << "Firmware version:       " << latin1_to_utf8(resp.product_revision_level, sizeof(resp.product_revision_level)) << '\n';
	os << "Vendor notes:           " << latin1_to_utf8(resp.VendorSpecific, sizeof(resp.VendorSpecific)) << '\n';

	// TODO: Check supported media types for CD/DVD/BD-ROM drives?
	// That's a bit more than an INQUIRY command...
	return os;
}

/** AtaIdentifyDevice **/

AtaIdentifyDevice::AtaIdentifyDevice(RpFile *file, bool packet)
	: file(file)
	, packet(packet)
{ }

std::ostream& operator<<(std::ostream& os, const AtaIdentifyDevice& si)
{
	ATA_RESP_IDENTIFY_DEVICE resp;
	int ret;
	if (si.packet) {
		ret = si.file->ata_identify_packet_device(&resp);
	} else {
		ret = si.file->ata_identify_device(&resp);
	}

	if (ret != 0) {
		// TODO: Decode the error.
		os << "-- " << rp_sprintf(C_("rpcli", "ATA %s failed: %08X"),
			(si.packet ? "IDENTIFY PACKET DEVICE" : "IDENTIFY DEVICE"),
			static_cast<unsigned int>(ret)) << '\n';
		return os;
	}

	// ATA device information.
	// TODO: Decode numeric values.
	// TODO: Trim spaces?
	// TODO: i18n?
	StreamStateSaver state(os);
	os << "-- ATA IDENTIFY " << (si.packet ? "PACKET " : "") << "DEVICE data for: " << si.file->filename() << '\n';
	os << "Model number:          " << latin1_to_utf8(resp.model_number, sizeof(resp.model_number)) << '\n';
	os << "Firmware version:      " << latin1_to_utf8(resp.firmware_revision, sizeof(resp.firmware_revision)) << '\n';
	os << "Serial number:         " << latin1_to_utf8(resp.serial_number, sizeof(resp.serial_number)) << '\n';
	os << "Media serial number:   " << latin1_to_utf8(resp.media_serial_number, sizeof(resp.media_serial_number)) << '\n';
	// TODO: Byte count.
	os << "Sector count (28-bit): " << resp.total_sectors << '\n';
	os << "Sector count (48-bit): " << resp.total_sectors_48 << '\n';
	os << "Integrity word:        " << rp_sprintf("%04X", resp.integrity) << '\n';
	return os;
}

#endif /* RP_OS_SCSI_SUPPORTED */

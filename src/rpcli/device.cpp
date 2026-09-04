/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * device.cpp: Extra functions for devices.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "device.hpp"

// NOTE: We can't check in CMake beacuse RP_OS_SCSI_SUPPORTED
// is checked in librpbase, not rpcli.
#ifdef RP_OS_SCSI_SUPPORTED

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpfile/RpFile.hpp"
#include "librptext/conversion.hpp"
using namespace LibRpText;
using LibRpFile::RpFile;

// SCSI and ATA protocols.
#include "librpfile/scsi/scsi_protocol.h"
#include "librpfile/scsi/ata_protocol.h"

// C++ STL classes
#include <array>
using std::array;
using std::ios;
using std::ostream;
using std::string;

// libfmt
#include "rp-libfmt.h"

// rapidjson
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

class StreamStateSaver {
	ios &stream;	// Stream being adjusted.
	ios state;		// Copy of original flags.
public:
	explicit StreamStateSaver(ios &stream)
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

namespace Private {

// SCSI: Peripheral Device Type table
static const array<const char*, 0x20> scsi_pdt_tbl = {{
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
}};

// SCSI: Peripheral Qualifier table
static const array<const char*, 8> scsi_pq_tbl = {{
	"Connected",		// 000b
	"Not connected",	// 001b
	"010b",			// 010b
	"Not supported",	// 011b
	"100b", "101b",		// 100b,101b
	"110b", "111b",		// 110b,111b
}};

// SCSI: Version table
static constexpr char scsi_version_tbl[8][8] = {
	"Any",		// 0x00
	"SCSI-1",	// 0x01
	"SCSI-2",	// 0x02
	"SPC",		// 0x03
	"SPC-2",	// 0x04
	"SPC-3",	// 0x05
	"SPC-4",	// 0x06
	"SPC-5",	// 0x07
};

}

/** ScsiInquiry **/

ScsiInquiry::ScsiInquiry(RpFile *file)
	: file(file)
{}

ostream &operator<<(ostream &os, const ScsiInquiry& si)
{
	SCSI_RESP_INQUIRY_STD resp;
	int ret = si.file->scsi_inquiry(&resp);
	if (ret != 0) {
		// TODO: Decode the error.
		os << "-- "
		   << fmt::format(FRUN(C_("rpcli", "SCSI INQUIRY failed: {:0>8X}")), static_cast<unsigned int>(ret))
		   << '\n';
		return os;
	}

	// SCSI device information.
	// TODO: Trim spaces?
	// TODO: i18n?
	StreamStateSaver state(os);
	os << "-- SCSI INQUIRY data for: " << si.file->filename() << '\n';

	os << "Peripheral device type: ";
	const char *const pdt = Private::scsi_pdt_tbl[resp.PeripheralDeviceType & 0x1F];
	os << (pdt ? pdt : fmt::format(FSTR("0x{:0>2X}"), static_cast<unsigned int>(resp.PeripheralDeviceType) & 0x1F))
	   << '\n';

	os << "Peripheral qualifier:   ";
	os << Private::scsi_pq_tbl[resp.PeripheralDeviceType >> 5] << '\n';

	os << "Removable media:        " << (resp.RMB_DeviceTypeModifier & 0x80 ? "Yes" : "No") << '\n';

	os << "SCSI version:           ";
	if (resp.Version <= 0x07) {
		os << Private::scsi_version_tbl[resp.Version];
	} else {
		os << fmt::format(FSTR("0x{:0>2X}"), resp.Version);
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

/** JSONScsiInquiry **/

JSONScsiInquiry::JSONScsiInquiry(RpFile *file)
	: file(file)
{}

ostream &operator<<(ostream &os, const JSONScsiInquiry& si)
{
	Document document;
	document.SetObject();	// document should be an object, not an array
	Document::AllocatorType& allocator = document.GetAllocator();

	// NOTE: Unlike JSONRomOutput:
	// - Always using pretty-printing. (TODO: override if using '-J'?)
	// - Not setting CRLF mode; this should be printed to
	//   a terminal or redirected to a file in text mode.

	SCSI_RESP_INQUIRY_STD resp;
	int ret = si.file->scsi_inquiry(&resp);
	if (ret != 0) {
		// TODO: Decode the error.
		Value error_val;
		error_val.SetString(
			fmt::format(FRUN(C_("rpcli", "SCSI INQUIRY failed: {:0>8X}")), static_cast<unsigned int>(ret)),
				allocator);
		document.AddMember("error", error_val, allocator);

		// Use pretty-printing.
		OStreamWrapper oswr(os);
		PrettyWriter<OStreamWrapper> writer(oswr);
		document.Accept(writer);
		return os;
	}

	Value tmpval;
	tmpval.SetString(si.file->filename(), allocator);
	document.AddMember("deviceFilename", tmpval, allocator);

	const char *const pdt = Private::scsi_pdt_tbl[resp.PeripheralDeviceType & 0x1F];
	tmpval.SetString((pdt ? pdt : fmt::format(FSTR("0x{:0>2X}"), static_cast<unsigned int>(resp.PeripheralDeviceType) & 0x1F)), allocator);
	document.AddMember("peripheralDeviceType", tmpval, allocator);

	tmpval.SetString(Private::scsi_pq_tbl[resp.PeripheralDeviceType >> 5], allocator);
	document.AddMember("peripheralQualifier", tmpval, allocator);

	tmpval.SetBool(!!(resp.RMB_DeviceTypeModifier & 0x80));
	document.AddMember("removableMedia", tmpval, allocator);

	if (resp.Version <= 0x07) {
		tmpval.SetString(Private::scsi_version_tbl[resp.Version], allocator);
	} else {
		tmpval.SetString(fmt::format(FSTR("0x{:0>2X}"), resp.Version), allocator);
	}
	document.AddMember("scsiVersion", tmpval, allocator);

	// TODO: ResponseDataFormat and high bits?
	// TODO: Verify AdditionalLength field?
	// TODO: Flags.
	// TODO: Trim spaces.
	tmpval.SetString(latin1_to_utf8(resp.vendor_id, sizeof(resp.vendor_id)), allocator);
	document.AddMember("vendorID", tmpval, allocator);
	tmpval.SetString(latin1_to_utf8(resp.product_id, sizeof(resp.product_id)), allocator);
	document.AddMember("productID", tmpval, allocator);
	tmpval.SetString(latin1_to_utf8(resp.product_revision_level, sizeof(resp.product_revision_level)), allocator);
	document.AddMember("firmwareVersion", tmpval, allocator);
	tmpval.SetString(latin1_to_utf8(resp.VendorSpecific, sizeof(resp.VendorSpecific)), allocator);
	document.AddMember("vendorNotes", tmpval, allocator);

	// TODO: Check supported media types for CD/DVD/BD-ROM drives?
	// That's a bit more than an INQUIRY command...

	// Use pretty-printing.
	OStreamWrapper oswr(os);
	PrettyWriter<OStreamWrapper> writer(oswr);
	document.Accept(writer);
	return os;
}

/** AtaIdentifyDevice **/

AtaIdentifyDevice::AtaIdentifyDevice(RpFile *file, bool packet)
	: file(file)
	, packet(packet)
{}

ostream &operator<<(ostream &os, const AtaIdentifyDevice& si)
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
		os << "-- "
		   << fmt::format(FRUN(C_("rpcli", "ATA {:s} failed: {:0>8X}")),
			(si.packet ? "IDENTIFY PACKET DEVICE" : "IDENTIFY DEVICE"),
			static_cast<unsigned int>(ret))
		   << '\n';
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
	os << "Integrity word:        " << fmt::format(FSTR("{:0>4X}"), +resp.integrity) << '\n';
	return os;
}

/** JSONAtaIdentifyDevice **/

JSONAtaIdentifyDevice::JSONAtaIdentifyDevice(RpFile *file, bool packet)
	: file(file)
	, packet(packet)
{}

ostream &operator<<(ostream &os, const JSONAtaIdentifyDevice& si)
{
	Document document;
	document.SetObject();	// document should be an object, not an array
	Document::AllocatorType& allocator = document.GetAllocator();

	// NOTE: Unlike JSONRomOutput:
	// - Always using pretty-printing. (TODO: override if using '-J'?)
	// - Not setting CRLF mode; this should be printed to
	//   a terminal or redirected to a file in text mode.

	ATA_RESP_IDENTIFY_DEVICE resp;
	int ret;
	if (si.packet) {
		ret = si.file->ata_identify_packet_device(&resp);
	} else {
		ret = si.file->ata_identify_device(&resp);
	}

	if (ret != 0) {
		// TODO: Decode the error.
		Value error_val;
		error_val.SetString(
			fmt::format(FRUN(C_("rpcli", "ATA {:s} failed: {:0>8X}")),
				(si.packet ? "IDENTIFY PACKET DEVICE" : "IDENTIFY DEVICE"),
				static_cast<unsigned int>(ret)),
				allocator);
		document.AddMember("error", error_val, allocator);

		// Use pretty-printing.
		OStreamWrapper oswr(os);
		PrettyWriter<OStreamWrapper> writer(oswr);
		document.Accept(writer);
		return os;
	}

	// ATA device information.
	// TODO: Decode numeric values.
	// TODO: Trim spaces?
	// TODO: i18n?
	Value tmpval;

	tmpval.SetString(si.packet ? "ATA PACKET IDENTIFY DEVICE" : "ATA IDENTIFY DEVICE", allocator);
	document.AddMember("inquiryType", tmpval, allocator);

	tmpval.SetString(si.file->filename(), allocator);
	document.AddMember("deviceFilename", tmpval, allocator);

	tmpval.SetString(latin1_to_utf8(resp.model_number, sizeof(resp.model_number)), allocator);
	document.AddMember("modelNumber", tmpval, allocator);

	tmpval.SetString(latin1_to_utf8(resp.firmware_revision, sizeof(resp.firmware_revision)), allocator);
	document.AddMember("firmwareVersion", tmpval, allocator);

	tmpval.SetString(latin1_to_utf8(resp.serial_number, sizeof(resp.serial_number)), allocator);
	document.AddMember("serialNumber", tmpval, allocator);

	tmpval.SetString(latin1_to_utf8(resp.media_serial_number, sizeof(resp.media_serial_number)), allocator);
	document.AddMember("mediaSerialNumber", tmpval, allocator);

	// TODO: Byte count.

	tmpval.SetUint(resp.total_sectors);
	document.AddMember("sectorCount_28bit", tmpval, allocator);

	tmpval.SetUint64(resp.total_sectors_48);
	document.AddMember("sectorCount_48bit", tmpval, allocator);

	tmpval.SetString(fmt::format(FSTR("{:0>4X}"), +resp.integrity), allocator);
	document.AddMember("integrityWord", tmpval, allocator);

	// Use pretty-printing.
	OStreamWrapper oswr(os);
	PrettyWriter<OStreamWrapper> writer(oswr);
	document.Accept(writer);
	return os;
}

#endif /* RP_OS_SCSI_SUPPORTED */

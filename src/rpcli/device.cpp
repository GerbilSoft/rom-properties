/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * device.cpp: Extra functions for devices.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "device.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/file/scsi/scsi_protocol.h"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C++ includes.
#include <iomanip>
#include <iostream>
using std::endl;

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

ScsiInquiry::ScsiInquiry(RpFile *file)
	: file(file)
{ }

std::ostream& operator<<(std::ostream& os, const ScsiInquiry& si)
{
	SCSI_RESP_INQUIRY_STD resp;
	int ret = si.file->scsi_inquiry(&resp);
	if (ret != 0) {
		// TODO: Decode the error.
		os << "-- " << rp_sprintf(C_("rpcli", "SCSI INQUIRY failed: %08X"), ret) << endl;
		return os;
	}

	// SCSI device information.
	// TODO: Decode numeric values.
	// TODO: i18n?
	StreamStateSaver state(os);
	os << "-- SCSI INQUIRY data for: " << si.file->filename() << endl;
	os << "Peripheral device type: " << rp_sprintf("%02X", (resp.PeripheralDeviceType & 0x1F)) << endl;
	os << "Peripheral qualifier:   " << rp_sprintf("%02X", (resp.PeripheralDeviceType >> 5)) << endl;
	os << "Removable media:        " << (resp.RMB_DeviceTypeModifier & 0x80 ? "Yes" : "No") << endl;
	os << "Version:                " << rp_sprintf("%02X", resp.Version) << endl;
	// TODO: ResponseDataFormat and high bits?
	// TODO: Verify AdditionalLength field?
	// TODO: Flags.
	// TODO: Trim spaces.
	os << "Vendor ID:              " << latin1_to_utf8(resp.vendor_id, sizeof(resp.vendor_id)) << endl;
	os << "Product ID:             " << latin1_to_utf8(resp.product_id, sizeof(resp.product_id)) << endl;
	os << "Firmware version:       " << latin1_to_utf8(resp.product_revision_level, sizeof(resp.product_revision_level)) << endl;
	os << "Vendor notes:           " << latin1_to_utf8(resp.VendorSpecific, sizeof(resp.VendorSpecific)) << endl;
	return os;
}

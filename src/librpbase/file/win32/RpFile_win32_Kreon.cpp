/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_stdio_kreon.cpp: Standard file object. (stdio implementation)    *
 * Kreon-specific functions.                                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#include "../RpFile.hpp"
#include "RpFile_win32_p.hpp"

// NT DDK SCSI functions.
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
// Old MinGW has the NT DDK headers in include/ddk/.
// IOCTL headers conflict with WinDDK.
# include <ddk/ntddscsi.h>
# include <ddk/ntddstor.h>
#else
// MinGW-w64 and MSVC has the NT DDK headers in include/.
// IOCTL headers are also required.
# include <winioctl.h>
# include <ntddscsi.h>
#endif

// C++ includes.
#include <vector>
using std::vector;

namespace LibRpBase {

// TODO: Consolidate these functions to a common RpFile_Kreon.cpp
// file with OS-specific SCSI implementations.

// SCSI_PASS_THROUGH_DIRECT struct with extra space for sense data.
struct srb_t {
	SCSI_PASS_THROUGH_DIRECT p;
	uint8_t sense[78];	// Additional sense data. (TODO: Best size?)
};

/**
 * Is this a supported Kreon drive?
 *
 * NOTE: This only checks the drive vendor and model.
 * Check the feature list to determine if it's actually
 * using Kreon firmware.
 *
 * @return True if the drive supports Kreon firmware; false if not.
 */
bool RpFile::isKreonDriveModel(void) const
{
	RP_D(const RpFile);
	if (!d->isDevice) {
		// Not a device.
		return false;
	}

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
		uint8_t VendorSpecific[20];
		uint8_t Reserved2[40];
	} SCSI_RESP_INQUIRY_STD;
	SCSI_RESP_INQUIRY_STD resp;

	// SCSI request buffer.
	srb_t srb;
	memset(&srb, 0, sizeof(srb));

	// SCSI INQUIRY command.
	srb.p.Cdb[0] = 0x12;
	srb.p.Cdb[1] = 0x00;
	srb.p.Cdb[2] = 0x00;
	srb.p.Cdb[3] = sizeof(SCSI_RESP_INQUIRY_STD) >> 8;
	srb.p.Cdb[4] = sizeof(SCSI_RESP_INQUIRY_STD) & 0xFF;
	srb.p.Cdb[5] = 0x00;

	srb.p.DataBuffer = &resp;
	srb.p.DataTransferLength = sizeof(resp);
	srb.p.DataIn = SCSI_IOCTL_DATA_IN;
	srb.p.CdbLength = 6;
	srb.p.Length = sizeof(srb.p);
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = sizeof(srb.p);
	srb.p.TimeOutValue = 5; // 5-second timeout.

	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		d->file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		(LPVOID)&srb.p, sizeof(srb.p),
		(LPVOID)&srb, sizeof(srb),
		&dwBytesReturned, nullptr);
	if (!bRet) {
		// DeviceIoControl() failed.
		DWORD x = GetLastError();
		return false;
	}
	// TODO: Check dwBytesReturned.

	// Check the drive vendor and product ID.
	if (!memcmp(resp.vendor_id, "TSSTcorp", 8)) {
		// Correct vendor ID.
		// Check for supported product IDs.
		// NOTE: More drive models are supported, but the
		// Kreon firmware only uses these product IDs.
		static const char *const product_id_tbl[] = {
			"DVD-ROM SH-D162C",
			"DVD-ROM TS-H353A",
			"DVD-ROM SH-D163B",
		};
		for (int i = 0; i < ARRAY_SIZE(product_id_tbl); i++) {
			if (!memcmp(resp.product_id, product_id_tbl[i], sizeof(resp.product_id))) {
				// Found a match.
				return true;
			}
		}
	}

	// Not supported.
	return false;
}

/**
 * Get a list of supported Kreon features.
 * @return List of Kreon feature IDs, or empty vector if not supported.
 */
vector<uint16_t> RpFile::getKreonFeatureList(void) const
{
	RP_D(const RpFile);
	vector<uint16_t> vec;

	if (!d->isDevice) {
		// Not a device.
		return vec;
	}

	uint8_t feature_buf[26];

	// SCSI request buffer.
	srb_t srb;
	memset(&srb, 0, sizeof(srb));

	// Kreon "Get Feature List" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1223
	srb.p.Cdb[0] = 0xFF;
	srb.p.Cdb[1] = 0x08;
	srb.p.Cdb[2] = 0x01;
	srb.p.Cdb[3] = 0x10;
	srb.p.Cdb[4] = 0x00;
	srb.p.Cdb[5] = 0x00;

	srb.p.DataBuffer = feature_buf;
	srb.p.DataTransferLength = sizeof(feature_buf);
	srb.p.DataIn = SCSI_IOCTL_DATA_IN;
	srb.p.CdbLength = 6;
	srb.p.Length = sizeof(srb.p);
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = sizeof(srb.p);
	srb.p.TimeOutValue = 5; // 5-second timeout.

	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		d->file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		(LPVOID)&srb.p, sizeof(srb.p),
		(LPVOID)&srb, sizeof(srb),
		&dwBytesReturned, nullptr);
	if (!bRet) {
		// DeviceIoControl() failed.
		return vec;
	}
	// TODO: Check dwBytesReturned.

	vec.reserve(sizeof(feature_buf)/sizeof(uint16_t));
	for (size_t i = 0; i < sizeof(feature_buf); i += 2) {
		const uint16_t feature = (feature_buf[i] << 8) | feature_buf[i+1];
		if (feature == 0)
			break;
		vec.push_back(feature);
	}

	if (vec.size() < 2 || vec[0] != KREON_FEATURE_HEADER_0 ||
	    vec[1] != KREON_FEATURE_HEADER_1)
	{
		// Kreon feature list is invalid.
		vec.clear();
		vec.shrink_to_fit();
	}

	return vec;
}

/**
 * Set Kreon error skip state.
 * @param skip True to skip; false for normal operation.
 * @return 0 on success; non-zero on error.
 */
int RpFile::setKreonErrorSkipState(bool skip)
{
	RP_D(const RpFile);
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI request buffer.
	srb_t srb;
	memset(&srb, 0, sizeof(srb));

	// Kreon "Set Error Skip State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1341
	srb.p.Cdb[0] = 0xFF;
	srb.p.Cdb[1] = 0x08;
	srb.p.Cdb[2] = 0x01;
	srb.p.Cdb[3] = 0x15;
	srb.p.Cdb[4] = (uint8_t)skip;
	srb.p.Cdb[5] = 0x00;

	srb.p.DataBuffer = nullptr;
	srb.p.DataTransferLength = 0;
	srb.p.DataIn = SCSI_IOCTL_DATA_IN;
	srb.p.CdbLength = 6;
	srb.p.Length = sizeof(srb.p);
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = sizeof(srb.p);
	srb.p.TimeOutValue = 5; // 5-second timeout.

	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		d->file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		(LPVOID)&srb.p, sizeof(srb.p),
		(LPVOID)&srb, sizeof(srb),
		&dwBytesReturned, nullptr);
	if (!bRet) {
		// DeviceIoControl() failed.
		return -EIO;
	}
	// TODO: Check dwBytesReturned.

	// Command succeeded.
	return 0;
}

/**
 * Set Kreon lock state
 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
 * @return 0 on success; non-zero on error.
 */
int RpFile::setKreonLockState(uint8_t lockState)
{
	RP_D(const RpFile);
	// NOTE: On Linux, this ioctl will fail if not running as root.
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI request buffer.
	srb_t srb;
	memset(&srb, 0, sizeof(srb));

	// Kreon "Set Lock State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1309
	srb.p.Cdb[0] = 0xFF;
	srb.p.Cdb[1] = 0x08;
	srb.p.Cdb[2] = 0x01;
	srb.p.Cdb[3] = 0x11;
	srb.p.Cdb[4] = (uint8_t)lockState;
	srb.p.Cdb[5] = 0x00;

	srb.p.DataBuffer = nullptr;
	srb.p.DataTransferLength = 0;
	srb.p.DataIn = SCSI_IOCTL_DATA_IN;
	srb.p.CdbLength = 6;
	srb.p.Length = sizeof(srb.p);
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = sizeof(srb.p);
	srb.p.TimeOutValue = 5; // 5-second timeout.

	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		d->file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		(LPVOID)&srb.p, sizeof(srb.p),
		(LPVOID)&srb, sizeof(srb),
		&dwBytesReturned, nullptr);
	if (!bRet) {
		// DeviceIoControl() failed.
		return -EIO;
	}
	// TODO: Check dwBytesReturned.

	// Command succeeded.
	return 0;
}

}

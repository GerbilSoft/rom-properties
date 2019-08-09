/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_scsi_win32.cpp: Standard file object. (Win32 SCSI)               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef _WIN32
# error RpFile_scsi_linux.cpp is for Windows ONLY.
#endif /* _WIN32 */

#include "../RpFile.hpp"
#include "../win32/RpFile_win32_p.hpp"

#include "scsi_protocol.h"

#include "libwin32common/w32err.h"
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

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRpBase {

/**
 * Send a SCSI command to the device.
 * @param cdb		[in] SCSI command descriptor block
 * @param cdb_len	[in] Length of cdb
 * @param data		[in/out] Data buffer, or nullptr for SCSI_DIR_NONE operations
 * @param data_len	[in] Length of data
 * @param direction	[in] Data direction
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::scsi_send_cdb(const void *cdb, uint8_t cdb_len,
	void *data, size_t data_len,
	ScsiDirection direction)
{
	// SCSI_PASS_THROUGH_DIRECT struct with extra space for sense data.
	struct srb_t {
		SCSI_PASS_THROUGH_DIRECT p;
		struct {
			SCSI_RESP_REQUEST_SENSE s;
			uint8_t b[78];	// Additional sense data. (TODO: Best size?)
		} sense;
	};
	srb_t srb;
	memset(&srb, 0, sizeof(srb));

	// Copy the CDB to the SCSI_PASS_THROUGH structure.
	assert(cdb_len <= sizeof(srb.p.Cdb));
	if (cdb_len > sizeof(srb.p.Cdb)) {
		// CDB is too big.
		return -EINVAL;
	}
	memcpy(srb.p.Cdb, cdb, cdb_len);

	// Data direction and buffer.
	switch (direction) {
		case SCSI_DIR_NONE:
			srb.p.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
			break;
		case SCSI_DIR_IN:
			srb.p.DataIn = SCSI_IOCTL_DATA_IN;
			break;
		case SCSI_DIR_OUT:
			srb.p.DataIn = SCSI_IOCTL_DATA_OUT;
			break;
		default:
			assert(!"Invalid SCSI direction.");
			return -EINVAL;
	}

	// Parameters.
	srb.p.DataBuffer = data;
	srb.p.DataTransferLength = static_cast<ULONG>(data_len);
	srb.p.CdbLength = cdb_len;
	srb.p.Length = sizeof(srb.p);
	srb.p.SenseInfoLength = sizeof(srb.sense);
	srb.p.SenseInfoOffset = offsetof(srb_t, sense.s);
	srb.p.TimeOutValue = 5; // 5-second timeout.

	RP_D(RpFile);
	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		d->file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		(LPVOID)&srb.p, sizeof(srb.p),
		(LPVOID)&srb, sizeof(srb),
		&dwBytesReturned, nullptr);
	if (!bRet) {
		// DeviceIoControl() failed.
		// TODO: Convert Win32 to POSIX?
		return -w32err_to_posix(GetLastError());
	}
	// TODO: Check dwBytesReturned.

	// Check if the command succeeded.
	int ret;
	switch (srb.sense.s.ErrorCode) {
		case SCSI_ERR_REQUEST_SENSE_CURRENT:
		case SCSI_ERR_REQUEST_SENSE_DEFERRED:
			// Error. Return the sense key.
			ret = (srb.sense.s.SenseKey << 16) |
			      (srb.sense.s.AddSenseCode << 8) |
			      (srb.sense.s.AddSenseQual);
			break;

		case SCSI_ERR_REQUEST_SENSE_CURRENT_DESC:
		case SCSI_ERR_REQUEST_SENSE_DEFERRED_DESC:
			// Error, but using descriptor format.
			// Return a generic error.
			ret = -EIO;
			break;

		default:
			// No error.
			ret = 0;
			break;
	}

	return ret;
}

}

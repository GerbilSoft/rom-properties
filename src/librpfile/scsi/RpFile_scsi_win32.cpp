/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_scsi_win32.cpp: Standard file object. (Win32 SCSI)               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef _WIN32
#  error RpFile_scsi_linux.cpp is for Windows ONLY.
#endif /* _WIN32 */

#include "stdafx.h"
#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

#include "scsi_protocol.h"

#include "libwin32common/w32err.hpp"

// Windows 8.1 SDK's ntddscsi.h doesn't prpoerly declare
// _NV_SEP_WRITE_CACHE_TYPE as an enum, so MSVC complains
// that the typedef is unnecessary.
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4091)
#endif /* _MSC_VER */

// NT DDK SCSI functions
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
// Old MinGW has the NT DDK headers in include/ddk/.
// IOCTL headers conflict with WinDDK.
#  include <ddk/ntddscsi.h>
#  include <ddk/ntddstor.h>
#else
// MinGW-w64 and MSVC has the NT DDK headers in include/.
// IOCTL headers are also required.
#  include <winioctl.h>
#  include <ntddscsi.h>
#endif

#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */

#include "tcharx.h"

namespace LibRpFile {

/**
 * Re-read device size using the native OS API.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeOS(off64_t *pDeviceSize, uint32_t *pSectorSize)
{
	// NOTE: IOCTL_DISK_GET_DRIVE_GEOMETRY_EX seems to report 512-byte sectors
	// for certain emulated CD-ROM device, e.g. the Verizon LG G2.
	// GetDiskFreeSpace() reports the correct value (2048).
	DWORD dwSectorsPerCluster, dwBytesPerSector;
	DWORD dwNumberOfFreeClusters, dwTotalNumberOfClusters;
	DWORD w32err = 0;

	RP_D(RpFile);
	if (unlikely(d->filenameW.empty())) {
		// No filename... Assume this isn't a device.
		return -ENODEV;
	}
	const TCHAR drive_name[4] = {static_cast<TCHAR>(d->filenameW[0]), _T(':'), _T('\\'), 0};
	BOOL bRet = GetDiskFreeSpace(drive_name,
		&dwSectorsPerCluster, &dwBytesPerSector,
		&dwNumberOfFreeClusters, &dwTotalNumberOfClusters);
	if (bRet && dwBytesPerSector >= 512 && dwTotalNumberOfClusters > 0) {
		// TODO: Make sure the sector size is a power of 2
		// and isn't a ridiculous value.

		// Save the device size and sector size.
		// NOTE: GetDiskFreeSpaceEx() eliminates the need for multiplications,
		// but it doesn't provide dwBytesPerSector.
		d->devInfo->device_size =
			static_cast<off64_t>(dwBytesPerSector) *
			static_cast<off64_t>(dwSectorsPerCluster) *
			static_cast<off64_t>(dwTotalNumberOfClusters);
		d->devInfo->sector_size = dwBytesPerSector;
	} else {
		// GetDiskFreeSpace() failed.
		w32err = GetLastError();
		if (w32err == ERROR_INVALID_PARAMETER) {
			// The disk may use some file system that
			// Windows doesn't recognize.
			// Try IOCTL_DISK_GET_DRIVE_GEOMETRY_EX instead.
			DISK_GEOMETRY_EX dg;
			DWORD dwBytesReturned;  // TODO: Check this?
			if (DeviceIoControl(d->file, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
				NULL, 0, &dg, sizeof(dg), &dwBytesReturned, NULL) != 0)
			{
				// Device geometry retrieved.
				w32err = 0;
				d->devInfo->device_size = dg.DiskSize.QuadPart;
				d->devInfo->sector_size = dg.Geometry.BytesPerSector;
			} else {
				// IOCTL failed.
				d->devInfo->device_size = 0;
				d->devInfo->sector_size = 0;

				w32err = GetLastError();
				if (w32err == 0) {
					w32err = ERROR_INVALID_PARAMETER;
				}
			}
		}
	}

	if (w32err == 0) {
		// Validate the sector size.
		assert(d->devInfo->sector_size >= 512);
		assert(d->devInfo->sector_size <= 65536);
		if (d->devInfo->sector_size < 512 || d->devInfo->sector_size > 65536) {
			// Sector size is out of range.
			// TODO: Also check for isPow2()?
			d->devInfo->device_size = 0;
			d->devInfo->sector_size = 0;
			return -EIO;
		}

		// Return the values.
		if (pDeviceSize) {
			*pDeviceSize = d->devInfo->device_size;
		}
		if (pSectorSize) {
			*pSectorSize = d->devInfo->sector_size;
		}
		return 0;
	}

	return w32err_to_posix(w32err);
}

/**
 * Send a SCSI command to the device.
 * @param cdb		[in] SCSI command descriptor block
 * @param cdb_len	[in] Length of cdb
 * @param data		[in/out] Data buffer, or nullptr for ScsiDirection::None operations
 * @param data_len	[in] Length of data
 * @param direction	[in] Data direction
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFilePrivate::scsi_send_cdb(const void *cdb, uint8_t cdb_len,
	void *data, size_t data_len,
	ScsiDirection direction)
{
	assert(cdb_len >= 6);
	if (cdb_len < 6) {
		return -EINVAL;
	}

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
		case ScsiDirection::None:
			srb.p.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
			break;
		case ScsiDirection::In:
			srb.p.DataIn = SCSI_IOCTL_DATA_IN;
			break;
		case ScsiDirection::Out:
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

	DWORD dwBytesReturned;
	BOOL bRet = DeviceIoControl(
		file, IOCTL_SCSI_PASS_THROUGH_DIRECT,
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

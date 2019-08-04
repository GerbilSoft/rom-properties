/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_Kreon.cpp: Standard file object. (Kreon-specific functions)      *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile.hpp"

#include "scsi_protocol.h"
#include "../byteswap.h"

#ifdef _WIN32
# include "libwin32common/w32err.h"
// NT DDK SCSI functions.
# if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
// Old MinGW has the NT DDK headers in include/ddk/.
// IOCTL headers conflict with WinDDK.
#  include <ddk/ntddscsi.h>
#  include <ddk/ntddstor.h>
# else
// MinGW-w64 and MSVC has the NT DDK headers in include/.
// IOCTL headers are also required.
#  include <winioctl.h>
#  include <ntddscsi.h>
# endif
#endif

#ifdef _WIN32
# include "win32/RpFile_win32_p.hpp"
#else /* !_WIN32 */
# include "RpFile_stdio_p.hpp"
#endif /* _WIN32 */

// SCSI and CD-ROM IOCTLs.
#ifdef __linux__
# include <sys/ioctl.h>
# include <scsi/sg.h>
# include <scsi/scsi.h>
# include <linux/cdrom.h>
#endif /* __linux__ */

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

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
	int ret = -EIO;

#if defined(_WIN32)
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
#elif defined(__linux__)
	// SCSI command buffers.
	struct sg_io_hdr sg_io;
	union {
		struct request_sense s;
		uint8_t u[18];
	} _sense;

	// TODO: Consolidate this.
	memset(&sg_io, 0, sizeof(sg_io));
	sg_io.interface_id = 'S';
	sg_io.mx_sb_len = sizeof(_sense);
	sg_io.sbp = _sense.u;
	sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;

	sg_io.cmdp = (unsigned char*)cdb;
	sg_io.cmd_len = cdb_len;

	switch (direction) {
		case SCSI_DIR_NONE:
			sg_io.dxfer_direction = SG_DXFER_NONE;
			break;
		case SCSI_DIR_IN:
			sg_io.dxfer_direction = SG_DXFER_FROM_DEV;
			break;
		case SCSI_DIR_OUT:
			sg_io.dxfer_direction = SG_DXFER_TO_DEV;
			break;
		default:
			assert(!"Invalid SCSI direction.");
			return -EINVAL;
	}
	sg_io.dxferp = data;
	sg_io.dxfer_len = data_len;

	RP_D(RpFile);
	if (ioctl(fileno(d->file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return -errno;
	}

	// Check if the command succeeded.
	if ((sg_io.info & SG_INFO_OK_MASK) == SG_INFO_OK) {
		// Command succeeded.
		ret = 0;
	} else {
		// Command failed.
		ret = -EIO;
		if (sg_io.masked_status & CHECK_CONDITION) {
			ret = ERRCODE(_sense.u);
			if (ret == 0) {
				ret = -EIO;
			}
		}
	}
#elif defined(__APPLE__)
	// Mac OS X doesn't allow sending arbitrary SCSI commands
	// unless an application-specific kernel driver is installed.
	// References:
	// - https://stackoverflow.com/questions/7349030/sending-a-specific-scsi-command-to-a-scsi-device-in-mac-os-x
	// - https://stackoverflow.com/a/7349373
	// - http://developer.apple.com/library/mac/#documentation/DeviceDrivers/Conceptual/WorkingWithSAM/WWS_SAMDevInt/WWS_SAM_DevInt.html
	return -ENOSYS;
#else
# error No SCSI implementation for this OS.
#endif
	return ret;
}

/**
 * Re-read device size using SCSI commands.
 * This may be needed for Kreon devices.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeScsi(int64_t *pDeviceSize, uint32_t *pSectorSize)
{
	RP_D(RpFile);
	if (!d->isDevice) {
		// Not a device.
		return -ENOTSUP;
	}

	int64_t device_size;
	uint32_t sector_size;
	int ret = scsi_read_capacity(&device_size, &sector_size);
	if (ret != 0) {
		// Error reading capacity.
		return ret;
	}

#ifdef _WIN32
	// Sector size should match.
	assert(d->sector_size == sector_size);

	// Update the device size.
	d->device_size = device_size;
#endif /* _WIN32 */

	// Return the values.
	if (pDeviceSize) {
		*pDeviceSize = device_size;
	}
	if (pSectorSize) {
		*pSectorSize = sector_size;
	}
	return 0;
}

/**
 * Get the capacity of the device using SCSI commands.
 * @param pDeviceSize	[out] Retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::scsi_read_capacity(int64_t *pDeviceSize, uint32_t *pSectorSize)
{
	assert(pDeviceSize != nullptr);
	if (!pDeviceSize)
		return -EINVAL;

	RP_D(RpFile);
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI command buffers.
	union {
		SCSI_CDB_READ_CAPACITY_10 cdb10;
		SCSI_CDB_READ_CAPACITY_16 cdb16;
	};
	union {
		SCSI_RESP_READ_CAPACITY_10 resp10;
		SCSI_RESP_READ_CAPACITY_16 resp16;
	};

	// NOTE: The returned LBA is the *last* LBA,
	// not the total number of LBAs. Hence, we'll
	// need to add one.

	// Try READ CAPACITY(10) first.
	cdb10.OpCode = SCSI_OP_READ_CAPACITY_10;
	cdb10.RelAdr = 0;
	cdb10.LBA = 0;
	cdb10.Reserved[0] = 0;
	cdb10.Reserved[1] = 0;
	cdb10.PMI = 0;
	cdb10.Control = 0;

	int ret = scsi_send_cdb(&cdb10, sizeof(cdb10), &resp10, sizeof(resp10), SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		return ret;
	}

	if (resp10.LBA != 0xFFFFFFFF) {
		// READ CAPACITY(10) has the full capacity.
		const uint32_t sector_size = be32_to_cpu(resp10.BlockLen);
		if (pSectorSize) {
			*pSectorSize = sector_size;
		}
		*pDeviceSize = static_cast<int64_t>(be32_to_cpu(resp10.LBA) + 1) *
			       static_cast<int64_t>(sector_size);
		return 0;
	}

	// READ CAPACITY(10) is truncated.
	// Try READ CAPACITY(16).
	cdb16.OpCode = SCSI_OP_SERVICE_ACTION_IN_16;
	cdb16.SAIn_OpCode = SCSI_SAIN_OP_READ_CAPACITY_16;
	cdb16.LBA = 0;
	cdb16.AllocLen = 0;
	cdb16.Reserved = 0;
	cdb16.Control = 0;

	ret = scsi_send_cdb(&cdb16, sizeof(cdb16), &resp16, sizeof(resp16), SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		// TODO: Return 0xFFFFFFFF+1 blocks anyway?
		return ret;
	}

	const uint32_t sector_size = be32_to_cpu(resp16.BlockLen);
	if (pSectorSize) {
		*pSectorSize = sector_size;
	}
	*pDeviceSize = static_cast<int64_t>(be64_to_cpu(resp16.LBA) + 1) *
		       static_cast<int64_t>(sector_size);
	return 0;
}

#ifdef _WIN32
/**
 * Read data from a device using SCSI commands.
 * @param lbaStart	[in] Starting LBA of the data to read.
 * @param lbaCount	[in] Number of LBAs to read.
 * @param pBuf		[out] Output buffer.
 * @param bufLen	[in] Output buffer length.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::scsi_read(uint32_t lbaStart, uint16_t lbaCount, uint8_t *pBuf, size_t bufLen)
{
	assert(pBuf != nullptr);
	if (!pBuf)
		return -EINVAL;

	// FIXME: d->sector_size is only in the Windows-specific class right now.
	RP_D(RpFile);
	assert(bufLen >= (int64_t)lbaCount * (int64_t)d->sector_size);
	if (bufLen < (int64_t)lbaCount * (int64_t)d->sector_size)
		return -EIO;	// TODO: Better error code?

	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI command buffers.
	// NOTE: Using READ(10), which has 32-bit LBA and transfer length.
	// READ(6) has 21-bit LBA and 8-bit transfer length.
	// TODO: May need to use READ(32) for large devices.
	SCSI_CDB_READ_10 cdb10;

	// SCSI READ(10)
	cdb10.OpCode = SCSI_OP_READ_10;
	cdb10.Flags = 0;
	cdb10.LBA = cpu_to_be32(lbaStart);
	cdb10.Reserved = 0;
	cdb10.TransferLen = cpu_to_be16(lbaCount);
	cdb10.Control = 0;

	return scsi_send_cdb(&cdb10, sizeof(cdb10), pBuf, bufLen, SCSI_DIR_IN);
}
#endif /* _WIN32 */

/**
 * Is this a supported Kreon drive?
 *
 * NOTE: This only checks the drive vendor and model.
 * Check the feature list to determine if it's actually
 * using Kreon firmware.
 *
 * @return True if the drive supports Kreon firmware; false if not.
 */
bool RpFile::isKreonDriveModel(void)
{
	RP_D(RpFile);
	if (!d->isDevice) {
		// Not a device.
		return false;
	}

	// SCSI INQUIRY command.
	SCSI_CDB_INQUIRY cdb;
	cdb.OpCode = SCSI_OP_INQUIRY;
	cdb.EVPD = 0;
	cdb.PageCode = 0;
	cdb.AllocLen = cpu_to_be16(sizeof(SCSI_RESP_INQUIRY_STD));
	cdb.Control = 0;

	SCSI_RESP_INQUIRY_STD resp;
	int ret = scsi_send_cdb(&cdb, sizeof(cdb), &resp, sizeof(resp), SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		return false;
	}

	// Check the device type, vendor, and product ID.
	if ((resp.PeripheralDeviceType & 0x1F) != SCSI_DEVICE_TYPE_CDROM) {
		// Wrong type of device.
		return false;
	}

	// TODO: Optimize by removing pointers? Doing so would require
	// direct use of character arrays, which is ugly...

	// TSSTcorp (Toshiba/Samsung)
	static const char *const TSSTcorp_product_tbl[] = {
		// Kreon
		"DVD-ROM SH-D162C",
		"DVD-ROM TS-H353A",
		"DVD-ROM SH-D163B",

		// 360
		"DVD-ROM TS-H943A",
		nullptr
	};

	// Philips/BenQ Digital Storage
	static const char *const PBDS_product_tbl[] = {
		"VAD6038         ",
		"VAD6038-64930C  ",
		nullptr
	};

	// Hitachi-LG Data Storage
	static const char *const HLDTST_product_tbl[] = {
		"DVD-ROM GDR3120L",	// Phat
		nullptr
	};

	// Vendor table.
	// NOTE: Vendor strings MUST be 8 characters long.
	// NOTE: Strings in product ID tables MUST be 16 characters long.
	static const struct {
		const char *vendor;
		const char *const *product_id_tbl;
	} vendor_tbl[] = {
		{"TSSTcorp", TSSTcorp_product_tbl},
		{"PBDS    ", PBDS_product_tbl},
		{"HL-DT-ST", HLDTST_product_tbl},
		{nullptr, nullptr}
	};

	// Find the vendor.
	const char *const *pProdTbl = nullptr;
	for (auto *pVendorTbl = vendor_tbl; pVendorTbl->vendor != nullptr; pVendorTbl++) {
		if (!memcmp(resp.vendor_id, pVendorTbl->vendor, 8)) {
			// Found a match.
			pProdTbl = pVendorTbl->product_id_tbl;
			break;
		}
	}
	if (!pProdTbl) {
		// Not found.
		return false;
	}

	// Check if the product ID is supported.
	bool found = false;
	for (; *pProdTbl != nullptr; pProdTbl++) {
		if (!memcmp(resp.product_id, *pProdTbl, 16)) {
			// Found a match.
			found = true;
			break;
		}
	}
	return found;
}

/**
 * Get a list of supported Kreon features.
 * @return List of Kreon feature IDs, or empty vector if not supported.
 */
vector<uint16_t> RpFile::getKreonFeatureList(void)
{
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	vector<uint16_t> vec;
	if (!d->isDevice) {
		// Not a device.
		return vec;
	}

	// Kreon "Get Feature List" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1223
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x10, 0x00, 0x00};
	uint8_t feature_buf[26];
	int ret = scsi_send_cdb(cdb, sizeof(cdb), feature_buf, sizeof(feature_buf), SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		return vec;
	}

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
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// Kreon "Set Error Skip State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1341
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x15, (uint8_t)skip, 0x00};
	return scsi_send_cdb(cdb, sizeof(cdb), nullptr, 0, SCSI_DIR_IN);
}

/**
 * Set Kreon lock state
 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
 * @return 0 on success; non-zero on error.
 */
int RpFile::setKreonLockState(KreonLockState lockState)
{
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// Kreon "Set Lock State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1309
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x11, static_cast<uint8_t>(lockState), 0x00};
	int ret = scsi_send_cdb(cdb, sizeof(cdb), nullptr, 0, SCSI_DIR_IN);
	if (ret == 0) {
		d->isKreonUnlocked = (lockState != KREON_STATE_LOCKED);
	}
	return ret;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_scsi_linux.cpp: Standard file object. (Linux SCSI)               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __linux__
# error RpFile_scsi_linux.cpp is for Linux ONLY.
#endif /* __linux__ */

#include "stdafx.h"
#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

#include "scsi_protocol.h"

// SCSI and CD-ROM IOCTLs.
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <linux/cdrom.h>
#include <linux/fs.h>

namespace LibRpFile {

/**
 * Re-read device size using the native OS API.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeOS(off64_t *pDeviceSize, uint32_t *pSectorSize)
{
	RP_D(RpFile);
	const int fd = fileno(d->file);

	if (ioctl(fd, BLKGETSIZE64, &d->devInfo->device_size) < 0) {
		d->devInfo->device_size = 0;
		d->devInfo->sector_size = 0;
		return -errno;
	}

	if (ioctl(fd, BLKSSZGET, &d->devInfo->sector_size) < 0) {
		d->devInfo->device_size = 0;
		d->devInfo->sector_size = 0;
		return -errno;
	}

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

/**
 * Send a SCSI command to the device.
 * @param cdb		[in] SCSI command descriptor block
 * @param cdb_len	[in] Length of cdb
 * @param data		[in/out] Data buffer, or nullptr for ScsiDirection::None operations
 * @param data_len	[in] Length of data
 * @param direction	[in] Data direction
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFilePrivate::scsi_send_cdb(const void *cdb, size_t cdb_len,
	void *data, size_t data_len,
	ScsiDirection direction)
{
	assert(cdb_len >= 6);
	assert(cdb_len <= 260);
	if (cdb_len < 6 || cdb_len > 260) {
		return -EINVAL;
	}

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
		case ScsiDirection::None:
			sg_io.dxfer_direction = SG_DXFER_NONE;
			break;
		case ScsiDirection::In:
			sg_io.dxfer_direction = SG_DXFER_FROM_DEV;
			break;
		case ScsiDirection::Out:
			sg_io.dxfer_direction = SG_DXFER_TO_DEV;
			break;
		default:
			assert(!"Invalid SCSI direction.");
			return -EINVAL;
	}
	sg_io.dxferp = data;
	sg_io.dxfer_len = data_len;

	if (ioctl(fileno(file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return -errno;
	}

	// Check if the command succeeded.
	int ret;
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
	return ret;
}

}

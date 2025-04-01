/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_scsi_freebsd.cpp: Standard file object. (FreeBSD SCSI)           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#if !defined(__FreeBSD__) && !defined(__DragonFly__)
# error RpFile_scsi_freebsd.cpp is for FreeBSD and DragonFly BSD ONLY.
#endif /* !__FreeBSD__ && !__DragonFly__ */

#include "stdafx.h"
#include "config.librpfile.h"

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

#include "scsi_protocol.h"

// C includes.
#include <fcntl.h>

// SCSI and CD-ROM IOCTLs.
#ifdef RP_OS_SCSI_SUPPORTED
#include <camlib.h>
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>
#endif /* RP_OS_SCSI_SUPPORTED */
#include <sys/cdio.h>
#include <sys/disk.h>

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

	// NOTE: DIOCGMEDIASIZE uses off_t, not off64_t.
	off_t device_size = 0;

	if (ioctl(fd, DIOCGMEDIASIZE, &device_size) < 0) {
		d->devInfo->device_size = 0;
		d->devInfo->sector_size = 0;
		return -errno;
	}
	d->devInfo->device_size = static_cast<off64_t>(device_size);

	if (ioctl(fd, DIOCGSECTORSIZE, &d->devInfo->sector_size) < 0) {
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
	// Partially based on libcdio-2.1.0's freebsd_cam.c.
	assert(cdb_len >= 6);
	assert(cdb_len <= 260);
	if (cdb_len < 6 || cdb_len > 260) {
		return -EINVAL;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// SCSI command buffer.
	union ccb ccb;
	memset(&ccb, 0, sizeof(ccb));

	struct cam_device *cam = devInfo->cam;
	if (!cam) {
		// Get the device name for CAM.
		const int fd = fileno(file);
		if (ioctl(fd, CDIOCALLOW) == -1) {
			// CDIOCALLOW failed.
			return -errno;
		}
		if (ioctl(fd, CAMGETPASSTHRU, &ccb) < 0) {
			// CAMGETPASSTHRU failed.
			return -errno;
		}
		char pass[40];
		snprintf(pass, sizeof(pass), "/dev/%.15s%u", ccb.cgdl.periph_name, ccb.cgdl.unit_number);

		struct cam_device *cam = cam_open_pass(pass, O_RDWR, nullptr);
		if (!cam) {
			// Unable to open the CAM device.
			return -EIO;
		}

		// Save the CAM device for later.
		devInfo->cam = cam;
	}

	// Create the command.
	memset(&ccb, 0, sizeof(ccb));
	ccb.ccb_h.path_id = cam->path_id;
	ccb.ccb_h.target_id = cam->target_id;
	ccb.ccb_h.target_lun = cam->target_lun;
	ccb.ccb_h.timeout = 20;	// 20ms timeout

	int ccb_direction = CAM_DEV_QFRZDIS | CAM_PASS_ERR_RECOVER;
	switch (direction) {
		case ScsiDirection::None:
			ccb_direction |= CAM_DIR_NONE;
			break;
		case ScsiDirection::In:
			ccb_direction |= CAM_DIR_IN;
			break;
		case ScsiDirection::Out:
			ccb_direction |= CAM_DIR_OUT;
			break;
		default:
			assert(!"Invalid SCSI direction.");
			cam_close_device(cam);
			return -EINVAL;
	}

	// Copy the CDB.
	memcpy(ccb.csio.cdb_io.cdb_bytes, cdb, cdb_len);
	ccb.csio.cdb_len = cdb_len;

	// Fill other CSIO stuff.
	cam_fill_csio(&(ccb.csio), 1, nullptr,
		ccb_direction | CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG,
		static_cast<uint8_t*>(data), data_len,
		sizeof(ccb.csio.sense_data), ccb.csio.cdb_len, 30*1000);

	// Attempt to send the CDB.
	if (cam_send_ccb(cam, &ccb) < 0) {
		// Error sending the CDB.
		return -errno;
	}

	if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP) {
		// CDB submitted successfully.
		return 0;
	}

	// Check the SCSI sense key.
	int ret = ERRCODE(reinterpret_cast<unsigned char*>(&ccb.csio.sense_data));
	if (ret == 0) {
		ret = -EIO;
	}
	return ret;
#else /* !RP_OS_SCSI_SUPPORTED */
	// Not supported yet.
	return -ENOSYS;
#endif /* RP_OS_SCSI_SUPPORTED */
}

}

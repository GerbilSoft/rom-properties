/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_scsi_netbsd.cpp: Standard file object. (NetBSD/OpenBSD SCSI)     *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#if !defined(__NetBSD__) && !defined(__OpenBSD__)
# error RpFile_scsi_netbsd.cpp is for NetBSD and OpenBSD ONLY.
#endif /* __linux__ */

#include "stdafx.h"
#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

#include "scsi_protocol.h"

// SCSI and CD-ROM IOCTLs.
#include <sys/dkio.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
//#include <sys/disk.h>
//#include <sys/cdio.h>

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

#if defined(DIOCGMEDIASIZE) && defined(DIOCGSECTORSIZE)
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
#elif defined(DIOCGDINFO)
	struct disklabel dl;
	if (ioctl(fd, DIOCGDINFO, &dl) < 0) {
		d->devInfo->device_size = 0;
		d->devInfo->sector_size = 0;
		return -errno;
	}

	// TODO: Verify how >2TB devices with 512-byte sectors are handled.
	// dl.d_secperunit * sector_size == device_size
	d->devInfo->device_size = (off64_t)dl.d_secperunit * (off64_t)dl.d_secsize;
	d->devInfo->sector_size = dl.d_secsize;
#else
# error No IOCTLs available.
#endif

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
	// Partially based on libcdio-2.1.0's run_scsi_cmd_netbsd().
	assert(cdb_len >= 6);
	assert(cdb_len <= 260);
	if (cdb_len < 6 || cdb_len > 260) {
		return -EINVAL;
	}

	// SCSI command buffers.
	// TODO: Sense data?
	scsireq_t req;
	assert(cdb_len <= sizeof(req.cmd));
	if (cdb_len > sizeof(req.cmd)) {
		// CDB is too big.
		return -EINVAL;
	}
	memset(&req, 0, sizeof(req));
	memcpy(req.cmd, cdb, cdb_len);
	req.cmdlen = static_cast<u_char>(cdb_len);
	req.datalen = data_len;
	req.databuf = static_cast<caddr_t>(data);
	req.timeout = 20;	// TODO

	switch (direction) {
		case ScsiDirection::None:
		case ScsiDirection::In:
			req.flags = SCCMD_READ;
			break;
		case ScsiDirection::Out:
			req.flags = SCCMD_WRITE;
			break;
		default:
			assert(!"Invalid SCSI direction.");
			return -EINVAL;
	}

	if (ioctl(fileno(file), SCIOCCOMMAND, &req) < 0) {
		// ioctl failed.
		return -errno;
	}

	// Check if the command succeeded.
	int ret = 0;
	if (req.retsts != SCCMD_OK) {
		// TODO: SCSI error code?
		//ret = ERRCODE(_sense.u);
		ret = -EIO;
	}
	return ret;
}

}

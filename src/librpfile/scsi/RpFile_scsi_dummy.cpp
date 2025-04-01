/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_scsi_dummy.cpp: Dummy implementation.                            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

namespace LibRpFile {

/**
 * Re-read device size using the native OS API.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeOS(off64_t *pDeviceSize, uint32_t *pSectorSize)
{
	// Not supported on this OS.
	RP_UNUSED(pDeviceSize);
	RP_UNUSED(pSectorSize);
	return -ENOSYS;
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
	// Not supported on this OS.
	RP_UNUSED(cdb);
	RP_UNUSED(cdb_len);
	RP_UNUSED(data);
	RP_UNUSED(data_len);
	RP_UNUSED(direction);
	return -ENOSYS;
}

}

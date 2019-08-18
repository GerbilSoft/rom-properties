/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_scsi_dummy.cpp: Dummy implementation.                            *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "../RpFile.hpp"

// C includes. (C++ namespace)
#include <cerrno>

namespace LibRpBase {

/**
 * Re-read device size using the native OS API.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeOS(int64_t *pDeviceSize, uint32_t *pSectorSize)
{
	// Not supported on this OS.
	RP_UNUSED(pDeviceSize);
	RP_UNUSED(pSectorSize);
	return -ENOSYS;
}

}

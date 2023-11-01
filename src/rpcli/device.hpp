/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * device.hpp: Extra functions for devices.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/config.librpfile.h"

// NOTE: We can't check in CMake beacuse RP_OS_SCSI_SUPPORTED
// is checked in librpbase, not rpcli.
#ifdef RP_OS_SCSI_SUPPORTED

#include <ostream>
namespace LibRpFile {
	class RpFile;
}

class ScsiInquiry
{
	LibRpFile::RpFile *const file;
public:
	explicit ScsiInquiry(LibRpFile::RpFile *file);
	friend std::ostream& operator<<(std::ostream& os, const ScsiInquiry& si);
};

class AtaIdentifyDevice
{
	LibRpFile::RpFile *const file;
	const bool packet;
public:
	explicit AtaIdentifyDevice(LibRpFile::RpFile *file, bool packet = false);
	friend std::ostream& operator<<(std::ostream& os, const AtaIdentifyDevice& si);
};

#endif /* RP_OS_SCSI_SUPPORTED */

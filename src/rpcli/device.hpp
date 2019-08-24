/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * device.hpp: Extra functions for devices.                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RPCLI_DEVICE_HPP__
#define __ROMPROPERTIES_RPCLI_DEVICE_HPP__

#include <ostream>
namespace LibRpBase {
	class RpFile;
}

class ScsiInquiry {
	LibRpBase::RpFile *const file;
public:
	explicit ScsiInquiry(LibRpBase::RpFile *file);
	friend std::ostream& operator<<(std::ostream& os, const ScsiInquiry& si);
};

class AtaIdentifyDevice {
	LibRpBase::RpFile *const file;
public:
	explicit AtaIdentifyDevice(LibRpBase::RpFile *file);
	friend std::ostream& operator<<(std::ostream& os, const AtaIdentifyDevice& si);
};

#endif /* __ROMPROPERTIES_RPCLI_DEVICE_HPP__ */

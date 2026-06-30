/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ParamSFO.hpp: PlayStation PARAM.SFO / PSF reader                        *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * Copyright (c) 2026 by Emma / InvoxiPlayGames.                           *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(ParamSFO)

public:
	enum class SFOValueType {
		None,
		UTF8,
		Int32
	};
	SFOValueType getKeyValueType(std::string key);

	const std::string getStringValue(std::string key);
	uint32_t getIntValue(std::string key);

ROMDATA_DECL_END()

} //namespace LibRomData

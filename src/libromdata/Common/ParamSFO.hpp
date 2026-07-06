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
ROMDATA_DECL_METADATA()

public:
	enum class SFOValueType {
		None,
		UTF8,
		Int32
	};
	SFOValueType getKeyValueType(const char *key) const;
	inline SFOValueType getKeyValueType(const std::string &key) const
	{
		return getKeyValueType(key.c_str());
	}

	std::string getStringValue(const char *key);
	inline std::string getStringValue(const std::string &key)
	{
		return getStringValue(key.c_str());
	}

	uint32_t getIntValue(const char *key);
	inline uint32_t getIntValue(const std::string &key)
	{
		return getIntValue(key.c_str());
	}

ROMDATA_DECL_END()

typedef std::shared_ptr<ParamSFO> ParamSFOPtr;

} //namespace LibRomData

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SegaPublishers.cpp: Sega third-party publishers list.                   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SegaPublishers.hpp"

namespace LibRomData { namespace SegaPublishers {

#include "SegaPublishers_data.h"

/** Public functions **/

/**
 * Look up a company code.
 * @param code Company code.
 * @return Publisher, or nullptr if not found.
 */
const char *lookup(unsigned int code)
{
	if (code > ARRAY_SIZE(SegaTCode_offtbl)) {
		return nullptr;
	}

	const unsigned int offset = SegaTCode_offtbl[code];
	return (likely(offset != 0) ? &SegaTCode_strtbl[offset] : nullptr);
}

} }

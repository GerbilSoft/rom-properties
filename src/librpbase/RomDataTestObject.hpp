/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RomDataTestObject.hpp: RomData test object for unit tests.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRpBase {

ROMDATA_DECL_BEGIN_EXPORT(RomDataTestObject)

public:
	/**
	 * Get a writable RomFields object for unit test purposes.
	 * @return Writable RomFields object
	 */
	RomFields *getWritableFields(void);

ROMDATA_DECL_END()

typedef std::shared_ptr<RomDataTestObject> RomDataTestObjectPtr;

} // namespace LibRpBase

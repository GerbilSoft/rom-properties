/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwanPublishers.hpp: Bandai WonderSwan third-party publishers list.*
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace WonderSwanPublishers {

/**
 * Look up a company name.
 * @param id Company ID.
 * @return Publisher name, or nullptr if not found.
 */
const char *lookup_name(uint8_t id);

/**
 * Look up a company code.
 * @param id Company ID.
 * @return Publisher code, or nullptr if not found.
 */
const char *lookup_code(uint8_t id);

} }

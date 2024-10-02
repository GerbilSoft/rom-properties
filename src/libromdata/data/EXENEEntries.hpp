/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXENEEntries.hpp: EXE NE Entry ordinal data                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2022 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace EXENEEntries {

/**
 * Look up an ordinal.
 * @param modname The module name
 * @param ordinal The ordinal
 * @return Name for the ordinal, or nullptr if not found.
 */
const char *lookup_ordinal(const char *modname, uint16_t ordinal);

} }

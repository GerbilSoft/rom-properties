/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * EXENEEntries.hpp: EXE NE Entry ordinal data                             *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * Copyright (c) 2022 by Egor.                                             *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DATA_EXENEENTRIES_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DATA_EXENEENTRIES_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

// C++ includes.
#include <string>

namespace LibRomData { namespace EXENEEntries {

/**
 * Look up an ordinal.
 * @param modname The module name
 * @param ordinal The ordinal
 * @return Name for the ordinal, or nullptr if not found.
 */
const char *lookup_ordinal(const std::string &modname, uint16_t ordinal);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_EXENEENTRIES_HPP__ */

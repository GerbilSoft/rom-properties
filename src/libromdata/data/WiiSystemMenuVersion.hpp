/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.hpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__

#include "common.h"

namespace LibRomData { namespace WiiSystemMenuVersion {

/**
 * Look up a Wii System Menu version.
 * @param version Version number.
 * @return Display version, or nullptr if not found.
 */
const char8_t *lookup(unsigned int version);

} }

#endif /* __ROMPROPERTIES_LIBROMDATA_WIISYSTEMMENUVERSION_HPP__ */

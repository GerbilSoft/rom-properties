/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.hpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace LibRomData { namespace WiiSystemMenuVersion {

/**
 * Look up a Wii System Menu version.
 * @param version Version number.
 * @return Display version, or nullptr if not found.
 */
const char *lookup(unsigned int version);

} }

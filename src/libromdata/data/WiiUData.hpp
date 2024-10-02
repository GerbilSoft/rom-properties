/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUData.hpp: Nintendo Wii U data.                                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// GameTDB uses ID6 for retail Wii U titles. The publisher ID
// is NOT stored in the plaintext .wud header, so it's not
// possible to use GameTDB unless we hard-code all of the
// publisher IDs here.

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace WiiUData {

/**
 * Look up a Wii U retail disc publisher.
 *
 * NOTE: Wii U uses 4-character publisher IDs.
 * If the first two characters are "00", then it's
 * equivalent to a 2-character publisher ID.
 *
 * @param Wii U retail disc ID4.
 * @return Packed publisher ID, or 0 if not found.
 */
uint32_t lookup_disc_publisher(const char *id4);

/**
 * Look up a Wii U application type.
 * @param app_type Application type ID
 * @return Application type string, or nullptr if unknown.
 */
const char *lookup_application_type(uint32_t app_type);

} }

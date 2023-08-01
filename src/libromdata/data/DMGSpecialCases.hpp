/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMGSpecialCases.hpp: Game Boy special cases for RPDB images.            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

extern "C" struct _DMG_RomHeader;

namespace LibRomData { namespace DMGSpecialCases {

/**
 * Get the publisher code as a string.
 * @param pbcode	[out] Output buffer (Must be at least 3 bytes)
 * @param romHeader	[in] ROM header
 * @return 0 on success; non-zero on error.
 */
int get_publisher_code(char pbcode[3], const struct _DMG_RomHeader *romHeader);

/**
 * Check if a DMG ROM image needs a checksum appended to its
 * filename for proper RPDB image lookup.
 *
 * Title-based version. This is used for games that don't have
 * a valid Game ID.
 *
 * @param romHeader DMG_RomHeader
 * @return True if a checksum is needed; false if not.
 */
bool is_rpdb_checksum_needed_TitleBased(const struct _DMG_RomHeader *romHeader);

/**
 * Check if a DMG ROM image needs a checksum appended to its
 * filename for proper RPDB image lookup.
 *
 * Game ID version. This is used for CGB games with a valid ID6.
 *
 * @param id6 ID6
 * @return True if a checksum is needed; false if not.
 */
bool is_rpdb_checksum_needed_ID6(const char *id6);

} }

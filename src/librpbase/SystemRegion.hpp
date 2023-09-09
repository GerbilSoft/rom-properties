/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.hpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
#include <stdint.h>

// C++ includes
#include <string>

namespace LibRpBase { namespace SystemRegion {

/**
 * Get the system country code. (ISO-3166)
 * This will always be an uppercase ASCII string.
 *
 * NOTE: Some newer country codes may use 3-character abbreviations.
 * The abbreviation will always be aligned towards the LSB, e.g.
 * 'US' will be 0x00005553.
 *
 * @return ISO-3166 country code as a uint32_t, or 0 on error.
 */
RP_LIBROMDATA_PUBLIC
uint32_t getCountryCode(void);

/**
 * Get the system language code. (ISO-639)
 * This will always be a lowercase ASCII string.
 *
 * NOTE: Some newer country codes may use 3-character abbreviations.
 * The abbreviation will always be aligned towards the LSB, e.g.
 * 'en' will be 0x0000656E.
 *
 * @return ISO-639 language code as a uint32_t, or 0 on error.
 */
RP_LIBROMDATA_PUBLIC
uint32_t getLanguageCode(void);

/**
 * Get a localized name for a language code.
 * Localized means in that language's language,
 * e.g. 'es' -> "Español".
 * @param lc Language code.
 * @return Localized name, or nullptr if not found.
 */
RP_LIBROMDATA_PUBLIC
const char *getLocalizedLanguageName(uint32_t lc);

// Flag sprite sheet columns/rows.
static const unsigned int FLAGS_SPRITE_SHEET_COLS = 4;
static const unsigned int FLAGS_SPRITE_SHEET_ROWS = 4;

/**
 * Get the position of a language code's flag icon in the flags sprite sheet.
 * @param lc		[in] Language code.
 * @param pCol		[out] Pointer to store the column value. (-1 if not found)
 * @param pRow		[out] Pointer to store the row value. (-1 if not found)
 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS(write_only, 2)
ATTR_ACCESS(write_only, 3)
RP_LIBROMDATA_PUBLIC
int getFlagPosition(uint32_t lc, int *pCol, int *pRow, bool forcePAL = false);

/**
 * Convert a language code to a string.
 * NOTE: The language code will be converted to lowercase if necessary.
 * @param lc Language code.
 * @return String.
 */
RP_LIBROMDATA_PUBLIC
std::string lcToString(uint32_t lc);

/**
 * Convert a language code to a string.
 * NOTE: The language code will be converted to uppercase.
 * @param lc Language code.
 * @return String.
 */
std::string lcToStringUpper(uint32_t lc);

#ifdef _WIN32
/**
 * Convert a language code to a wide string.
 * NOTE: The language code will be converted to lowercase if necessary.
 * @param lc Language code.
 * @return Wide string.
 */
RP_LIBROMDATA_PUBLIC
std::wstring lcToWString(uint32_t lc);

/**
 * Convert a language code to a wide string.
 * NOTE: The language code will be converted to uppercase.
 * @param lc Language code.
 * @return Wide string.
 */
std::wstring lcToWStringUpper(uint32_t lc);

#  ifdef _UNICODE
#    define lcToTString(lc)		lcToWString(lc)
#    define lcToTStringUpper(lc)	lcToWStringUpper(lc)
#  else /* !_UNICODE */
#    define lcToTString(lc)		lcToString(lc)
#    define lcToTStringUpper(lc)	lcToStringUpper(lc)
#  endif /* _UNICODE */

#endif /* _WIN32 */

} }

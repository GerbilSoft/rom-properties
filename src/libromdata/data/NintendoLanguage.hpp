/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoLanguage.hpp: Get the system language for Nintendo systems.     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace NintendoLanguage {

/**
 * Determine the system language for PAL GameCube.
 * @return GCN_PAL_Language_ID. (If unknown, defaults to GCN_PAL_LANG_ENGLISH.)
 */
int getGcnPalLanguage(void);

/**
 * Convert a GameCube PAL language ID to a language code.
 * @param langID GameCube PAL language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getGcnPalLanguageCode(int langID);

/**
 * Determine the system language for Wii.
 * @return Wii_Language_ID. (If unknown, defaults to WII_LANG_ENGLISH.)
 */
int getWiiLanguage(void);

/**
 * Convert a Wii language ID to a language code.
 * @param langID GameCube PAL language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getWiiLanguageCode(int langID);

/**
 * Determine the system language for Nintendo DS.
 * @param version NDS_IconTitleData version.
 * @return NDS_Language_ID. If unknown, defaults to NDS_LANG_ENGLISH.
 */
int getNDSLanguage(uint16_t version);

/**
 * Determine the system language for Nintendo 3DS.
 * TODO: Verify against the game's region code?
 * @return N3DS_Language_ID. If unknown, defaults to N3DS_LANG_ENGLISH.
 */
int getN3DSLanguage(void);

/**
 * Convert a Nintendo DS/3DS language ID to a language code.
 * @param langID Nintendo DS/3DS language ID.
 * @param maxID Maximum language ID, inclusive. (es, hans, ko, or hant)
 * @return Language code, or 0 on error.
 */
uint32_t getNDSLanguageCode(int langID, int maxID = 9001);

} }

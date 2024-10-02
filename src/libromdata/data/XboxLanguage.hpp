/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxLanguage.hpp: Get the system language for Microsoft Xbox systems.   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

namespace LibRomData { namespace XboxLanguage {

/**
 * Determine the system language for Xbox 360.
 * @return XDBF_Language_e. (If unknown, returns XDBF_LANGUAGE_UNKNOWN.)
 */
int getXbox360Language(void);

/**
 * Convert an Xbox 360 language ID to a language code.
 * @param langID Xbox 360 language ID.
 * @return Language code, or 0 on error.
 */
uint32_t getXbox360LanguageCode(int langID);

} }

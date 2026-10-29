/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * utf8_funcs.cpp: UTF-8 functions                                         *
 *                                                                         *
 * Copyright (c) 2022-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstddef>

// C++ includes
#include <string>
#include "rp-string_view.hpp"

namespace LibRpText {

/**
 * Determine the display length of a UTF-8 string.
 * This is used for monospaced console/text output only.
 * NOTE: Assuming the string is valid UTF-8.
 * @param str UTF-8 string (to specify a size limit, use a string_view substr)
 * @return Display length
 */
size_t utf8_disp_strlen(std::string_view sv);

/**
 * Encode a Unicode code point as UTF-8.
 * @param chr Code point
 * @return UTF-8 encoded code point
 */
std::string utf8_encode_code_point(char32_t chr);

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSFTagParser.hpp: PSF-style tag parser.                                 *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

// C++ STL classes
#include <string>
#include <unordered_map>

namespace LibRomData { namespace PSFTagParser {

/**
 * PSF tag style
 */
enum class PSFTagStyle {
	PSF,	// Standard PSF tags
};

/**
 * Parse a PSF tag section.
 * @param pData Tag section
 * @param size Size of tag section
 * @param style Style of PSF tags
 */
std::unordered_map<std::string, std::string> parseTags(const char *pData, size_t size, PSFTagStyle style = PSFTagStyle::PSF);

} }

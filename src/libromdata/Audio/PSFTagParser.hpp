/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSFTagParser.hpp: PSF-style tag parser.                                 *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/RomFields.hpp"

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
	S98,	// S98 tags: UTF-8 is indicated by a UTF-8 BOM after the magic number
};

/**
 * Parse a PSF tag section.
 * @param pData Tag section
 * @param size Size of tag section
 * @param style Style of PSF tags
 */
std::unordered_map<std::string, std::string> parseTags(const char *pData, size_t size, PSFTagStyle style = PSFTagStyle::PSF);

/**
 * Add PSF tags to RomFields.
 * @param fields RomFields
 * @param tags PSF tags [parsed using parseTags()]
 * @param psfby Key for "psfby" field
 * @return Number of fields added
 */
int addTagsToRomFields(LibRpBase::RomFields *fields, const std::unordered_map<std::string, std::string> &tags, const char *psfby = "psfby");

} }

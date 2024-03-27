/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.cpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiSystemMenuVersion.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace WiiSystemMenuVersion {

struct SysVersionEntry_t {
	uint16_t version;
	char str[6];
};

/**
 * Nintendo Wii System Menu version list.
 * References:
 * - https://wiibrew.org/wiki/System_Menu
 * - https://wiiubrew.org/wiki/Title_database
 * - https://yls8.mtheall.com/ninupdates/reports.php
 */
static const array<SysVersionEntry_t, 48> sysVersionList = {{
	// Wii
	// Reference: https://wiibrew.org/wiki/System_Menu
	{ 33, "1.0U"}, { 34, "1.0E"}, { 64, "1.0J"},
	{ 97, "2.0U"}, {128, "2.0J"}, {130, "2.0E"},
	{162, "2.1E"},
	{192, "2.2J"}, {193, "2.2U"}, {194, "2.2E"},
	{224, "3.0J"}, {225, "3.0U"}, {226, "3.0E"},
	{256, "3.1J"}, {257, "3.1U"}, {258, "3.1E"},
	{288, "3.2J"}, {289, "3.2U"}, {290, "3.2E"},
	{326, "3.3K"}, {352, "3.3J"}, {353, "3.3U"}, {354, "3.3E"},
	{384, "3.4J"}, {385, "3.4U"}, {386, "3.4E"},
	{390, "3.5K"},
	{416, "4.0J"}, {417, "4.0U"}, {418, "4.0E"},
	{448, "4.1J"}, {449, "4.1U"}, {450, "4.1E"}, {454, "4.1K"},
	{480, "4.2J"}, {481, "4.2U"}, {482, "4.2E"}, {486, "4.2K"},
	{512, "4.3J"}, {513, "4.3U"}, {514, "4.3E"}, {518, "4.3K"},

	// vWii
	// References:
	// - https://wiiubrew.org/wiki/Title_database
	// - https://yls8.mtheall.com/ninupdates/reports.php
	// NOTE: These are all listed as 4.3.
	// NOTE 2: vWii also has 512, 513, and 514.
	{544, "4.3J"}, {545, "4.3U"}, {546, "4.3E"},
	{608, "4.3J"}, {609, "4.3U"}, {610, "4.3E"},
}};

/** Public functions **/

/**
 * Look up a Wii System Menu version.
 * @param version Version number.
 * @return Display version, or nullptr if not found.
 */
const char *lookup(unsigned int version)
{
	// Do a binary search.
	auto pVer = std::lower_bound(sysVersionList.cbegin(), sysVersionList.cend(), version,
		[](const SysVersionEntry_t &sysVersion, unsigned int version) noexcept -> bool {{
			return (sysVersion.version < version);
		}});
	if (pVer == sysVersionList.cend() || pVer->version != version) {
		return nullptr;
	}
	return pVer->str;
}

} }

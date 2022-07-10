/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.cpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiSystemMenuVersion.hpp"

namespace LibRomData { namespace WiiSystemMenuVersion {

struct SysVersionEntry_t {
	uint16_t version;
	char8_t str[6];
};

/**
 * Nintendo Wii System Menu version list.
 * References:
 * - https://wiibrew.org/wiki/System_Menu
 * - https://wiiubrew.org/wiki/Title_database
 * - https://yls8.mtheall.com/ninupdates/reports.php
 */
static const SysVersionEntry_t sysVersionList[] = {
	// Wii
	// Reference: https://wiibrew.org/wiki/System_Menu
	{ 33, U8("1.0")},
	{ 97, U8("2.0U")}, {128, U8("2.0J")}, {130, U8("2.0E")},
	{162, U8("2.1E")},
	{192, U8("2.2J")}, {193, U8("2.2U")}, {194, U8("2.2E")},
	{224, U8("3.0J")}, {225, U8("3.0U")}, {226, U8("3.0E")},
	{256, U8("3.1J")}, {257, U8("3.1U")}, {258, U8("3.1E")},
	{288, U8("3.2J")}, {289, U8("3.2U")}, {290, U8("3.2E")},
	{326, U8("3.3K")}, {352, U8("3.3J")}, {353, U8("3.3U")}, {354, U8("3.3E")},
	{384, U8("3.4J")}, {385, U8("3.4U")}, {386, U8("3.4E")},
	{390, U8("3.5K")},
	{416, U8("4.0J")}, {417, U8("4.0U")}, {418, U8("4.0E")},
	{448, U8("4.1J")}, {449, U8("4.1U")}, {450, U8("4.1E")}, {454, U8("4.1K")},
	{480, U8("4.2J")}, {481, U8("4.2U")}, {482, U8("4.2E")}, {483, U8("4.2K")},
	{512, U8("4.3J")}, {513, U8("4.3U")}, {514, U8("4.3E")}, {518, U8("4.3K")},

	// vWii
	// References:
	// - https://wiiubrew.org/wiki/Title_database
	// - https://yls8.mtheall.com/ninupdates/reports.php
	// NOTE: These are all listed as 4.3.
	// NOTE 2: vWii also has 512, 513, and 514.
	{544, U8("4.3J")}, {545, U8("4.3U")}, {546, U8("4.3E")},
	{608, U8("4.3J")}, {609, U8("4.3U")}, {610, U8("4.3E")},

	// End of list.
	{0, U8("")}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
static int RP_C_API compar(const void *a, const void *b)
{
	unsigned int v1 = static_cast<const SysVersionEntry_t*>(a)->version;
	unsigned int v2 = static_cast<const SysVersionEntry_t*>(b)->version;
	if (v1 < v2) return -1;
	if (v1 > v2) return 1;
	return 0;
}

/** Public functions **/

/**
 * Look up a Wii System Menu version.
 * @param version Version number.
 * @return Display version, or nullptr if not found.
 */
const char8_t *lookup(unsigned int version)
{
	// Do a binary search.
	const SysVersionEntry_t key = {static_cast<uint16_t>(version), U8("")};
	const SysVersionEntry_t *res =
		static_cast<const SysVersionEntry_t*>(bsearch(&key,
			sysVersionList,
			ARRAY_SIZE(sysVersionList)-1,
			sizeof(SysVersionEntry_t),
			compar));
	return (res) ? res->str : nullptr;
}

} }

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiSystemMenuVersion.cpp: Nintendo Wii System Menu version list.        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "WiiSystemMenuVersion.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class WiiSystemMenuVersionPrivate {
	private:
		// Static class.
		WiiSystemMenuVersionPrivate();
		~WiiSystemMenuVersionPrivate();
		RP_DISABLE_COPY(WiiSystemMenuVersionPrivate)

	public:
		/**
		 * Nintendo Wii System Menu version list.
		 * References:
		 * - http://wiibrew.org/wiki/System_Menu
		 * - http://wiiubrew.org/wiki/Title_database
		 * - https://yls8.mtheall.com/ninupdates/reports.php
		 */
		struct SysVersionEntry_t {
			unsigned int version;
			const char *str;
		};
		static const SysVersionEntry_t sysVersionList[];

	public:
		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);
};

/** WiiSystemMenuVersionPrivate **/

/**
 * Nintendo Wii System Menu version list.
 * References:
 * - http://wiibrew.org/wiki/System_Menu
 * - http://wiiubrew.org/wiki/Title_database
 * - https://yls8.mtheall.com/ninupdates/reports.php
 */
const WiiSystemMenuVersionPrivate::SysVersionEntry_t WiiSystemMenuVersionPrivate::sysVersionList[] = {
	// Wii
	// Reference: http://wiibrew.org/wiki/System_Menu
	{ 33, "1.0"},
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
	{480, "4.2J"}, {481, "4.2U"}, {482, "4.2E"}, {483, "4.2K"},
	{512, "4.3J"}, {513, "4.3U"}, {514, "4.3E"}, {518, "4.3K"},

	// vWii
	// References:
	// - http://wiiubrew.org/wiki/Title_database
	// - https://yls8.mtheall.com/ninupdates/reports.php
	// NOTE: These are all listed as 4.3.
	// NOTE 2: vWii also has 512, 513, and 514.
	{544, "4.3J"}, {545, "4.3U"}, {546, "4.3E"},
	{608, "4.3J"}, {609, "4.3U"}, {610, "4.3E"},

	// End of list.
	{0, nullptr}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API WiiSystemMenuVersionPrivate::compar(const void *a, const void *b)
{
	unsigned int v1 = static_cast<const SysVersionEntry_t*>(a)->version;
	unsigned int v2 = static_cast<const SysVersionEntry_t*>(b)->version;
	if (v1 < v2) return -1;
	if (v1 > v2) return 1;
	return 0;
}

/** WiiSystemMenuVersion **/

/**
 * Look up a Wii System Menu version.
 * @param version Version number.
 * @return Display version, or nullptr if not found.
 */
const char *WiiSystemMenuVersion::lookup(unsigned int version)
{
	// Do a binary search.
	const WiiSystemMenuVersionPrivate::SysVersionEntry_t key = {version, nullptr};
	const WiiSystemMenuVersionPrivate::SysVersionEntry_t *res =
		static_cast<const WiiSystemMenuVersionPrivate::SysVersionEntry_t*>(bsearch(&key,
			WiiSystemMenuVersionPrivate::sysVersionList,
			ARRAY_SIZE(WiiSystemMenuVersionPrivate::sysVersionList)-1,
			sizeof(WiiSystemMenuVersionPrivate::SysVersionEntry_t),
			WiiSystemMenuVersionPrivate::compar));
	return (res ? res->str : nullptr);
}

}

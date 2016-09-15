/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SystemRegion.cpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "SystemRegion.hpp"
#include "common.h"

#ifdef _WIN32
#include "RpWin32.hpp"
#else
#include <cctype>
#include <clocale>
#include <cstring>
#endif

namespace LibRomData {

/**
 * Get the system country code. (two-character ISO-3166)
 * @return Two-character ISO-3166 country code as a uint16_t, or 0 on error.
 */
uint16_t SystemRegion::getCountryCode(void)
{
	static uint16_t cc = 0;
	static bool cc_retrieved = false;
	if (cc_retrieved) {
		// Country code has already been obtained.
		return cc;
	}

#ifdef _WIN32
	// References:
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/dd318101(v=vs.85).aspx
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/dd318101(v=vs.85).aspx
	// NOTE: LOCALE_SISO3166CTRYNAME might not work on some old versions
	// of Windows, but our minimum is Windows XP.
	wchar_t locale[16];
	int ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, locale, ARRAY_SIZE(locale));
	if (ret == 3) {
		// Retrieved the two-character country code.
		// (ret == 3 due to the NULL terminator.)
		cc = ((locale[0] << 8) | locale[1]);
	}
#else
	// TODO: Check the C++ locale if this fails?
	const char *locale = setlocale(LC_ALL, nullptr);
	if (locale) {
		// Look for an underscore. ('_')
		const char *underscore = strchr(locale, '_');
		if (underscore) {
			// Found an underscore.
			// Check if we have a two-character country code.
			// TODO: Handle 3-character country codes?
			if (isalpha(underscore[1]) && isalpha(underscore[2]) &&
			    !isalpha(underscore[3]))
			{
				// Found a two-character country code.
				cc = ((underscore[1] << 8) | underscore[2]);
			}
		}
	}
#endif

	// Country code retrieved.
	cc_retrieved = true;
	return cc;
}

}

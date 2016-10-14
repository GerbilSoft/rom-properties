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
 * Get the system country code. (ISO-3166)
 * This will always be an uppercase ASCII value.
 *
 * NOTE: Some newer country codes may use 3-character abbreviations.
 * The abbreviation will always be aligned towards the LSB, e.g.
 * 'US' will be 0x00005553.
 *
 * @return ISO-3166 country code as a uint32_t, or 0 on error.
 */
uint32_t SystemRegion::getCountryCode(void)
{
	static uint32_t cc = 0;
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
	switch (ret) {
		case 3:
			// 2-character country code.
			// (ret == 3 due to the NULL terminator.)
			cc = ((locale[0] << 8) | locale[1]);
			break;
		case 4:
			// 3-character country code.
			// (ret == 3 due to the NULL terminator.)
			cc = ((locale[0] << 16) | (locale[1] << 8) | locale[2]);
			break;

		default:
			// Unsupported. (MSDN says the string could be up to 9 characters!)
			break;
	}
#else
	// TODO: Check the C++ locale if this fails?
	const char *locale = setlocale(LC_ALL, nullptr);
	if (locale) {
		// Look for an underscore. ('_')
		const char *underscore = strchr(locale, '_');
		if (underscore) {
			// Found an underscore.
			if (isalpha(underscore[1]) && isalpha(underscore[2])) {
				if (!isalpha(underscore[3])) {
					// 2-character country code.
					cc = ((underscore[1] << 8) | underscore[2]);
				} else if (isalpha(underscore[3]) && !isalpha(underscore[4])) {
					// 3-character country code.
					cc = ((underscore[1] << 16) | (underscore[2] << 8) | underscore[3]);
				}
			}
		}
	}
#endif

	// Make sure the country code is uppercase.
	// FIXME: May contain numbers...
	if (cc != 0) {
		cc &= ~0x2020;
	}

	// Country code retrieved.
	cc_retrieved = true;
	return cc;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.cpp: Get the system country code.                          *
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

#include "SystemRegion.hpp"
#include "librpbase/common.h"

// One-time initialization.
#include "threads/pthread_once.h"

#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#else
# include <cctype>
# include <clocale>
# include <cstring>
#endif

namespace LibRpBase {

class SystemRegionPrivate
{
	private:
		// SystemRegion is a static class.
		SystemRegionPrivate();
		~SystemRegionPrivate();
		RP_DISABLE_COPY(SystemRegionPrivate)

	public:
		// Country and language codes.
		static uint32_t cc;
		static uint32_t lc;

		/** TODO: Combine the initialization functions so they retrieve **
		 ** both country code and language code at the same time.       **/

		// One-time initialization variable and functions.
		static pthread_once_t once_control;

		/**
		 * Get the system region information.
		 * Called by pthread_once().
		 * Country code will be stored in 'cc'.
		 * Language code will be stored in 'lc'.
		 */
		static void getSystemRegion(void);
};

// Country and language codes.
uint32_t SystemRegionPrivate::cc = 0;
uint32_t SystemRegionPrivate::lc = 0;

// pthread_once() control variable.
pthread_once_t SystemRegionPrivate::once_control = PTHREAD_ONCE_INIT;

#ifdef _WIN32
/**
 * Get the system region information.
 * Called by pthread_once().
 * (Windows version)
 *
 * Country code will be stored in 'cc'.
 * Language code will be stored in 'lc'.
 */
void SystemRegionPrivate::getSystemRegion(void)
{
	// References:
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/dd318101(v=vs.85).aspx
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/dd318101(v=vs.85).aspx

	// NOTE: LOCALE_SISO3166CTRYNAME might not work on some old versions
	// of Windows, but our minimum is Windows XP.
	// FIXME: Non-ASCII locale names will break!
	wchar_t locale[16];
	int ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, locale, ARRAY_SIZE(locale));
	switch (ret) {
		case 3:
			// 2-character country code.
			// (ret == 3 due to the NULL terminator.)
			cc = (((towupper(locale[0]) & 0xFF) << 8) |
			       (towupper(locale[1]) & 0xFF));
			break;
		case 4:
			// 3-character country code.
			// (ret == 4 due to the NULL terminator.)
			cc = (((towupper(locale[0]) & 0xFF) << 16) |
			      ((towupper(locale[1]) & 0xFF) << 8) |
			       (towupper(locale[2]) & 0xFF));
			break;

		default:
			// Unsupported. (MSDN says the string could be up to 9 characters!)
			cc = 0;
			break;
	}

	// NOTE: LOCALE_SISO639LANGNAME might not work on some old versions
	// of Windows, but our minimum is Windows XP.
	// FIXME: Non-ASCII locale names will break!
	ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, locale, ARRAY_SIZE(locale));
	switch (ret) {
		case 3:
			// 2-character language code.
			// (ret == 3 due to the NULL terminator.)
			lc = (((towlower(locale[0]) & 0xFF) << 8) |
			       (towlower(locale[1]) & 0xFF));
			break;
		case 4:
			// 3-character language code.
			// (ret == 4 due to the NULL terminator.)
			lc = (((towlower(locale[0]) & 0xFF) << 16) |
			      ((towlower(locale[1]) & 0xFF) << 8) |
			       (towlower(locale[2]) & 0xFF));
			break;

		default:
			// Unsupported. (MSDN says the string could be up to 9 characters!)
			lc = 0;
			break;
	}
}

#else /* !_WIN32 */

/**
 * Get the system region information.
 * Called by pthread_once().
 * (Unix/Linux version)
 *
 * Country code will be stored in 'cc'.
 * Language code will be stored in 'lc'.
 */
void SystemRegionPrivate::getSystemRegion(void)
{
	// TODO: Check the C++ locale if this fails?
	const char *const locale = setlocale(LC_ALL, nullptr);
	if (!locale) {
		// Unable to get the locale.
		cc = 0;
		lc = 0;
		return;
	}

	// Language code: Read up to the first non-alphabetic character.
	if (isalpha(locale[0]) && isalpha(locale[1])) {
		if (!isalpha(locale[2])) {
			// 2-character language code.
			lc = ((tolower(locale[0]) << 8) |
			       tolower(locale[1]));
		} else if (isalpha(locale[2]) && !isalpha(locale[3])) {
			// 3-character language code.
			lc = ((tolower(locale[0]) << 16) |
			      (tolower(locale[1]) << 8) |
			       tolower(locale[2]));
		} else {
			// Invalid language code.
			lc = 0;
		}
	} else {
		// Invalid language code.
		lc = 0;
	}

	// Look for an underscore. ('_')
	const char *const underscore = strchr(locale, '_');
	if (!underscore) {
		// No country code...
		cc = 0;
		return;
	}

	// Found an underscore.
	if (isalpha(underscore[1]) && isalpha(underscore[2])) {
		if (!isalpha(underscore[3])) {
			// 2-character country code.
			cc = ((toupper(underscore[1]) << 8) |
			       toupper(underscore[2]));
		} else if (isalpha(underscore[3]) && !isalpha(underscore[4])) {
			// 3-character country code.
			cc = ((toupper(underscore[1]) << 16) |
			      (toupper(underscore[2]) << 8) |
			       toupper(underscore[3]));
		} else {
			// Invalid country code.
			cc = 0;
		}
	} else {
		// Invalid country code.
		cc = 0;
	}
}
#endif /* _WIN32 */

/**
 * Get the system country code. (ISO-3166)
 * This will always be an uppercase ASCII string.
 *
 * NOTE: Some newer country codes may use 3-character abbreviations.
 * The abbreviation will always be aligned towards the LSB, e.g.
 * 'US' will be 0x00005553.
 *
 * @return ISO-3166 country code as a uint32_t, or 0 on error.
 */
uint32_t SystemRegion::getCountryCode(void)
{
	pthread_once(&SystemRegionPrivate::once_control,
		SystemRegionPrivate::getSystemRegion);
	return SystemRegionPrivate::cc;
}

/**
 * Get the system language code. (ISO-639)
 * This will always be a lowercase ASCII string.
 *
 * NOTE: Some newer country codes may use 3-character abbreviations.
 * The abbreviation will always be aligned towards the LSB, e.g.
 * 'en' will be 0x0000656E.
 *
 * @return ISO-639 language code as a uint32_t, or 0 on error.
 */
uint32_t SystemRegion::getLanguageCode(void)
{
	pthread_once(&SystemRegionPrivate::once_control,
		SystemRegionPrivate::getSystemRegion);
	return SystemRegionPrivate::lc;
}

}

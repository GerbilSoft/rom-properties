/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.cpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "SystemRegion.hpp"
#include "librpbase/common.h"

// librpthreads
#include "librpthreads/pthread_once.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <clocale>
#include <cstring>

#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
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
		 * Get the LC_MESSAGES or LC_ALL environment variable.
		 * @return Value, or nullptr if not found.
		 */
		static const char *get_LC_MESSAGES(void);

		/**
		 * Get the system region information from a Unix-style language code.
		 *
		 * @param locale LC_MESSAGES value.
		 * @return 0 on success; non-zero on error.
		 *
		 * Country code will be stored in 'cc'.
		 * Language code will be stored in 'lc'.
		 */
		static int getSystemRegion_LC_MESSAGES(const char *locale);

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

/**
 * Get the LC_MESSAGES or LC_ALL environment variable.
 * @return Value, or nullptr if not found.
 */
const char *SystemRegionPrivate::get_LC_MESSAGES(void)
{
	// TODO: Check the C++ locale if this fails?
	// TODO: On Windows startup in main() functions, get LC_ALL/LC_MESSAGES vars if msvc doesn't?

	// Environment variables override the system defaults.
	const char *locale = getenv("LC_MESSAGES");
	if (!locale || locale[0] == '\0') {
		locale = getenv("LC_ALL");
	}
#ifndef _WIN32
	// NOTE: MSVCRT doesn't support LC_MESSAGES.
	if (!locale || locale[0] == '\0') {
		locale = setlocale(LC_MESSAGES, nullptr);
	}
#endif /* _WIN32 */
	if (!locale || locale[0] == '\0') {
		locale = setlocale(LC_ALL, nullptr);
	}

	if (!locale || locale[0] == '\0') {
		// Unable to get the LC_MESSAGES or LC_ALL variables.
		return nullptr;
	}

	return locale;
}

/**
 * Get the system region information from a Unix-style language code.
 *
 * @param locale LC_MESSAGES value.
 * @return 0 on success; non-zero on error.
 *
 * Country code will be stored in 'cc'.
 * Language code will be stored in 'lc'.
 */
int SystemRegionPrivate::getSystemRegion_LC_MESSAGES(const char *locale)
{
	if (!locale || locale[0] == '\0') {
		// No locale...
		cc = 0;
		lc = 0;
		return -1;
	}

	int ret = -1;

	// Language code: Read up to the first non-alphabetic character.
	if (ISALPHA(locale[0]) && ISALPHA(locale[1])) {
		if (!ISALPHA(locale[2])) {
			// 2-character language code.
			lc = ((TOLOWER(locale[0]) << 8) |
			       TOLOWER(locale[1]));
		} else if (ISALPHA(locale[2]) && !ISALPHA(locale[3])) {
			// 3-character language code.
			lc = ((TOLOWER(locale[0]) << 16) |
			      (TOLOWER(locale[1]) << 8) |
			       TOLOWER(locale[2]));
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
		return ret;
	}

	// Found an underscore.
	if (ISALPHA(underscore[1]) && ISALPHA(underscore[2])) {
		if (!ISALPHA(underscore[3])) {
			// 2-character country code.
			cc = ((TOUPPER(underscore[1]) << 8) |
			       TOUPPER(underscore[2]));
			ret = 0;
		} else if (ISALPHA(underscore[3]) && !ISALPHA(underscore[4])) {
			// 3-character country code.
			cc = ((TOUPPER(underscore[1]) << 16) |
			      (TOUPPER(underscore[2]) << 8) |
			       TOUPPER(underscore[3]));
			ret = 0;
		} else {
			// Invalid country code.
			cc = 0;
		}
	} else {
		// Invalid country code.
		cc = 0;
	}

	return ret;
}

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
	TCHAR locale[16];
	int ret;

	// Check if LC_MESSAGES or LC_ALL is set.
	const char *const locale_var = get_LC_MESSAGES();
	if (locale_var != nullptr && locale_var[0] != '\0') {
		ret = getSystemRegion_LC_MESSAGES(locale_var);
		if (ret == 0) {
			// LC_MESSAGES or LC_ALL is set and is valid.
			return;
		}

		// LC_MESSAGES or LC_ALL is not set or is invalid.
		// Continue with the Windows-specific code.

		// NOTE: If LC_MESSAGE or LC_ALL had a language code
		// but not a region code, we'll keep that portion.
	}

	// References:
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/dd318101(v=vs.85).aspx

	// NOTE: LOCALE_SISO3166CTRYNAME might not work on some old versions
	// of Windows, but our minimum is Windows XP.
	// FIXME: Non-ASCII locale names will break!
	if (cc == 0) {
		ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, locale, ARRAY_SIZE(locale));
		switch (ret) {
			case 3:
				// 2-character country code.
				// (ret == 3 due to the NULL terminator.)
				cc = (((_totupper(locale[0]) & 0xFF) << 8) |
				       (_totupper(locale[1]) & 0xFF));
				break;
			case 4:
				// 3-character country code.
				// (ret == 4 due to the NULL terminator.)
				cc = (((_totupper(locale[0]) & 0xFF) << 16) |
				      ((_totupper(locale[1]) & 0xFF) << 8) |
				       (_totupper(locale[2]) & 0xFF));
				break;

			default:
				// Unsupported. (MSDN says the string could be up to 9 characters!)
				cc = 0;
				break;
		}
	}

	// NOTE: LOCALE_SISO639LANGNAME might not work on some old versions
	// of Windows, but our minimum is Windows XP.
	// FIXME: Non-ASCII locale names will break!
	if (lc == 0) {
		ret = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, locale, ARRAY_SIZE(locale));
		switch (ret) {
			case 3:
				// 2-character language code.
				// (ret == 3 due to the NULL terminator.)
				lc = (((_totlower(locale[0]) & 0xFF) << 8) |
				       (_totlower(locale[1]) & 0xFF));
				break;
			case 4:
				// 3-character language code.
				// (ret == 4 due to the NULL terminator.)
				lc = (((_totlower(locale[0]) & 0xFF) << 16) |
				      ((_totlower(locale[1]) & 0xFF) << 8) |
				       (_totlower(locale[2]) & 0xFF));
				break;

			default:
				// Unsupported. (MSDN says the string could be up to 9 characters!)
				lc = 0;
				break;
		}
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
inline void SystemRegionPrivate::getSystemRegion(void)
{
	getSystemRegion_LC_MESSAGES(get_LC_MESSAGES());
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

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.cpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SystemRegion.hpp"

// librpthreads
#include "librpthreads/pthread_once.h"

// C includes. (C++ namespace)
#include <clocale>

#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

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

		/** Language names **/

		struct LangName_t {
			uint32_t lc;
			const char *name;
		};

		// Language name mapping.
		static const LangName_t langNames[];

		/**
		 * LangName_t bsearch() comparison function.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API LangName_t_compar(const void *a, const void *b);
};

// Country and language codes.
uint32_t SystemRegionPrivate::cc = 0;
uint32_t SystemRegionPrivate::lc = 0;

// pthread_once() control variable.
pthread_once_t SystemRegionPrivate::once_control = PTHREAD_ONCE_INIT;

// Language name mapping.
// NOTE: This MUST be sorted by 'lc'!
// NOTE: Names MUST be in UTF-8!
// Reference: https://www.omniglot.com/language/names.htm
const SystemRegionPrivate::LangName_t SystemRegionPrivate::langNames[] = {
	{'de',	"Deutsch"},
	{'en',	"English"},
	{'es',	"Español"},
	{'fr',	"Français"},
	{'it',	"Italiano"},
	{'ja',	"日本語"},
	{'ko',	"한국어"},	// South Korea
	{'nl',	"Nederlands"},
	{'pl',	"Polski"},
	{'pt',	"Português"},
	{'ru',	"Русский"},
	{'hans', "简体中文"},
	{'hant', "繁體中文"},
};

/**
 * char_id_t bsearch() comparison function.
 * @param a
 * @param b
 * @return
 */
int RP_C_API SystemRegionPrivate::LangName_t_compar(const void *a, const void *b)
{
	uint32_t lc1 = static_cast<const LangName_t*>(a)->lc;
	uint32_t lc2 = static_cast<const LangName_t*>(b)->lc;
	if (lc1 < lc2) return -1;
	if (lc1 > lc2) return 1;
	return 0;
}

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

	// Look for an underscore or a hyphen. ('_', '-')
	const char *underscore = strchr(locale, '_');
	if (!underscore) {
		underscore = strchr(locale, '-');
		if (!underscore) {
			// No country code...
			cc = 0;
			return ret;
		}
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
		} else if (ISALPHA(underscore[4]) && !ISALPHA(underscore[5])) {
			// 4-character country code.
			cc = ((TOUPPER(underscore[1]) << 24) |
			      (TOUPPER(underscore[2]) << 16) |
			      (TOUPPER(underscore[3]) << 8) |
			       TOUPPER(underscore[4]));

			// Special handling for compatibility:
			// - 'HANS' -> 'CN' (and/or 'SG')
			// - 'HANT' -> 'TW' (and/or 'HK')
			if (cc == 'HANS') {
				cc = 'CN';
			} else if (cc == 'HANT') {
				cc = 'TW';
			}

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

/**
 * Get a localized name for a language code.
 * Localized means in that language's language,
 * e.g. 'es' -> "Español".
 * @param lc Language code.
 * @return Localized name, or nullptr if not found.
 */
const char *SystemRegion::getLocalizedLanguageName(uint32_t lc)
{
	// Do a binary search.
	const SystemRegionPrivate::LangName_t key = {lc, nullptr};
	const SystemRegionPrivate::LangName_t *res =
		static_cast<const SystemRegionPrivate::LangName_t*>(bsearch(&key,
			SystemRegionPrivate::langNames,
			ARRAY_SIZE(SystemRegionPrivate::langNames),
			sizeof(SystemRegionPrivate::LangName_t),
			SystemRegionPrivate::LangName_t_compar));
	return (res ? res->name : nullptr);
}

/**
 * Get the position of a language code's flag icon in the flags sprite sheet.
 * @param lc	[in] Language code.
 * @param pCol	[out] Pointer to store the column value. (-1 if not found)
 * @param pRow	[out] Pointer to store the row value. (-1 if not found)
 * @return 0 on success; negative POSIX error code on error.
 */
int SystemRegion::getFlagPosition(uint32_t lc, int *pCol, int *pRow)
{
	int ret = -ENOENT;

	// Flags are stored in a sprite sheet, so we need to
	// determine the column and row.
	struct flagpos_t {
		uint32_t lc;
		uint16_t col;
		uint16_t row;
		};
	static const flagpos_t flagpos[] = {
		{'hans',	0, 0},
		{'hant',	0, 0},
		{'de',		1, 0},
		{'es',		2, 0},
		{'fr',		3, 0},
		//{'gb',		0, 1},
		{'it',		1, 1},
		{'ja',		2, 1},
		{'ko',		3, 1},
		{'nl',		0, 2},
		{'pl',		0, 3},
		{'pt',		1, 2},
		{'ru',		2, 2},
		//{'us',		3, 0},

		{0, 0, 0}
	};

	if (lc == 'en') {
		// Special case for English:
		// Use the 'us' flag if the country code is US,
		// and the 'gb' flag for everywhere else.
		if (getCountryCode() == 'US') {
			*pCol = 3;
			*pRow = 2;
		} else {
			*pCol = 0;
			*pRow = 1;
		}
		ret = 0;
	} else {
		// Other flags. Check the table.
		*pCol = -1;
		*pRow = -1;
		for (const flagpos_t *p = flagpos; p->lc != 0; p++) {
			if (p->lc == lc) {
				// Match!
				*pCol = p->col;
				*pRow = p->row;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

}

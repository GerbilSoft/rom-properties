/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SystemRegion.cpp: Get the system country code.                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SystemRegion.hpp"
#include "ctypex.h"

// librpthreads
#include "librpthreads/pthread_once.h"

// C includes (C++ namespace)
#include <clocale>

// C++ STL classes
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include "tcharx.h"
#endif /* _WIN32 */

namespace LibRpBase { namespace SystemRegion {

/** TODO: Combine the initialization functions so they retrieve **
 ** both country code and language code at the same time.       **/

// Country and language codes.
static uint32_t cc = 0;
static uint32_t lc = 0;

// pthread_once() control variable.
static pthread_once_t system_region_once_control = PTHREAD_ONCE_INIT;

struct LanguageOffTbl_t {
	uint32_t lc;
	const uint32_t offset;
};

// Language name string table.
// NOTE: Strings are UTF-8.
// Reference: https://www.omniglot.com/language/names.htm
static const char languages_strtbl[] =
	"English (AU)\0"	// 'au' (GameTDB only)
	"Deutsch\0"		// 'de'
	"English\0"		// 'en'
	"Español\0"		// 'es'
	"Français\0"		// 'fr'
	"Italiano\0"		// 'it'
	"日本語\0"		// 'ja'
	"한국어\0"		// 'ko': South Korea
	"Nederlands\0"		// 'nl'
	"Polski\0"		// 'pl'
	"Português\0"		// 'pt'
	"Русский\0"		// 'ru'
	"简体中文\0"		// 'hans'
	"繁體中文\0";		// 'hant'

// Language name mapping.
// NOTE: This MUST be sorted by 'lc'!
static const LanguageOffTbl_t languages_offtbl[] = {
	{'au',	0}, // GameTDB only
	{'de',	13},
	{'en',	21},
	{'es',	29},
	{'fr',	38},
	{'it',	48},
	{'ja',	57},
	{'ko',	67},
	{'nl',	77},
	{'pl',	88},
	{'pt',	95},
	{'ru',	106},
	{'hans', 121},
	{'hant', 134},
};
static_assert(sizeof(languages_strtbl) == 0x93+1, "languages_strtbl[] size has changed!");

/**
 * Get the LC_MESSAGES or LC_ALL environment variable.
 * @return Value, or nullptr if not found.
 */
static const char *get_LC_MESSAGES(void)
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
static int getSystemRegion_LC_MESSAGES(const char *locale)
{
	if (!locale || locale[0] == '\0') {
		// No locale...
		lc = 0;
		cc = 0;
		return -1;
	}

	// Explicitly check for the "C" locale.
	if (!strcasecmp(locale, "C")) {
		// "C" locale. Use 0.
		lc = 0;
		cc = 0;
		return 0;
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
static void getSystemRegion(void)
{
	TCHAR locale[16];
	int ret;

	// Check if LC_MESSAGES or LC_ALL is set.
	const char *const locale_var = get_LC_MESSAGES();
	if (locale_var != nullptr && locale_var[0] != '\0') {
		// Check for an actual locale setting.
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
	// - https://docs.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getlocaleinfow

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
static inline void getSystemRegion(void)
{
	getSystemRegion_LC_MESSAGES(get_LC_MESSAGES());
}
#endif /* _WIN32 */

/** Public functions **/

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
uint32_t getCountryCode(void)
{
	pthread_once(&system_region_once_control, getSystemRegion);
	return cc;
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
uint32_t getLanguageCode(void)
{
	pthread_once(&system_region_once_control, getSystemRegion);
	return lc;
}

/**
 * Get a localized name for a language code.
 * Localized means in that language's language,
 * e.g. 'es' -> "Español".
 * @param lc Language code.
 * @return Localized name, or nullptr if not found.
 */
const char *getLocalizedLanguageName(uint32_t lc)
{
	// Do a binary search.
	static const LanguageOffTbl_t *const p_languages_offtbl_end =
		&languages_offtbl[ARRAY_SIZE(languages_offtbl)];
	auto pLangOffTbl = std::lower_bound(languages_offtbl, p_languages_offtbl_end, lc,
		[](const LanguageOffTbl_t &langOffTbl, uint32_t lc) noexcept -> bool {
			return (langOffTbl.lc < lc);
		});
	if (pLangOffTbl == p_languages_offtbl_end || pLangOffTbl->lc != lc) {
		return nullptr;
	}
	return &languages_strtbl[pLangOffTbl->offset];
}

/**
 * Get the position of a language code's flag icon in the flags sprite sheet.
 * @param lc		[in] Language code.
 * @param pCol		[out] Pointer to store the column value. (-1 if not found)
 * @param pRow		[out] Pointer to store the row value. (-1 if not found)
 * @param forcePAL	[in,opt] If true, force PAL regions, e.g. always use the 'gb' flag for English.
 * @return 0 on success; negative POSIX error code on error.
 */
int getFlagPosition(uint32_t lc, int *pCol, int *pRow, bool forcePAL)
{
	int ret = -ENOENT;

	// Flags are stored in a sprite sheet, so we need to
	// determine the column and row.
	struct flagpos_tbl_t {
		uint32_t lc;
		uint16_t col;
		uint16_t row;
	};
	static const flagpos_tbl_t flagpos_tbl[] = {
		{'hans',	0, 0},
		{'hant',	0, 0},
		{'au',		1, 3},	// GameTDB only
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
	};

	if (lc == 'en') {
		// Special case for English:
		// Use the 'us' flag if the country code is US,
		// and the 'gb' flag for everywhere else.
		// EXCEPTION: If forcing PAL mode, always use 'gb'.
		if (!forcePAL && getCountryCode() == 'US') {
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
		for (const flagpos_tbl_t &p : flagpos_tbl) {
			if (p.lc == lc) {
				// Match!
				*pCol = p.col;
				*pRow = p.row;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

/**
 * Convert a language code to a string.
 * NOTE: The language code will be converted to lowercase if necessary.
 * @param lc Language code.
 * @return String.
 */
string lcToString(uint32_t lc)
{
	string s_lc;
	s_lc.reserve(4);
	for (; lc != 0; lc <<= 8) {
		const uint8_t chr = static_cast<uint8_t>(lc >> 24);
		if (chr != 0) {
			s_lc += TOLOWER(chr);
		}
	}
	return s_lc;
}

/**
 * Convert a language code to a string.
 * NOTE: The language code will be converted to uppercase.
 * @param lc Language code.
 * @return String.
 */
string lcToStringUpper(uint32_t lc)
{
	string s_lc;
	s_lc.reserve(4);
	for (; lc != 0; lc <<= 8) {
		const uint8_t chr = static_cast<uint8_t>(lc >> 24);
		if (chr != 0) {
			s_lc += TOUPPER(chr);
		}
	}
	return s_lc;
}

#ifdef _WIN32
/**
 * Convert a language code to a wide string.
 * NOTE: The language code will be converted to lowercase if necessary.
 * @param lc Language code.
 * @return Wide string.
 */
wstring lcToWString(uint32_t lc)
{
	wstring ws_lc;
	ws_lc.reserve(4);
	for (; lc != 0; lc <<= 8) {
		uint8_t chr = (uint8_t)(lc >> 24);
		if (chr != 0) {
			ws_lc += towlower(chr);
		}
	}
	return ws_lc;
}

/**
 * Convert a language code to a wide string.
 * NOTE: The language code will be converted to uppercase.
 * @param lc Language code.
 * @return Wide string.
 */
wstring lcToWStringUpper(uint32_t lc)
{
	wstring ws_lc;
	ws_lc.reserve(4);
	for (; lc != 0; lc <<= 8) {
		uint8_t chr = (uint8_t)(lc >> 24);
		if (chr != 0) {
			ws_lc += towupper(chr);
		}
	}
	return ws_lc;
}
#endif /* _WIN32 */

} }

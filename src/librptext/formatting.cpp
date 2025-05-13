/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * formatting.cpp: Text formatting functions                               *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "formatting.hpp"

#ifdef _WIN32
// for the decimal point
#  include "conversion.hpp"
#endif /* _WIN32 && !HAVE_STRUCT_LCONV_WCHAR_T */

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpthreads/pthread_once.h"

// C includes
#ifdef HAVE_NL_LANGINFO
#  include <langinfo.h>
#else /* !HAVE_NL_LANGINFO */
#  include <clocale>
#endif /* HAVE_NL_LANGINFO */

// C includes (C++ namespace)
#include <cassert>
#include <cstring>
#include <cwctype>

// C++ includes and STL classes
#include <iomanip>
#include <sstream>
#include <string>
using std::ostringstream;
using std::string;

// libfmt
#include "rp-libfmt.h"

namespace LibRpText {

// Localized decimal point
static pthread_once_t lc_decimal_once_control = PTHREAD_ONCE_INIT;
static bool is_C_locale;
static char lc_decimal[8];

/** File size formatting **/

template<typename T>
static inline CONSTEXPR_MULTILINE int calc_frac_part_binary(T val, T mask)
{
	const float f = static_cast<float>(val & (mask - 1)) / static_cast<float>(mask);
	int frac_part = static_cast<int>(f * 1000.0f);
	if (frac_part >= 990) {
		// Edge case: The fractional portion is >= 99.
		// In extreme cases, it could be 100 due to rounding.
		// Always return 99 in this case.
		return 99;
	}

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	const int round_adj = ((frac_part % 10) > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

template<typename T>
static inline CONSTEXPR_MULTILINE int calc_frac_part_decimal(T val, T divisor)
{
	const float f = static_cast<float>(val % divisor) / static_cast<float>(divisor);
	int frac_part = static_cast<int>(f * 1000.0f);
	if (frac_part >= 990) {
		// Edge case: The fractional portion is >= 99.
		// In extreme cases, it could be 100 due to rounding.
		// Always return 99 in this case.
		return 99;
	}

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	const int round_adj = ((frac_part % 10) > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Initialize the localized decimal point.
 * Called by pthread_once().
 */
static void initLocalizedDecimalPoint(void)
{
	// Check if we're using the C locale.
	const char *locale = getenv("LC_MESSAGES");
	if (!locale || locale[0] == '\0') {
		locale = getenv("LC_ALL");
	}
	if (locale && locale[0] == 'C' && (locale[1] == '\0' || locale[1] == '.')) {
		// We're using the "C" locale. (or "C.UTF-8")
		is_C_locale = true;
		strcpy(lc_decimal, ".");
		return;
	}

	// Not using C locale. Get the localized decimal point.
#if defined(_WIN32)
	// Use localeconv(). (Windows: Convert from UTF-16 to UTF-8.)
#  if defined(HAVE_STRUCT_LCONV_WCHAR_T)
	// MSVCRT: `struct lconv` has wchar_t fields.
	const string s_dec = utf16_to_utf8(reinterpret_cast<const char16_t*>(
		localeconv()->_W_decimal_point), -1);
	strncpy(lc_decimal, s_dec.c_str(), sizeof(lc_decimal));
#  else /* !HAVE_STRUCT_LCONV_WCHAR_T */
	// MinGW v5,v6: `struct lconv` does not have wchar_t fields.
	// NOTE: The `char` fields are ANSI.
	const string s_dec = ansi_to_utf8(localeconv()->decimal_point, -1);
	strncpy(lc_decimal, s_dec.c_str(), sizeof(lc_decimal));
#  endif /* HAVE_STRUCT_LCONV_WCHAR_T */
#elif defined(HAVE_NL_LANGINFO)
	// Use nl_langinfo().
	// Reference: https://www.gnu.org/software/libc/manual/html_node/The-Elegant-and-Fast-Way.html
	// NOTE: RADIXCHAR is the portable version of DECIMAL_POINT.
	const char *const radix = nl_langinfo(RADIXCHAR);
	strncpy(lc_decimal, radix ? radix : ".", sizeof(lc_decimal));
#else
	// Use localeconv(). (Assuming UTF-8)
	const char *const radix = localeconv()->decimal_point;
	strncpy(lc_decimal, radix ? radix : ".", sizeof(lc_decimal));
#endif

	// Ensure NULL-termination.
	lc_decimal[sizeof(lc_decimal)-1] = '\0';
}

/**
 * Format a file size.
 * @param size File size
 * @param dialect
 * @return Formatted file size.
 */
string formatFileSize(off64_t size, BinaryUnitDialect dialect)
{
	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;
	bool needs_pgettext = true;

	if (likely(dialect != BinaryUnitDialect::MetricBinaryDialect)) {
		// Binary KiB (or binary KB)
		const bool isKiB = (dialect == BinaryUnitDialect::DefaultBinaryDialect ||
		                    dialect == BinaryUnitDialect::IECBinaryDialect);

		if (size < 0) {
			// Invalid size. Print the value as-is.
			suffix = nullptr;
			whole_part = static_cast<int>(size);
			frac_part = 0;
		} else if (size < (2LL << 10)) {
			// tr: Bytes (< 1,024)
			suffix = NC_("LibRpText|FileSize", "byte", "bytes", static_cast<int>(size));
			needs_pgettext = false;
			whole_part = static_cast<int>(size);
			frac_part = 0;
		} else if (size < (2LL << 20)) {
			suffix = isKiB
				// tr: Kilobytes (binary)
				? NOP_C_("LibRpText|FileSize", "KiB")
				// tr: Kilobytes (decimal)
				: NOP_C_("LibRpText|FileSize", "KB");
			whole_part = static_cast<int>(size >> 10);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 10));
		} else if (size < (2LL << 30)) {
			suffix = isKiB
				// tr: Megabytes (binary)
				? NOP_C_("LibRpText|FileSize", "MiB")
				// tr: Megabytes (decimal)
				: NOP_C_("LibRpText|FileSize", "MB");
			whole_part = static_cast<int>(size >> 20);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 20));
		} else if (size < (2LL << 40)) {
			suffix = isKiB
				// tr: Gigabytes (binary)
				? NOP_C_("LibRpText|FileSize", "GiB")
				// tr: Gigabytes (decimal)
				: NOP_C_("LibRpText|FileSize", "GB");
			whole_part = static_cast<int>(size >> 30);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 30));
		} else if (size < (2LL << 50)) {
			suffix = isKiB
				// tr: Terabytes (binary)
				? NOP_C_("LibRpText|FileSize", "TiB")
				// tr: Terabytes (decimal)
				: NOP_C_("LibRpText|FileSize", "TB");
			whole_part = static_cast<int>(size >> 40);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 40));
		} else if (size < (2LL << 60)) {
			suffix = isKiB
				// tr: Petabytes (binary)
				? NOP_C_("LibRpText|FileSize", "PiB")
				// tr: Petabytes (decimal)
				: NOP_C_("LibRpText|FileSize", "PB");
			whole_part = static_cast<int>(size >> 50);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 50));
		} else /*if (size < (2LL << 70))*/ {
			suffix = isKiB
				// tr: Exabytes (binary)
				? NOP_C_("LibRpText|FileSize", "EiB")
				// tr: Exabytes (decimal)
				: NOP_C_("LibRpText|FileSize", "EB");
			whole_part = static_cast<int>(size >> 60);
			frac_part = calc_frac_part_binary<off64_t>(size, (1LL << 60));
		}
	} else {
		// Decimal KB
		if (size < 0) {
			// Invalid size. Print the value as-is.
			suffix = nullptr;
			whole_part = static_cast<int>(size);
			frac_part = 0;
		} else if (size < (2LL * 1000)) {
			// tr: Bytes (< 1,000)
			suffix = NC_("LibRpText|FileSize", "byte", "bytes", static_cast<int>(size));
			needs_pgettext = false;
			whole_part = static_cast<int>(size);
			frac_part = 0;
		} else if (size < (2LL * 1000*1000)) {
			// tr: Kilobytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "KB");
			whole_part = static_cast<int>(size / 1000);
			frac_part = calc_frac_part_decimal<off64_t>(size, (1LL * 1000));
		} else if (size < (2LL * 1000*1000*1000)) {
			// tr: Megabytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "MB");
			whole_part = static_cast<int>(size / (1000LL * 1000));
			frac_part = calc_frac_part_decimal<off64_t>(size, (1000LL * 1000));
		} else if (size < (2LL * 1000*1000*1000*1000)) {
			// tr: Gigabytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "GB");
			whole_part = static_cast<int>(size / (1000LL * 1000 * 1000));
			frac_part = calc_frac_part_decimal<off64_t>(size, (1000LL * 1000 * 1000));
		} else if (size < (2LL * 1000*1000*1000*1000*1000)) {
			// tr: Terabytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "TB");
			whole_part = static_cast<int>(size / (1000LL * 1000 * 1000 * 1000));
			frac_part = calc_frac_part_decimal<off64_t>(size, (1000LL * 1000 * 1000 * 1000));
		} else if (size < (2LL * 1000*1000*1000*1000*1000*1000)) {
			// tr: Petabytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "PB");
			whole_part = static_cast<int>(size / (1000LL * 1000 * 1000 * 1000 * 1000));
			frac_part = calc_frac_part_decimal<off64_t>(size, (1000LL * 1000 * 1000 * 1000 * 1000));
		} else /*if (size < (2LL * 1000*1000*1000*1000*1000*1000*1000))*/ {
			// tr: Exabytes (decimal)
			suffix = NOP_C_("LibRpText|FileSize", "EB");
			whole_part = static_cast<int>(size / (1000LL * 1000 * 1000 * 1000 * 1000 * 1000));
			frac_part = calc_frac_part_decimal<off64_t>(size, (1000LL * 1000 * 1000 * 1000 * 1000 * 1000));
		}
	}

	// Do the actual localization here.
	if (suffix && needs_pgettext) {
		suffix = pgettext_expr("LibRpText|FileSize", suffix);
	}

	// Localize the whole part.
	ostringstream s_value;
#ifdef _WIN32
	if (is_C_locale) {
		// MSVC doesn't handle LC_ALL/LC_MESSAGES, so we have to
		// set ostringstream's locale manually.
		// TODO: Cache the C locale?
		s_value.imbue(std::locale("C"));
	}
#endif /* _WIN32 */
	s_value << whole_part;

	if (size >= (2LL << 10)) {
		// Fractional part
		int frac_digits = 2;
		if (whole_part >= 10) {
			const int round_adj = ((frac_part % 10) > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}

		// Ensure the localized decimal point is initialized.
		pthread_once(&lc_decimal_once_control, initLocalizedDecimalPoint);

		// Append the fractional part using the required number of digits.
		s_value << lc_decimal;
		s_value << std::setw(frac_digits) << std::setfill('0') << frac_part;
	}

	if (suffix) {
		// tr: {0:s} == localized value, {1:s} == suffix (e.g. MiB)
		return fmt::format(FRUN(C_("LibRpText|FileSize", "{0:s} {1:s}")),
			s_value.str(), suffix);
	}

	// No suffix needed.
	return s_value.str();
}

/**
 * Format a file size, in KiB.
 *
 * This function expects the size to be a multiple of 1024,
 * so it doesn't do any fractional rounding or printing.
 *
 * @param size File size
 * @param dialect
 * @return Formatted file size.
 */
std::string formatFileSizeKiB(unsigned int size, BinaryUnitDialect dialect)
{
	// Localize the number.
	// FIXME: If using C locale, don't do localization.
	if (likely(dialect != BinaryUnitDialect::MetricBinaryDialect)) {
		size /= 1024;
	} else {
		size /= 1000;
	}

	const char *suffix;
	switch (dialect) {
		default:
		case BinaryUnitDialect::DefaultBinaryDialect:
		case BinaryUnitDialect::IECBinaryDialect:
			suffix = C_("LibRpText|FileSize", "KiB");
			break;

		case BinaryUnitDialect::JEDECBinaryDialect:
		case BinaryUnitDialect::MetricBinaryDialect:
			suffix = C_("LibRpText|FileSize", "KB");
			break;
	}

	// tr: {0:Ld} == localized value, {1:s} == suffix (e.g. MiB)
	return fmt::format(FRUN(C_("LibRpText|FileSize", "{0:Ld} {1:s}")), size, suffix);
}

/**
 * Format a frequency.
 * @param frequency Frequency.
 * @return Formatted frequency.
 */
std::string formatFrequency(uint32_t frequency)
{
	const char *suffix;
	// frac_part is always 0 to 1,000.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	if (frequency < (2*1000)) {
		// tr: Hertz (< 1,000)
		suffix = C_("LibRpText|Frequency", "Hz");
		whole_part = frequency;
		frac_part = 0;
	} else if (frequency < (2*1000*1000)) {
		// tr: Kilohertz
		suffix = C_("LibRpText|Frequency", "kHz");
		whole_part = frequency / 1000;
		frac_part = frequency % 1000;
	} else if (frequency < (2*1000*1000*1000)) {
		// tr: Megahertz
		suffix = C_("LibRpText|Frequency", "MHz");
		whole_part = frequency / (1000*1000);
		frac_part = (frequency / 1000) % 1000;
	} else /*if (frequency < (2*1000*1000*1000*1000))*/ {
		// tr: Gigahertz
		suffix = C_("LibRpText|Frequency", "GHz");
		whole_part = frequency / (1000*1000*1000);
		frac_part = (frequency / (1000*1000)) % 1000;
	}

	// Localize the whole part.
	ostringstream s_value;
#ifdef _WIN32
	if (is_C_locale) {
		// MSVC doesn't handle LC_ALL/LC_MESSAGES, so we have to
		// set ostringstream's locale manually.
		// TODO: Cache the C locale?
		s_value.imbue(std::locale("C"));
	}
#endif /* _WIN32 */
	s_value << whole_part;

	if (frequency >= (2*1000)) {
		// Fractional part
		const int frac_digits = 3;

		// Ensure the localized decimal point is initialized.
		pthread_once(&lc_decimal_once_control, initLocalizedDecimalPoint);

		// Append the fractional part using the required number of digits.
		s_value << lc_decimal;
		s_value << std::setw(frac_digits) << std::setfill('0') << frac_part;
	}

	if (suffix) {
		// tr: {0:s} == localized value, {1:s} == suffix (e.g. MHz)
		return fmt::format(FRUN(C_("LibRpText|Frequency", "{0:s} {1:s}")),
			s_value.str(), suffix);
	}

	// No suffix needed.
	return s_value.str();
}

/** Audio functions **/

/**
 * Format a sample value as m:ss.cs.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return m:ss.cs
 */
string formatSampleAsTime(unsigned int sample, unsigned int rate)
{
	unsigned int min, sec, cs;

	assert(rate != 0);
	if (rate == 0) {
		// Division by zero! Someone goofed.
		return "#DIV/0!";
	}

	assert(rate < 21474836);	// debug assert on overflow
	const unsigned int cs_frames = (sample % rate);
	if (cs_frames != 0) {
		// Calculate centiseconds.
		cs = (cs_frames * 100) / rate;
	} else {
		// No centiseconds.
		cs = 0;
	}

	sec = sample / rate;
	min = sec / 60;
	sec %= 60;

	return fmt::format(FSTR("{:d}:{:0>2d}.{:0>2d}"), min, sec, cs);
}

/**
 * Convert a sample value to milliseconds.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return Milliseconds.
 */
unsigned int convSampleToMs(unsigned int sample, unsigned int rate)
{
	unsigned int sec, ms;

	assert(rate < 2147483);		// debug assert on overflow
	const unsigned int ms_frames = (sample % rate);
	if (ms_frames != 0) {
		// Calculate milliseconds.
		ms = (ms_frames * 1000) / rate;
	} else {
		// No milliseconds.
		ms = 0;
	}

	// Calculate seconds.
	sec = sample / rate;

	// Convert to milliseconds and add the milliseconds value.
	return (sec * 1000) + ms;
}

}

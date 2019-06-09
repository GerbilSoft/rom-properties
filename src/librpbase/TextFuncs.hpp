/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs.hpp: Text encoding functions.                                 *
 *                                                                         *
 * Copyright (c) 2009-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__

// System byteorder is needed for conversions from UTF-16.
// Conversions to UTF-16 always use host-endian.
#include "byteorder.h"

// printf()-style function attribute.
#ifndef ATTR_PRINTF
# ifdef __GNUC__
#  define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
# else
#  define ATTR_PRINTF(fmt, args)
# endif
#endif /* ATTR_PRINTF */

// C includes.
#include <stddef.h>	/* size_t */

// C++ includes.
#include <string>

#ifdef __cplusplus
namespace LibRpBase {

/* Define to 1 if the system has a 16-bit wchar_t. */
#ifdef _WIN32
# define RP_WIS16 1
#endif

/** UTF-16 string functions. **/

/**
 * char16_t strlen().
 * @param wcs 16-bit string.
 * @return Length of wcs, in characters.
 */
#ifdef RP_WIS16
static inline size_t u16_strlen(const char16_t *wcs)
{
	return wcslen(reinterpret_cast<const wchar_t*>(wcs));
}
#else /* !RP_WIS16 */
size_t u16_strlen(const char16_t *wcs);
#endif /* RP_WIS16 */

/**
 * char16_t strnlen().
 * @param wcs 16-bit string.
 * @param maxlen Maximum length.
 * @return Length of wcs, in characters.
 */
#ifdef RP_WIS16
static inline size_t u16_strnlen(const char16_t *wcs, size_t maxlen)
{
	return wcsnlen(reinterpret_cast<const wchar_t*>(wcs), maxlen);
}
#else /* !RP_WIS16 */
size_t u16_strnlen(const char16_t *wcs, size_t maxlen);
#endif /* RP_WIS16 */

/**
 * char16_t strdup().
 * @param wcs 16-bit string.
 * @return Copy of wcs.
 */
#ifdef RP_WIS16
static inline char16_t *u16_strdup(const char16_t *wcs)
{
	return reinterpret_cast<char16_t*>(
		wcsdup(reinterpret_cast<const wchar_t*>(wcs)));
}
#else /* !RP_WIS16 */
char16_t *u16_strdup(const char16_t *wcs);
#endif /* RP_WIS16 */

/**
 * char16_t strcmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcmp() result.
 */
#ifdef RP_WIS16
static inline int u16_strcmp(const char16_t *wcs1, const char16_t *wcs2)
{
	return wcscmp(reinterpret_cast<const wchar_t*>(wcs1),
		      reinterpret_cast<const wchar_t*>(wcs2));
}
#else /* !RP_WIS16 */
int u16_strcmp(const char16_t *wcs1, const char16_t *wcs2);
#endif /* RP_WIS16 */

/**
 * char16_t strcmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcmp() result.
 */
#ifdef RP_WIS16
static inline int u16_strcasecmp(const char16_t *wcs1, const char16_t *wcs2)
{
	return wcscasecmp(reinterpret_cast<const wchar_t*>(wcs1),
			  reinterpret_cast<const wchar_t*>(wcs2));
}
#else /* !RP_WIS16 */
int u16_strcasecmp(const char16_t *wcs1, const char16_t *wcs2);
#endif /* RP_WIS16 */

/** Text conversion functions **/

// NOTE: All of these functions will remove trailing
// NULL characters from strings.

#ifndef CP_ACP
# define CP_ACP 0
#endif
#ifndef CP_LATIN1
# define CP_LATIN1 28591
#endif
#ifndef CP_UTF8
# define CP_UTF8 65001
#endif

// Text conversion flags.
typedef enum {
	// Enable cp1252 fallback if the text fails to
	// decode using the specified code page.
	TEXTCONV_FLAG_CP1252_FALLBACK		= (1 << 0),
} TextConv_Flags_e;

/**
 * Convert 8-bit text to UTF-8.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] 8-bit text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @param flags	[in] Flags. (See TextConv_Flags_e.)
 * @return UTF-8 string.
 */
std::string cpN_to_utf8(unsigned int cp, const char *str, int len, unsigned int flags = 0);

/**
 * Convert 8-bit text to UTF-16.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] 8-bit text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @param flags	[in] Flags. (See TextConv_Flags_e.)
 * @return UTF-16 string.
 */
std::u16string cpN_to_utf16(unsigned int cp, const char *str, int len, unsigned int flags = 0);

/**
 * Convert UTF-8 to 8-bit text.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 * Invalid characters will be ignored.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return 8-bit text.
 */
std::string utf8_to_cpN(unsigned int cp, const char *str, int len);

/**
 * Convert UTF-16 to 8-bit text.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 * Invalid characters will be ignored.
 *
 * @param cp	[in] Code page number.
 * @param wcs	[in] UTF-16 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return 8-bit text.
 */
std::string utf16_to_cpN(unsigned int cp, const char16_t *wcs, int len);

/** Inline wrappers for text conversion functions **/

/* ANSI */

/**
 * Convert ANSI text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] ANSI text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string ansi_to_utf8(const char *str, int len)
{
	return cpN_to_utf8(CP_ACP, str, len);
}

/**
 * Convert ANSI text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] ANSI text.
 * @return UTF-8 string.
 */
static inline std::string ansi_to_utf8(const std::string &str)
{
	return cpN_to_utf8(CP_ACP, str.data(), static_cast<int>(str.size()));
}

/**
 * Convert UTF-8 text to ANSI
 * Trailing NULL bytes will be removed.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return ANSI string.
 */
static inline std::string utf8_to_ansi(const char *str, int len)
{
	return utf8_to_cpN(CP_ACP, str, len);
}

/**
 * Convert UTF-8 text to ANSI
 * Trailing NULL bytes will be removed.
 * @param str	[in] ANSI text.
 * @return ANSI string.
 */
static inline std::string utf8_to_ansi(const std::string &str)
{
	return utf8_to_cpN(CP_ACP, str.data(), static_cast<int>(str.size()));
}

/* cp1252 */

/**
 * Convert cp1252 text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string cp1252_to_utf8(const char *str, int len)
{
	return cpN_to_utf8(1252, str, len);
}

/**
 * Convert cp1252 text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 text.
 * @return UTF-8 string.
 */
static inline std::string cp1252_to_utf8(const std::string &str)
{
	return cpN_to_utf8(1252, str.data(), static_cast<int>(str.size()));
}

/**
 * Convert cp1252 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
static inline std::u16string cp1252_to_utf16(const char *str, int len)
{
	return cpN_to_utf16(1252, str, len);
}

/**
 * Convert UTF-8 text to cp1252.
 * Trailing NULL bytes will be removed.
 * Invalid characters will be ignored.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return cp1252 string.
 */
static inline std::string utf8_to_cp1252(const char *str, int len)
{
	return utf8_to_cpN(1252, str, len);
}

/**
 * Convert UTF-16 text to cp1252.
 * Trailing NULL bytes will be removed.
 * Invalid characters will be ignored.
 * @param wcs	[in] UTF-16 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return cp1252 string.
 */
static inline std::string utf16_to_cp1252(const char16_t *wcs, int len)
{
	return utf16_to_cpN(1252, wcs, len);
}

/* Shift-JIS (cp932) with cp1252 fallback */

/**
 * Convert cp1252 or Shift-JIS (cp932) text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 or Shift-JIS (cp932) text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string cp1252_sjis_to_utf8(const char *str, int len)
{
	return cpN_to_utf8(932, str, len, TEXTCONV_FLAG_CP1252_FALLBACK);
}

/**
 * Convert cp1252 or Shift-JIS (cp932) text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 or Shift-JIS string.
 * @return UTF-8 string.
 */
static inline std::string cp1252_sjis_to_utf8(const std::string &str)
{
	return cpN_to_utf8(932, str.data(), static_cast<int>(str.size()),
		TEXTCONV_FLAG_CP1252_FALLBACK);
}

/**
 * Convert cp1252 or Shift-JIS (cp932) text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str	[in] cp1252 or Shift-JIS (cp932) text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
static inline std::u16string cp1252_sjis_to_utf16(const char *str, int len)
{
	return cpN_to_utf16(932, str, len, TEXTCONV_FLAG_CP1252_FALLBACK);
}

/* Latin-1 (ISO-8859-1) */

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] Latin-1 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string latin1_to_utf8(const char *str, int len)
{
	return cpN_to_utf8(CP_LATIN1, str, len, TEXTCONV_FLAG_CP1252_FALLBACK);
}

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8
 * Trailing NULL bytes will be removed.
 * @param str	[in] Latin-1 string.
 * @return UTF-8 string.
 */
static inline std::string latin1_to_utf8(const std::string &str)
{
	return cpN_to_utf8(CP_LATIN1, str.data(), static_cast<int>(str.size()),
		TEXTCONV_FLAG_CP1252_FALLBACK);
}

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str	[in] Latin-1 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
static inline std::u16string latin1_to_utf16(const char *str, int len)
{
	return cpN_to_utf16(CP_LATIN1, str, len, TEXTCONV_FLAG_CP1252_FALLBACK);
}

/**
 * Convert UTF-8 text to Latin-1 (ISO-8859-1).
 * Trailing NULL bytes will be removed.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return Latin-1 string.
 */
static inline std::string utf8_to_latin1(const char *str, int len)
{
	return utf8_to_cpN(CP_LATIN1, str, len);
}

/**
 * Convert UTF-8 text to Latin-1 (ISO-8859-1).
 * Trailing NULL bytes will be removed.
 * @param str	[in] UTF-8 text.
 * @return Latin-1 string.
 */
static inline std::string utf8_to_latin1(const std::string &str)
{
	return utf8_to_cpN(CP_LATIN1, str.data(), static_cast<int>(str.size()));
}

/**
 * Convert UTF-16 Latin-1 (ISO-8859-1).
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return Latin-1 string.
 */
static inline std::string utf16_to_latin1(const char16_t *wcs, int len)
{
	return utf16_to_cpN(CP_LATIN1, wcs, len);
}

/* UTF-8 to UTF-16 and vice-versa */

/**
 * Convert UTF-8 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
static inline std::u16string utf8_to_utf16(const char *str, int len)
{
	return cpN_to_utf16(CP_UTF8, str, len);
}

/**
 * Convert UTF-8 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str UTF-8 string.
 * @return UTF-16 string.
 */
static inline std::u16string utf8_to_utf16(const std::string &str)
{
	return cpN_to_utf16(CP_UTF8, str.data(), static_cast<int>(str.size()));
}

/* Specialized UTF-16 conversion functions */

/**
 * Convert UTF-16LE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16LE text.
 * @param len	[in] Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16le_to_utf8(const char16_t *wcs, int len);

/**
 * Convert UTF-16BE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16BE text.
 * @param len	[in] Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16be_to_utf8(const char16_t *wcs, int len);

/**
 * Convert UTF-16 text to UTF-8. (host-endian)
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16 text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string utf16_to_utf8(const char16_t *wcs, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16le_to_utf8(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16be_to_utf8(wcs, len);
#endif
}

/** UTF-16 to UTF-16 conversion functions **/

/**
 * Byteswap and return UTF-16 text.
 * @param wcs UTF-16 text to byteswap.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Byteswapped UTF-16 string.
 */
std::u16string utf16_bswap(const char16_t *wcs, int len);

/**
 * Convert UTF-16LE text to host-endian UTF-16.
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16LE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16le_to_utf16(const char16_t *wcs, int len)
{
	// Check for a NULL terminator.
	if (len < 0) {
		len = static_cast<int>(u16_strlen(wcs));
	} else {
		len = static_cast<int>(u16_strnlen(wcs, len));
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return std::u16string(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16_bswap(wcs, len);
#endif
}

/**
 * Convert UTF-16BE text to host-endian UTF-16.
 * @param wcs UTF-16BLE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16be_to_utf16(const char16_t *wcs, int len)
{
	// Check for a NULL terminator.
	if (len < 0) {
		len = static_cast<int>(u16_strlen(wcs));
	} else {
		len = static_cast<int>(u16_strnlen(wcs, len));
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16_bswap(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return std::u16string(wcs, len);
#endif
}

/** Specialized text conversion functions **/

/**
 * Convert Atari ST text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] Atari ST text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string atariST_to_utf8(const char *str, int len);

/**
 * Convert Atari ATASCII text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] Atari ATASCII text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string atascii_to_utf8(const char *str, int len);

/**
 * Convert Commodore PETSCII text (C64 variant) to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] Commodore PETSCII text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @param shifted [in] False for unshifted (uppercase+graphics); true for shifted (lowercase+uppercase).
 * @return UTF-8 string.
 */
std::string petscii_to_utf8(const char *str, int len, bool shifted = false);

/** sprintf() **/

/**
 * sprintf()-style function for std::string.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string
 */
std::string rp_sprintf(const char *fmt, ...) ATTR_PRINTF(1, 2);

#ifdef _MSC_VER
/**
 * sprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string.
 */
std::string rp_sprintf_p(const char *fmt, ...) ATTR_PRINTF(1, 2);
#else
// glibc supports positional format string arguments
// in the standard printf() functions.
#define rp_sprintf_p(fmt, ...) rp_sprintf(fmt, ##__VA_ARGS__)
#endif

/** Other useful text functions **/

/**
 * Format a file size.
 * @param fileSize File size.
 * @return Formatted file size.
 */
std::string formatFileSize(int64_t fileSize);

/**
 * Remove trailing spaces from a string.
 * NOTE: This modifies the string *in place*.
 * @param str String.
 */
void trimEnd(std::string &str);

/**
 * Convert DOS (CRLF) line endings to UNIX (LF) line endings.
 * @param str_dos	[in] String with DOS line endings.
 * @param len		[in] Length of str, in characters. (-1 for NULL-terminated string)
 * @param lf_count	[out,opt] Number of CRLF pairs found.
 * @return String with UNIX line endings.
 */
std::string dos2unix(const char *str_dos, int len = -1, int *lf_count = nullptr);

/**
 * Convert DOS (CRLF) line endings to UNIX (LF) line endings.
 * @param str_dos	[in] String with DOS line endings.
 * @param lf_count	[out,opt] Number of CRLF pairs found.
 * @return String with UNIX line endings.
 */
static inline std::string dos2unix(const std::string &str_dos, int *lf_count = nullptr)
{
	return dos2unix(str_dos.data(), static_cast<int>(str_dos.size()), lf_count);
}

/** Audio functions. **/

/**
 * Format a sample value as m:ss.cs.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return m:ss.cs
 */
std::string formatSampleAsTime(unsigned int sample, unsigned int rate);

/**
 * Convert a sample value to milliseconds.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return Milliseconds.
 */
unsigned int convSampleToMs(unsigned int sample, unsigned int rate);

}
#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * html_entities.cpp: HTML entities handling.                              *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "html_entities.hpp"

// C includes (C++ namespace)
#include <cstddef>	// for size_t
#include <cstdlib>
#include <cstring>

// C++ STL classes
#include <array>
using std::array;

namespace LibRpText { namespace HtmlEntities {

/**
 * HTML entity entry, sorted by entity name.
 * Can be used with e.g. bsearch() or std::lower_bound().
 * References:
 * - https://www.w3schools.com/HTML/html_entities.asp
 * - https://www.toptal.com/designers/htmlarrows/symbols/
 */
struct html_entity_tbl_t {
	char entity[8];	// HTML entity, minus '&' and ';'
	char16_t chr;	// UTF-16 code point
};

/**
 * HTML entities, sorted by entity name.
 * Can be used with e.g. bsearch() or std::lower_bound().
 * References:
 * - https://www.w3schools.com/HTML/html_entities.asp
 * - https://www.toptal.com/designers/htmlarrows/symbols/
 */
static const array<html_entity_tbl_t, 76> html_entity_tbl = {{
	{"Cfr",		0x212D},	// ℭ
	{"Copf",	0x2102},	// ℂ
	{"DD",		0x2145},	// ⅅ
	{"Escr",	0x2130},	// ℰ
	{"Fscr",	0x2131},	// ℱ
	{"Hfr",		0x210C},	// ℌ
	{"Hopf",	0x210D},	// ℍ
	{"Lscr",	0x2112},	// ℒ
	{"Mscr",	0x2133},	// ℳ
	{"Nopf",	0x2115},	// ℕ
	{"Popf",	0x2119},	// ℙ
	{"Qopf",	0x211A},	// ℚ
	{"Ropf",	0x211D},	// ℝ
	{"Rscr",	0x211B},	// ℛ
	{"Zfr",		0x2128},	// ℨ
	{"Zopf",	0x2124},	// ℤ
	{"alefsym",	0x2135},	// ℵ
	{"amp",		0x0026},	// &
	{"apos",	0x0027},	// '
	{"bernou",	0x212C},	// ℬ
	{"beth",	0x2136},	// ℶ
	{"cent",	0x00A2},	// ¢
	{"check",	0x2713},	// ✓
	{"clubs",	0x2663},	// ♣
	{"commat",	0x00A0},	// @
	{"copy",	0x00A9},	// ©
	{"copysr",	0x2117},	// ℗
	{"cross",	0x2717},	// ✗
	{"daleth",	0x2138},	// ℸ
	{"dd",		0x2146},	// ⅆ
	{"diams",	0x2666},	// ♦
	{"ee",		0x2147},	// ⅇ
	{"ell",		0x2113},	// ℓ
	{"escr",	0x212F},	// ℯ
	{"euro",	0x20AC},	// €
	{"female",	0x2640},	// ♀
	{"flat",	0x266D},	// ♭
	{"gimel",	0x2137},	// ℷ
	{"gscr",	0x210A},	// ℊ
	{"gt",		0x003E},	// >
	{"hamlit",	0x210B},	// ℋ
	{"hearts",	0x2665},	// ♥
	{"ii",		0x2148},	// ⅈ
	{"iiota",	0x2129},	// ℩
	{"image",	0x2111},	// ℑ
	{"incare",	0x2105},	// ℅
	{"lbbrk",	0x2772},	// ❲
	{"lscr",	0x2110},	// ℐ
	{"lt",		0x003C},	// <
	{"male",	0x2642},	// ♂
	{"malt",	0x2720},	// ✠
	{"mho",		0x2127},	// ℧
	{"natural",	0x266E},	// ♮
	{"nbsp",	0x00A0},	// nbsp
	{"numero",	0x2116},	// №
	{"oscr",	0x2134},	// ℴ
	{"para",	0x00B6},	// ¶
	{"phone",	0x260E},	// ☎
	{"planck",	0x210F},	// ℏ
	{"planckh",	0x210E},	// ℎ
	{"pound",	0x00A3},	// £
	{"quot",	0x0022},	// "
	{"rbbrk",	0x2773},	// ❳
	{"real",	0x211C},	// ℜ
	{"reg",		0x00AE},	// ®
	{"rx",		0x211E},	// ℞
	{"sect",	0x00A7},	// §
	{"sext",	0x2736},	// ✶
	{"sharp",	0x266F},	// ♯
	{"spades",	0x2660},	// ♠
	{"star",	0x2606},	// ☆
	{"starf",	0x2605},	// ★
	{"sung",	0x266A},	// ♪
	{"trade",	0x2122},	// ™
	//{"VerticalSeparator",	0x2758},	// ❘ (TODO: Special check for this one?)
	{"weierp",	0x2118},	// ℘
	{"yen",		0x00A5},	// ¥
}};

/** Wrapper functions for T_parseHtmlEntity **/

static inline const char *strchr_wrapper(const char *s, int c)
{
	return strchr(s, c);
}

static inline unsigned long strtoul_wrapper(const char *nptr, char **endptr, int base)
{
	return strtoul(nptr, endptr, base);
}

#ifdef _WIN32
static inline const wchar_t *strchr_wrapper(const wchar_t *ws, wchar_t wc)
{
	return wcschr(ws, wc);
}

static inline unsigned long strtoul_wrapper(const wchar_t *nptr, wchar_t **endptr, int base)
{
	return wcstoul(nptr, endptr, base);
}
#endif /* _WIN32 */

/**
 * Parse an HTML entity.
 * @param entity Pointer to HTML tag (will be modified) (MUST be pointing to a NULL-terminated string!)
 * @return Parsed HTML entity (as a Unicode code point)
 */
template<typename CharType>
static char32_t T_parseHtmlEntity(const CharType *&entity)
{
	// First character is always '&', so skip it.
	const CharType *p = entity + 1;

	// Find the end of the entity string.
	const CharType *const p_end = strchr_wrapper(p, ';');
	if (!p_end) {
		// No end... Skip it.
		entity++;
		return U'&';
	}

	size_t len = p_end - p;
	if (len > 32) {
		// Entity is too big...
		entity++;
		return U'&';
	}

	// Need to copy the value into a buffer for NULL termination.
	CharType tmpbuf[33];

	// Check for a numeric entity.
	if (p[0] == '#') {
		// Numeric Unicode entity.
		CharType *endptr = nullptr;
		unsigned int chr;
		if (p[1] == 'x') {
			// Hexadecimal number
			p += 2;
			len -= 2;
			memcpy(tmpbuf, p, len * sizeof(CharType));
			tmpbuf[len] = '\0';
			chr = strtoul_wrapper(tmpbuf, &endptr, 16);
		} else {
			// Decimal number
			p++;
			len--;
			memcpy(tmpbuf, p, len * sizeof(CharType));
			tmpbuf[len] = '\0';
			chr = strtoul_wrapper(tmpbuf, &endptr, 10);
		}
		entity = p_end + 1;
		return chr;
	}

	// Check for known HTML entities.
	// FIXME: std::lower_bound() isn't working...
	// Using bsearch() instead.
	html_entity_tbl_t key;
	if (len >= sizeof(key.entity) - 1) {
		// Entity is too long!
		entity++;
	}
	// TODO: `if constexpr` to use strcpy() for char?
	// NOTE: html_entity_tbl_t uses ASCII.
	// Assuming the source entity is also ASCII.
	// TODO: Fail if it isn't...
	for (size_t i = 0; i < len && p[i] != 0; i++) {
		key.entity[i] = static_cast<char>(p[i]);
	}
	key.entity[len] = '\0';
	key.chr = 0;

	void *ptr = bsearch(&key, html_entity_tbl.data(),
		html_entity_tbl.size(), sizeof(html_entity_tbl_t),
		[](const void *a, const void *b) -> int
		{
			const html_entity_tbl_t *const pa = static_cast<const html_entity_tbl_t*>(a);
			const html_entity_tbl_t *const pb = static_cast<const html_entity_tbl_t*>(b);
			return strcmp(pa->entity, pb->entity);
		});

	if (!ptr) {
		// Unsupported entity...
		entity++;
		return U'&';
	}

	// Return the decoded entity.
	entity = p_end + 1;
	const html_entity_tbl_t *const p_tbl = reinterpret_cast<const html_entity_tbl_t*>(ptr);
	return static_cast<char32_t>(p_tbl->chr);
}

/**
 * Parse an HTML entity.
 * @param entity Pointer to HTML tag (will be modified) (MUST be pointing to a NULL-terminated string!)
 * @return Parsed HTML entity (as a Unicode code point)
 */
char32_t parseHtmlEntity(const char *&entity)
{
	return T_parseHtmlEntity(entity);
}

#ifdef _WIN32
/**
 * Parse an HTML entity.
 * @param entity Pointer to HTML tag (will be modified) (MUST be pointing to a NULL-terminated string!)
 * @return Parsed HTML entity (as a Unicode code point)
 */
char32_t parseHtmlEntity(const wchar_t *&entity)
{
	return T_parseHtmlEntity(entity);
}
#endif /* _WIN32 */

}} // namespace LibRpText::HtmlEntities

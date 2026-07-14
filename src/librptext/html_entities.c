/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * html_entities.c: HTML entities table.                                   *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "html_entities.h"

#include "common.h"	// for ARRAY_SIZE()

/**
 * HTML entities, sorted by entity name.
 * Can be used with e.g. bsearch() or std::lower_bound().
 * References:
 * - https://www.w3schools.com/HTML/html_entities.asp
 * - https://www.toptal.com/designers/htmlarrows/symbols/
 */
static const html_entity_tbl_t html_entity_tbl[] = {
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

	// end of table
	{"", 0}
};

/**
 * Get the HTML entities table.
 * Table is terminated with an empty-string entry.
 *
 * @return HTML entities table
 */
const html_entity_tbl_t *rp_get_html_entities_table(void)
{
	return html_entity_tbl;
}

/**
 * Get the number of entries in the HTML entities table.
 * NOTE: This does *not* include the empty-string terminator entry.
 *
 * @return Number of entries in the HTML entities table
 */
size_t rp_get_html_entities_table_count(void)
{
	return ARRAY_SIZE(html_entity_tbl) - 1;
}

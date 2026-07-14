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
	{"Cfr",		"ℭ"},
	{"Copf",	"ℂ"},
	{"DD",		"ⅅ"},
	{"Escr",	"ℰ"},
	{"Fscr",	"ℱ"},
	{"Hfr",		"ℌ"},
	{"Hopf",	"ℍ"},
	{"Lscr",	"ℒ"},
	{"Mscr",	"ℳ"},
	{"Nopf",	"ℕ"},
	{"Popf",	"ℙ"},
	{"Qopf",	"ℚ"},
	{"Ropf",	"ℝ"},
	{"Rscr",	"ℛ"},
	{"Zfr",		"ℨ"},
	{"Zopf",	"ℤ"},
	{"alefsym",	"ℵ"},
	{"amp",		"&"},
	{"apos",	"'"},
	{"bernou",	"ℬ"},
	{"beth",	"ℶ"},
	{"cent",	"¢"},
	{"check",	"✓"},
	{"clubs",	"♣"},
	{"commat",	"@"},
	{"copy",	"©"},
	{"copysr",	"℗"},
	{"cross",	"✗"},
	{"daleth",	"ℸ"},
	{"dd",		"ⅆ"},
	{"diams",	"♦"},
	{"ee",		"ⅇ"},
	{"ell",		"ℓ"},
	{"escr",	"ℯ"},
	{"euro",	"€"},
	{"female",	"♀"},
	{"flat",	"♭"},
	{"gimel",	"ℷ"},
	{"gscr",	"ℊ"},
	{"gt",		">"},
	{"hamlit",	"ℋ"},
	{"hearts",	"♥"},
	{"ii",		"ⅈ"},
	{"iiota",	"℩"},
	{"image",	"ℑ"},
	{"incare",	"℅"},
	{"lbbrk",	"❲"},
	{"lscr",	"ℐ"},
	{"lt",		"<"},
	{"male",	"♂"},
	{"malt",	"✠"},
	{"mho",		"℧"},
	{"natural",	"♮"},
	{"nbsp",	"\xC2\xA0"},
	{"numero",	"№"},
	{"oscr",	"ℴ"},
	{"para",	"¶"},
	{"phone",	"☎"},
	{"planck",	"ℏ"},
	{"planckh",	"ℎ"},
	{"pound",	"£"},
	{"quot",	"\""},
	{"rbbrk",	"❳"},
	{"real",	"ℜ"},
	{"reg",		"®"},
	{"rx",		"℞"},
	{"sect",	"§"},
	{"sext",	"✶"},
	{"sharp",	"♯"},
	{"spades",	"♠"},
	{"star",	"☆"},
	{"starf",	"★"},
	{"sung",	"♪"},
	{"trade",	"™"},
	//{"VerticalSeparator",	"❘"},	// TODO: Special check for this one?
	{"weierp",	"℘"},
	{"yen",		"¥"},

	// end of table
	{"", ""}
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

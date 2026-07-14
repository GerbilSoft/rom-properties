/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * html_entities.hpp: HTML entities table.                                 *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
#include <array>

namespace LibRpBase {

// HTML entities, sorted by entity name.
// Can be used with e.g. bsearch() or std::lower_bound().
// References:
// - https://www.w3schools.com/HTML/html_entities.asp
// - https://www.toptal.com/designers/htmlarrows/symbols/
struct html_entity_tbl_t {
	char entity[8];	// HTML entity, minus '&' and ';'
	char value[4];	// Actual text value
};

static const std::array<html_entity_tbl_t, 77> html_entity_tbl = {{
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
}};

} // namespace LibRpBase

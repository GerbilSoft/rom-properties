/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * html_entities.hpp: HTML entities handling.                              *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
// NOTE: gcc-5 on Ubuntu 16.04 is missing <cuchar>...
#include <uchar.h>	// for char16_t

// C includes (C++ namespace)
#include <cstddef>	// for size_t

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
 * Get the HTML entities table.
 * Table is terminated with an empty-string entry.
 *
 * @return HTML entities table
 */
RP_LIBROMDATA_PUBLIC
const html_entity_tbl_t *get_table(void);

/**
 * Get the number of entries in the HTML entities table.
 * NOTE: This does *not* include the empty-string terminator entry.
 *
 * @return Number of entries in the HTML entities table
 */
RP_LIBROMDATA_PUBLIC
size_t get_count(void);

}} // namespace LibRpText::HtmlEntities

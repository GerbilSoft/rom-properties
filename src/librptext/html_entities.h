/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * html_entities.h: HTML entities table.                                   *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HTML entities, sorted by entity name.
 * Can be used with e.g. bsearch() or std::lower_bound().
 * References:
 * - https://www.w3schools.com/HTML/html_entities.asp
 * - https://www.toptal.com/designers/htmlarrows/symbols/
 */
typedef struct _html_entity_tbl_t {
	char entity[8];	// HTML entity, minus '&' and ';'
	char value[4];	// Actual text value
} html_entity_tbl_t;

/**
 * Get the HTML entities table.
 * Table is terminated with an empty-string entry.
 *
 * @return HTML entities table
 */
RP_LIBROMDATA_PUBLIC
const html_entity_tbl_t *rp_get_html_entities_table(void);

/**
 * Get the number of entries in the HTML entities table.
 * NOTE: This does *not* include the empty-string terminator entry.
 *
 * @return Number of entries in the HTML entities table
 */
RP_LIBROMDATA_PUBLIC
size_t rp_get_html_entities_table_count(void);

#ifdef __cplusplus
}
#endif

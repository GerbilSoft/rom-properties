/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * sort_funcs_common.c: Common sort functions.                             *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "sort_funcs_common.h"

/**
 * String sorting function for COLSORT_STANDARD (case-sensitive).
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_standard(const char *strA, const char *strB)
{
	// Do a case-sensitive string comparison.
	return g_strcmp0(strA, strB);
}

/**
 * String sorting function for COLSORT_NOCASE (case-insensitive).
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_nocase(const char *strA, const char *strB)
{
	gint ret = 0;

	if (!strA || !strB) {
		if (!strA && !strB) {
			// Both strings are NULL.
			// Handle this as if they're equal.
		} else {
			// Only one string is NULL.
			// That will be sorted before the other string.
			ret = (!strA ? -1 : 1);
		}
	} else {
		// Casefold the strings, then use glib's string collation function.
		// TODO: Maybe precompute collation values with g_utf8_collate_key()?
		gchar *strCF_A = g_utf8_casefold(strA, -1);
		gchar *strCF_B = g_utf8_casefold(strB, -1);
		ret = g_utf8_collate(strCF_A, strCF_B);
		g_free(strCF_A);
		g_free(strCF_B);
	}

	return ret;
}

/**
 * String sorting function for COLSORT_NUMERIC.
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_numeric(const char *strA, const char *strB)
{
	gint ret = 0;

	// Handle NULL strings as if they're 0.
	// TODO: Allow arbitrary bases?
	gchar *endptrA = (gchar*)"", *endptrB = (gchar*)"";
	int64_t valA = (strA ? g_ascii_strtoll(strA, &endptrA, 10) : 0);
	int64_t valB = (strB ? g_ascii_strtoll(strB, &endptrB, 10) : 0);

	// If the values match, do a case-insensitive string comparison
	// if the strings didn't fully convert to numbers.
	if (valA == valB) {
		if (*endptrA == '\0' && *endptrB == '\0') {
			// Both strings are numbers.
			// No need to do a string comparison.
		} else if (!strA || !strB) {
			if (!strA && !strB) {
				// Both strings are NULL.
				// Handle this as if they're equal.
			} else {
				// Only one string is NULL.
				// That will be sorted before the other string.
				ret = (!strA ? -1 : 1);
			}
		} else {
			// Casefold the strings, then use glib's string collation function.
			// TODO: Maybe precompute collation values with g_utf8_collate_key()?
			gchar *strCF_A = g_utf8_casefold(strA, -1);
			gchar *strCF_B = g_utf8_casefold(strB, -1);
			ret = g_utf8_collate(strCF_A, strCF_B);
			g_free(strCF_A);
			g_free(strCF_B);
		}
	} else if (valA < valB) {
		ret = -1;
	} else /*if (valA > valB)*/ {
		ret = 1;
	}

	return ret;
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * sort_funcs_gtk4.h: GCompareDataFunc sort functions.                     *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "sort_funcs.h"
#include "ListDataItem.h"

/**
 * RFT_LISTDATA sorting function for COLSORT_STANDARD (case-sensitive).
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_standard(gconstpointer a, gconstpointer b, gpointer userdata)
{
	const int column = GPOINTER_TO_INT(userdata);

	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a case-sensitive string comparison.
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

	return g_strcmp0(strA, strB);
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NOCASE (case-insensitive).
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_nocase(gconstpointer a, gconstpointer b, gpointer userdata)
{
	gint ret = 0;
	const int column = GPOINTER_TO_INT(userdata);

	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a case-insensitive string comparison.
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

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
 * RFT_LISTDATA sorting function for COLSORT_NUMERIC.
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint sort_RFT_LISTDATA_numeric(gconstpointer a, gconstpointer b, gpointer userdata)
{
	gint ret = 0;
	const int column = GPOINTER_TO_INT(userdata);

	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a numeric comparison.
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

	// Handle NULL strings as if they're 0.
	// TODO: Allow arbitrary bases?
	gchar *endptrA = (gchar*)"", *endptrB = (gchar*)"";
	gint64 valA = (strA ? g_ascii_strtoll(strA, &endptrA, 10) : 0);
	gint64 valB = (strB ? g_ascii_strtoll(strB, &endptrB, 10) : 0);

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

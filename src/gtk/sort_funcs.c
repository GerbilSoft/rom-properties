/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * sort_funcs.h: GtkTreeSortable sort functions.                           *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "sort_funcs.h"

/**
 * RFT_LISTDATA sorting function for COLSORT_NOCASE (case-insensitive).
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_nocase(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	gint ret = 0;
	gchar *strA, *strB;

	// Get the text and do a case-insensitive string comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &strA, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &strB, -1);
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
		// Use glib's string collation function.
		// TODO: Maybe precompute collation values with g_utf8_collate_key()?
		ret = g_utf8_collate(strA, strB);
	}

	g_free(strA);
	g_free(strB);
	return ret;
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NUMERIC.
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint sort_RFT_LISTDATA_numeric(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	gint ret = 0;

	gchar *strA, *strB;
	gint64 valA, valB;
	gchar *endptrA = (gchar*)"", *endptrB = (gchar*)"";

	// Get the text and do a numeric comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &strA, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &strB, -1);

	// Handle NULL strings as if they're 0.
	// TODO: Allow arbitrary bases?
	valA = (strA ? g_ascii_strtoll(strA, &endptrA, 10) : 0);
	valB = (strB ? g_ascii_strtoll(strB, &endptrB, 10) : 0);

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
			// Use glib's string collation function.
			// TODO: Maybe precompute collation values with g_utf8_collate_key()?
			ret = g_utf8_collate(strA, strB);
		}
	} else if (valA < valB) {
		ret = -1;
	} else /*if (valA > valB)*/ {
		ret = 1;
	}

	g_free(strA);
	g_free(strB);
	return ret;
}

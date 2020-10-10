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
gint sort_RFT_LISTDATA_nocase(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, G_GNUC_UNUSED gpointer userdata)
{
	gint ret = 0;
	gchar *text1, *text2;

	// Get the text and do a case-insensitive string comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &text1, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &text2, -1);
        if (!text1 || !text2) {
		if (!text1 && !text2) {
			// Both strings are NULL.
			// Handle this as if they're equal.
		} else {
			// Only one string is NULL.
			// That will be sorted before the other string.
			ret = (!text1 ? -1 : 1);
		}
	} else {
		// Use glib's string collation function.
		// TODO: Maybe precompute collation values with g_utf8_collate_key()?
		ret = g_utf8_collate(text1, text2);
	}

	g_free(text1);
	g_free(text2);
	return ret;
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NUMERIC.
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_numeric(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, G_GNUC_UNUSED gpointer userdata)
{
	gint ret = 0;

	gchar *text1, *text2;
	gint64 val1, val2;
	gchar *endptr1 = (gchar*)"", *endptr2 = (gchar*)"";

	// Get the text and do a numeric comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &text1, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &text2, -1);

	// Handle NULL strings as if they're 0.
	// TODO: Allow arbitrary bases?
	val1 = (text1 ? g_ascii_strtoll(text1, &endptr1, 10) : 0);
	val2 = (text2 ? g_ascii_strtoll(text2, &endptr2, 10) : 0);

	// If the values match, do a case-insensitive string comparison
	// if the strings didn't fully convert to numbers.
	if (val1 == val2) {
		if (*endptr1 == '\0' && *endptr2 == '\0') {
			// Both strings are numbers.
			// No need to do a string comparison.
		} else if (!text1 || !text2) {
			if (!text1 && !text2) {
				// Both strings are NULL.
				// Handle this as if they're equal.
			} else {
				// Only one string is NULL.
				// That will be sorted before the other string.
				ret = (!text1 ? -1 : 1);
			}
		} else {
			// Use glib's string collation function.
			// TODO: Maybe precompute collation values with g_utf8_collate_key()?
			ret = g_utf8_collate(text1, text2);
		}
	} else if (val1 < val2) {
		ret = -1;
	} else /*if (val1 > val2)*/ {
		ret = 1;
	}

	g_free(text1);
	g_free(text2);
	return ret;
}

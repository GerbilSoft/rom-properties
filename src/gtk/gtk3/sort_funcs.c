/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * sort_funcs.c: GCompareDataFunc sort functions.                          *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "sort_funcs.h"
#include "../sort_funcs_common.h"

/**
 * RFT_LISTDATA sorting function for COLSORT_STANDARD (case-sensitive).
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_RFT_LISTDATA_standard(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	// Get the text and do a case-sensitive string comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gchar *strA, *strB;
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &strA, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &strB, -1);

	gint ret = rp_sort_string_standard(strA, strB);

	g_free(strA);
	g_free(strB);
	return ret;
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NOCASE (case-insensitive).
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint rp_sort_RFT_LISTDATA_nocase(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	// Get the text and do a case-insensitive string comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gchar *strA, *strB;
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &strA, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &strB, -1);

	gint ret = rp_sort_string_nocase(strA, strB);

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
gint rp_sort_RFT_LISTDATA_numeric(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	// Get the text and do a numeric comparison.
	// Reference: https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Sorting
	gchar *strA, *strB;
	gtk_tree_model_get(model, a, GPOINTER_TO_INT(userdata), &strA, -1);
	gtk_tree_model_get(model, b, GPOINTER_TO_INT(userdata), &strB, -1);

	gint ret = rp_sort_string_numeric(strA, strB);

	g_free(strA);
	g_free(strB);
	return ret;
}

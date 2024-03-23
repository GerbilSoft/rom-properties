/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * sort_funcs_gtk4.h: GCompareDataFunc sort functions.                     *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "sort_funcs.h"
#include "../sort_funcs_common.h"

#include "ListDataItem.h"

/**
 * RFT_LISTDATA sorting function for COLSORT_STANDARD (case-sensitive).
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_RFT_LISTDATA_standard(gconstpointer a, gconstpointer b, gpointer userdata)
{
	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a case-sensitive string comparison.
	const int column = GPOINTER_TO_INT(userdata);
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

	return rp_sort_string_standard(strA, strB);
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NOCASE (case-insensitive).
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_RFT_LISTDATA_nocase(gconstpointer a, gconstpointer b, gpointer userdata)
{
	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a case-insensitive string comparison.
	const int column = GPOINTER_TO_INT(userdata);
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

	return rp_sort_string_nocase(strA, strB);
}

/**
 * RFT_LISTDATA sorting function for COLSORT_NUMERIC.
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_RFT_LISTDATA_numeric(gconstpointer a, gconstpointer b, gpointer userdata)
{
	RpListDataItem *const ldiA = RP_LIST_DATA_ITEM((gpointer)a);
	RpListDataItem *const ldiB = RP_LIST_DATA_ITEM((gpointer)b);

	// Get the text and do a numeric comparison.
	const int column = GPOINTER_TO_INT(userdata);
	const char *const strA = rp_list_data_item_get_column_text(ldiA, column);
	const char *const strB = rp_list_data_item_get_column_text(ldiB, column);

	return rp_sort_string_numeric(strA, strB);
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * sort_funcs.h: GtkTreeSortable sort functions.                           *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_SORT_FUNCS_H__
#define __ROMPROPERTIES_GTK_SORT_FUNCS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * RFT_LISTDATA sorting function for COLSORT_NOCASE (case-insensitive).
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_nocase(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata);

/**
 * RFT_LISTDATA sorting function for COLSORT_NUMERIC.
 * @param model
 * @param a
 * @param b
 * @param userdata Column ID
 * @return -1, 0, or 1.
 */
gint sort_RFT_LISTDATA_numeric(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_SORT_FUNCS_H__ */

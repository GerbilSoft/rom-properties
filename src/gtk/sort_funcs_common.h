/***************************************************************************
 * ROM Properties Page shell extension. (GTK)                              *
 * sort_funcs_common.h: Common sort functions.                             *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * String sorting function for COLSORT_STANDARD (case-sensitive).
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_standard(const char *strA, const char *strB);

/**
 * String sorting function for COLSORT_NOCASE (case-insensitive).
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_nocase(const char *strA, const char *strB);

/**
 * String sorting function for COLSORT_NUMERIC.
 * @param strA
 * @param strB
 * @return -1, 0, or 1 (like strcmp())
 */
gint rp_sort_string_numeric(const char *strA, const char *strB);

G_END_DECLS

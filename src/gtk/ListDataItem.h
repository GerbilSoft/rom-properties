/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ListDataItem.h: RFT_LISTDATA item                                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"
#include "PIMGTYPE.hpp"

G_BEGIN_DECLS

#define RP_TYPE_LIST_DATA_ITEM (rp_list_data_item_get_type())
G_DECLARE_FINAL_TYPE(RpListDataItem, rp_list_data_item, RP, LIST_DATA_ITEM, GObject)

/** RpListDataItemCol0Type: Column 0 type **/
typedef enum {
	RP_LIST_DATA_ITEM_COL0_TYPE_TEXT	= 0,	/*< nick="Text only" >*/
	RP_LIST_DATA_ITEM_COL0_TYPE_CHECKBOX	= 1,	/*< nick="Column 0 is a checkbox" >*/
	RP_LIST_DATA_ITEM_COL0_TYPE_ICON	= 2,	/*< nick="Column 1 is an icon" >*/

	RP_LIST_DATA_ITEM_COL0_TYPE_LAST
} RpListDataItemCol0Type;

RpListDataItem *rp_list_data_item_new			(int column_count, RpListDataItemCol0Type col0_type) G_GNUC_MALLOC;

RpListDataItemCol0Type	rp_list_data_item_get_col0_type	(RpListDataItem *item);

void		rp_list_data_item_set_icon		(RpListDataItem *item, PIMGTYPE icon);
PIMGTYPE	rp_list_data_item_get_icon		(RpListDataItem *item);

void		rp_list_data_item_set_checked		(RpListDataItem *item, gboolean checked);
gboolean	rp_list_data_item_get_checked		(RpListDataItem *item);

int		rp_list_data_item_get_column_count	(RpListDataItem *item);

void		rp_list_data_item_set_column_text	(RpListDataItem *item, int column, const char *text);
const char*	rp_list_data_item_get_column_text	(RpListDataItem *item, int column);	// owned by this object

G_END_DECLS

/***************************************************************************
 * ROM Properties Page shell extension. (GTK4)                             *
 * XAttrViewitem.h: XAttrView item for GtkColumnView                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"
#include "PIMGTYPE.hpp"

#if !GTK_CHECK_VERSION(4,0,0)
#  error XAttrViewItem requires GTK4 or later
#endif /* !GTK_CHECK_VERSION(4,0,0) */

G_BEGIN_DECLS

#define RP_TYPE_XATTRVIEW_ITEM (rp_xattrview_item_get_type())
G_DECLARE_FINAL_TYPE(RpXAttrViewItem, rp_xattrview_item, RP, XATTRVIEW_ITEM, GObject)

RpXAttrViewItem *rp_xattrview_item_new		(const char *name, const char *value) G_GNUC_MALLOC;

void		rp_xattrview_item_set_name	(RpXAttrViewItem *item, const char *name);
const char*	rp_xattrview_item_get_name	(RpXAttrViewItem *item);

void		rp_xattrview_item_set_value	(RpXAttrViewItem *item, const char *value);
const char*	rp_xattrview_item_get_value	(RpXAttrViewItem *item);

G_END_DECLS

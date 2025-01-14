/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XfsAttrView.h: XFS file system attribute viewer widget.                 *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_XFS_ATTR_VIEW (rp_xfs_attr_view_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpXfsAttrView, rp_xfs_attr_view, RP, XFS_ATTR_VIEW, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpXfsAttrView, rp_xfs_attr_view, RP, XFS_ATTR_VIEW, GtkVBox)
#endif

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_xfs_attr_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_xfs_attr_view_new			(void) G_GNUC_MALLOC;

void		rp_xfs_attr_view_set_xflags		(RpXfsAttrView *widget, guint32 xflags);
guint32		rp_xfs_attr_view_get_xflags		(RpXfsAttrView *widget);
void		rp_xfs_attr_view_clear_xflags		(RpXfsAttrView *widget);

void		rp_xfs_attr_view_set_project_id		(RpXfsAttrView *widget, guint32 project_id);
guint32		rp_xfs_attr_view_get_project_id		(RpXfsAttrView *widget);
void		rp_xfs_attr_view_clear_project_id	(RpXfsAttrView *widget);

G_END_DECLS

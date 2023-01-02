/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LinuxAttrView.h: Linux file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_LINUX_ATTR_VIEW (rp_linux_attr_view_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpLinuxAttrView, rp_linux_attr_view, RP, LINUX_ATTR_VIEW, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpLinuxAttrView, rp_linux_attr_view, RP, LINUX_ATTR_VIEW, GtkHBox)
#endif

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_linux_attr_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_linux_attr_view_new			(void) G_GNUC_MALLOC;

void		rp_linux_attr_view_set_flags		(RpLinuxAttrView *widget, int flags);
int 		rp_linux_attr_view_get_flags		(RpLinuxAttrView *widget);
void		rp_linux_attr_view_clear_flags		(RpLinuxAttrView *widget);

G_END_DECLS

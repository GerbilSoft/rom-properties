/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DosAttrView.h: MS-DOS file system attribute viewer widget.              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_XATTR_DOSATTRVIEW_H__
#define __ROMPROPERTIES_GTK_XATTR_DOSATTRVIEW_H__

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_DOS_ATTR_VIEW (rp_dos_attr_view_get_type())
#if GTK_CHECK_VERSION(3,0,0)
G_DECLARE_FINAL_TYPE(RpDosAttrView, rp_dos_attr_view, RP, DOS_ATTR_VIEW, GtkBox)
#else /* !GTK_CHECK_VERSION(3,0,0) */
G_DECLARE_FINAL_TYPE(RpDosAttrView, rp_dos_attr_view, RP, DOS_ATTR_VIEW, GtkHBox)
#endif

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_dos_attr_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_dos_attr_view_new		(void) G_GNUC_MALLOC;

void		rp_dos_attr_view_set_attrs	(RpDosAttrView *widget, unsigned int attrs);
unsigned int 	rp_dos_attr_view_get_attrs	(RpDosAttrView *widget);
void		rp_dos_attr_view_clear_attrs	(RpDosAttrView *widget);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_XATTR_DOSATTRVIEW_H__ */

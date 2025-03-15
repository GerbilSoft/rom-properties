/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DosAttrView.h: MS-DOS file system attribute viewer widget.              *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_DOS_ATTR_VIEW (rp_dos_attr_view_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpDosAttrView_super		GtkBox
#  define _RpDosAttrView_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpDosAttrView_super		GtkVBox
#  define _RpDosAttrView_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpDosAttrView, rp_dos_attr_view, RP, DOS_ATTR_VIEW, _RpDosAttrView_super)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_dos_attr_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_dos_attr_view_new		(void) G_GNUC_MALLOC;

void		rp_dos_attr_view_set_attrs	(RpDosAttrView *widget, unsigned int attrs);
unsigned int 	rp_dos_attr_view_get_attrs	(RpDosAttrView *widget);
void		rp_dos_attr_view_clear_attrs	(RpDosAttrView *widget);

void		rp_dos_attr_view_set_valid_attrs	(RpDosAttrView *widget, unsigned int validAttrs);
unsigned int 	rp_dos_attr_view_get_valid_attrs	(RpDosAttrView *widget);
void		rp_dos_attr_view_clear_valid_attrs	(RpDosAttrView *widget);

void		rp_dos_attr_view_set_current_and_valid_attrs	(RpDosAttrView *widget, unsigned int attrs, unsigned int validAttrs);

G_END_DECLS

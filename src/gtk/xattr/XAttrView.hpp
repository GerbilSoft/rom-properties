/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * XAttrView.hpp: Extended attribute viewer property page.                 *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_XATTR_VIEW (rp_xattr_view_get_type())

#if GTK_CHECK_VERSION(3, 0, 0)
#  define _RpXAttrView_super		GtkBox
#  define _RpXAttrView_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
#  define _RpXAttrView_super		GtkVBox
#  define _RpXAttrView_superClass	GtkVBoxClass
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

G_DECLARE_FINAL_TYPE(RpXAttrView, rp_xattr_view, RP, XATTR_VIEW, _RpXAttrView_super)

/* this function is implemented automatically by the G_DEFINE_TYPE macro */
void		rp_xattr_view_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*rp_xattr_view_new		(const gchar *uri) G_GNUC_MALLOC;

void		rp_xattr_view_set_uri		(RpXAttrView *widget, const gchar *uri);
const gchar	*rp_xattr_view_get_uri		(RpXAttrView *widget);

/**
 * Are attributes loaded from the current URI?
 * @param widget RpXAttrView
 * @return True if we have attributes; false if not.
 */
gboolean	rp_xattr_view_has_attributes	(RpXAttrView *widget);

G_END_DECLS

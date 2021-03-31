/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * gtk2-compat.h: GTK+ compatibility functions.                        *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_GTK2_COMPAT_H__
#define __ROMPROPERTIES_GTK_GTK2_COMPAT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#if !GTK_CHECK_VERSION(3,0,0)
static inline GtkWidget*
gtk_tree_view_column_get_button(GtkTreeViewColumn *tree_column)
{
	// Reference: https://github.com/kynesim/wireshark/blob/master/ui/gtk/old-gtk-compat.h
	g_return_val_if_fail(tree_column != NULL, NULL);

	/* This is too late, see https://bugzilla.gnome.org/show_bug.cgi?id=641089
	 * According to
	 * http://ftp.acc.umu.se/pub/GNOME/sources/gtk+/2.13/gtk+-2.13.4.changes
	 * access to the button element was sealed during 2.13. They also admit that
	 * they missed a use case and thus failed to provide an accessor function:
	 * http://mail.gnome.org/archives/commits-list/2010-December/msg00578.html
	 * An accessor function was finally added in 3.0.
	 */
#  if (GTK_CHECK_VERSION(2,14,0) && defined(GSEAL_ENABLE))
	return tree_column->_g_sealed__button;
#  else
	return tree_column->button;
#  endif
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */

#if !GTK_CHECK_VERSION(3,2,0)
static inline gboolean
gdk_event_get_button(const GdkEvent *event, guint *button)
{
	// from GTK+ source
	gboolean fetched = TRUE;
	guint number = 0;

	g_return_val_if_fail (event != NULL, FALSE);

	switch ((guint) event->any.type)
	{
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			number = event->button.button;
			break;
		default:
			fetched = FALSE;
			break;
	}

	if (button)
		*button = number;

	return fetched;
}
#endif /* !GTK_CHECK_VERSION(3,2,0) */

#if !GTK_CHECK_VERSION(3,10,0)
static inline GdkEventType
gdk_event_get_event_type (const GdkEvent *event)
{
	g_return_val_if_fail(event != NULL, GDK_NOTHING);

	return event->any.type;
}
#endif /* !GTK_CHECK_VERSION(3,10,0) */

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_GTK2_COMPAT_H__ */
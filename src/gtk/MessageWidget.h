/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.h: Message widget (similar to KMessageWidget)             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_MESSAGE_WIDGET (rp_message_widget_get_type())

#if GTK_CHECK_VERSION(4,0,0)
#  define _RpMessageWidget_super	GtkBox
#  define _RpMessageWidget_superClass	GtkBoxClass
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  define _RpMessageWidget_super	GtkEventBox
#  define _RpMessageWidget_superClass	GtkEventBoxClass
#endif /* GTK_CHECK_VERSION(4,0,0) */

G_DECLARE_FINAL_TYPE(RpMessageWidget, rp_message_widget, RP, MESSAGE_WIDGET, _RpMessageWidget_super)

GtkWidget	*rp_message_widget_new		(void) G_GNUC_MALLOC;

void		rp_message_widget_set_text	(RpMessageWidget *widget, const gchar *str);
const gchar*	rp_message_widget_get_text	(RpMessageWidget *widget);

void		rp_message_widget_set_message_type	(RpMessageWidget *widget, GtkMessageType messageType);
GtkMessageType	rp_message_widget_get_message_type	(RpMessageWidget *widget);

gboolean	rp_message_widget_get_child_revealed	(RpMessageWidget *widget);

void		rp_message_widget_set_reveal_child	(RpMessageWidget *widget, gboolean reveal_child);
gboolean	rp_message_widget_get_reveal_child	(RpMessageWidget *widget);

void		rp_message_widget_set_transition_duration(RpMessageWidget *widget, guint duration);
guint		rp_message_widget_get_transition_duration(RpMessageWidget *widget);

#if !GTK_CHECK_VERSION(3,9,0)
// GtkRevealer is not available. Define GtkRevealerTransitionType here.
typedef enum {
	GTK_REVEALER_TRANSITION_TYPE_NONE,
	GTK_REVEALER_TRANSITION_TYPE_CROSSFADE,
	GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT,
	GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT,
	GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP,
	GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
} GtkRevealerTransitionType;
#endif /* !GTK_CHECK_VERSION(3,9,0) */

void				rp_message_widget_set_transition_type	(RpMessageWidget *widget, GtkRevealerTransitionType transition);
GtkRevealerTransitionType	rp_message_widget_get_transition_type	(RpMessageWidget *widget);

G_END_DECLS

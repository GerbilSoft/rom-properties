/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.h: Message widget (similar to KMessageWidget)             *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "gtk-compat.h"

G_BEGIN_DECLS

#define RP_TYPE_MESSAGE_WIDGET (rp_message_widget_get_type())
#if GTK_CHECK_VERSION(4,0,0)
G_DECLARE_FINAL_TYPE(RpMessageWidget, rp_message_widget, RP, MESSAGE_WIDGET, GtkBox)
#else /* !GTK_CHECK_VERSION(4,0,0) */
G_DECLARE_FINAL_TYPE(RpMessageWidget, rp_message_widget, RP, MESSAGE_WIDGET, GtkEventBox)
#endif /* GTK_CHECK_VERSION(4,0,0) */

GtkWidget	*rp_message_widget_new		(void) G_GNUC_MALLOC;

void		rp_message_widget_set_text	(RpMessageWidget *widget, const gchar *str);
const gchar*	rp_message_widget_get_text	(RpMessageWidget *widget);

void		rp_message_widget_set_message_type	(RpMessageWidget *widget, GtkMessageType messageType);
GtkMessageType	rp_message_widget_get_message_type	(RpMessageWidget *widget);

void		rp_message_widget_set_reveal_child	(RpMessageWidget *widget, gboolean reveal_child);
gboolean	rp_message_widget_get_reveal_child	(RpMessageWidget *widget);
gboolean	rp_message_widget_get_child_revealed	(RpMessageWidget *widget);

G_END_DECLS

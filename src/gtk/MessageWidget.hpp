/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.hpp: Message widget. (Similar to KMessageWidget)          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_MESSAGEWIDGET_HPP__
#define __ROMPROPERTIES_GTK_MESSAGEWIDGET_HPP__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _MessageWidgetClass	MessageWidgetClass;
typedef struct _MessageWidget	MessageWidget;

#define TYPE_MESSAGE_WIDGET            (message_widget_get_type())
#define MESSAGE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_MESSAGE_WIDGET, MessageWidget))
#define MESSAGE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_MESSAGE_WIDGET, MessageWidgetClass))
#define IS_MESSAGE_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_MESSAGE_WIDGET))
#define IS_MESSAGE_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_MESSAGE_WIDGET))
#define MESSAGE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_MESSAGE_WIDGET, MessageWidgetClass))

/* these two functions are implemented automatically by the G_DEFINE_TYPE macro */
GType		message_widget_get_type		(void) G_GNUC_CONST G_GNUC_INTERNAL;
void		message_widget_register_type	(GtkWidget *widget) G_GNUC_INTERNAL;

GtkWidget	*message_widget_new		(void) G_GNUC_MALLOC;

void		message_widget_set_text		(MessageWidget *widget, const gchar *str);
const gchar*	message_widget_get_text		(MessageWidget *widget);

void		message_widget_set_message_type	(MessageWidget *widget, GtkMessageType messageType);
GtkMessageType	message_widget_get_message_type	(MessageWidget *widget);

void		message_widget_show_with_timeout(MessageWidget *widget);

G_END_DECLS

#endif /* __ROMPROPERTIES_GTK_MESSAGEWIDGET_HPP__ */

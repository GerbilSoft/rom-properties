/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.hpp: Message widget. (Similar to KMessageWidget)          *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageWidget.hpp"

/* Property identifiers */
typedef enum {
	PROP_0,
	PROP_TEXT,
	PROP_MESSAGE_TYPE,
	PROP_LAST
} MessageWidgetPropID;
static GParamSpec *properties[PROP_LAST];

static void	message_widget_dispose	(GObject	*object);
static void	message_widget_finalize	(GObject	*object);

static void	message_widget_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	message_widget_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

/** Signal handlers. **/
static void	closeButton_clicked_handler	(GtkButton	*button,
						 gpointer	 user_data);

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// MessageWidget class.
struct _MessageWidgetClass {
	superclass __parent__;
};

// MessageWidget instance.
struct _MessageWidget {
	super __parent__;

	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *closeButton;

	GtkMessageType messageType;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(MessageWidget, message_widget,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
message_widget_class_init(MessageWidgetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = message_widget_dispose;
	gobject_class->finalize = message_widget_finalize;
	gobject_class->get_property = message_widget_get_property;
	gobject_class->set_property = message_widget_set_property;

	/** Properties **/

	properties[PROP_TEXT] = g_param_spec_string(
		"text", "Text", "Text displayed on the MessageWidget.",
		nullptr,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	properties[PROP_MESSAGE_TYPE] = g_param_spec_enum(
		"message-type", "Message Type", "Message type.",
		GTK_TYPE_MESSAGE_TYPE, GTK_MESSAGE_OTHER,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, properties);
}

static void
message_widget_init(MessageWidget *widget)
{
	widget->messageType = GTK_MESSAGE_OTHER;

	widget->image = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(widget), widget->image, FALSE, FALSE, 4);

	widget->label = gtk_label_new(nullptr);
	gtk_widget_show(widget->label);
	gtk_box_pack_start(GTK_BOX(widget), widget->label, FALSE, FALSE, 0);

	// TODO: Prpoer alignment.

	widget->closeButton = gtk_button_new();
	GtkWidget *const imageClose = gtk_image_new_from_icon_name("dialog-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget->closeButton), imageClose);
	gtk_button_set_relief(GTK_BUTTON(widget->closeButton), GTK_RELIEF_NONE);
	gtk_widget_show(widget->closeButton);
	gtk_box_pack_end(GTK_BOX(widget), widget->closeButton, FALSE, FALSE, 0);

	g_signal_connect(widget->closeButton, "clicked",
		G_CALLBACK(closeButton_clicked_handler), widget);
}

static void
message_widget_dispose(GObject *object)
{
	//MessageWidget *const widget = MESSAGE_WIDGET(object);

	// Nothing to do here right now...

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(message_widget_parent_class)->dispose(object);
}

static void
message_widget_finalize(GObject *object)
{
	//MessageWidget *const widget = MESSAGE_WIDGET(object);

	// Nothing to do here right now...

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(message_widget_parent_class)->finalize(object);
}

GtkWidget*
message_widget_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_MESSAGE_WIDGET, nullptr));
}

/** Properties **/

static void
message_widget_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	MessageWidget *const widget = MESSAGE_WIDGET(object);

	switch (prop_id) {
		case PROP_TEXT:
			g_value_set_string(value, gtk_label_get_text(GTK_LABEL(widget->label)));
			break;

		case PROP_MESSAGE_TYPE:
			g_value_set_enum(value, widget->messageType);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
message_widget_set_property(GObject	*object,
			   guint	 prop_id,
			   const GValue	*value,
			   GParamSpec	*pspec)
{
	MessageWidget *const widget = MESSAGE_WIDGET(object);

	switch (prop_id) {
		case PROP_TEXT:
			gtk_label_set_text(GTK_LABEL(widget->label), g_value_get_string(value));
			break;

		case PROP_MESSAGE_TYPE:
			message_widget_set_message_type(widget, static_cast<GtkMessageType>(g_value_get_enum(value)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

void
message_widget_set_text(MessageWidget *widget, const gchar *str)
{
	gtk_label_set_text(GTK_LABEL(widget->label), str);
}

const gchar*
message_widget_get_text(MessageWidget *widget)
{
	return gtk_label_get_text(GTK_LABEL(widget->label));
}

void
message_widget_set_message_type(MessageWidget *widget, GtkMessageType messageType)
{
	if (widget->messageType == messageType)
		return;

	widget->messageType = messageType;

	// Update the icon.
	// TODO: Background color?
	bool show;
	const char *icon_name;
	switch (messageType) {
		default:
		case GTK_MESSAGE_OTHER:
			show = false;
			icon_name = nullptr;
			break;
		case GTK_MESSAGE_INFO:
			show = true;
			icon_name = "dialog-information";
			break;
		case GTK_MESSAGE_WARNING:
			show = true;
			icon_name = "dialog-warning";
			break;
		case GTK_MESSAGE_QUESTION:
			// TODO: Make sure dialog-question exists.
			show = true;
			icon_name = "dialog-question";
			break;
		case GTK_MESSAGE_ERROR:
			show = true;
			icon_name = "dialog-error";
			break;
	}

	gtk_widget_set_visible(widget->image, show);
	if (show) {
		// TODO: Better icon size?
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->image), icon_name, GTK_ICON_SIZE_BUTTON);
	}
}

GtkMessageType
message_widget_get_message_type(MessageWidget *widget)
{
	return widget->messageType;
}

/** Signal handlers **/

/**
 * The Close button was clicked.
 * @param button Close button
 * @param user_data MessageWidget*
 */
static void
closeButton_clicked_handler(GtkButton	*button,
			    gpointer	 user_data)
{
	// TODO: Animation like KMessageWidget.
	RP_UNUSED(button);
	gtk_widget_hide(GTK_WIDGET(user_data));
}

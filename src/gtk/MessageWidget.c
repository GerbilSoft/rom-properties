/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.c: Message widget (similar to KMessageWidget)             *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageWidget.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_TEXT,
	PROP_MESSAGE_TYPE,

	PROP_LAST
} MessageWidgetPropID;

static void	rp_message_widget_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rp_message_widget_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);

/** Signal handlers. **/
static void	rp_message_widget_close_button_clicked_handler	(GtkButton		*button,
								 RpMessageWidget	*widget);

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3,0,0) */
// Top class is GtkEventBox, since we can't
// set the background color of GtkHBox.
typedef GtkEventBoxClass superclass;
typedef GtkEventBox super;
#define GTK_TYPE_SUPER GTK_TYPE_EVENT_BOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

static GParamSpec *props[PROP_LAST];

// MessageWidget class
struct _RpMessageWidgetClass {
	superclass __parent__;
};

// MessageWidget instance
struct _RpMessageWidget {
	super __parent__;

#if !GTK_CHECK_VERSION(3,0,0)
	GtkWidget *evbox_inner;
	GtkWidget *hbox;
#endif /* !GTK_CHECK_VERSION(3,0,0) */

	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *close_button;

	GtkMessageType messageType;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpMessageWidget, rp_message_widget,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_message_widget_class_init(RpMessageWidgetClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_message_widget_set_property;
	gobject_class->get_property = rp_message_widget_get_property;

	/** Properties **/

	props[PROP_TEXT] = g_param_spec_string(
		"text", "Text", "Text displayed on the MessageWidget.",
		NULL,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_MESSAGE_TYPE] = g_param_spec_enum(
		"message-type", "Message Type", "Message type.",
		GTK_TYPE_MESSAGE_TYPE, GTK_MESSAGE_OTHER,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

#if GTK_CHECK_VERSION(3,0,0)
	// Initialize MessageWidget CSS.
	GtkCssProvider *const provider = gtk_css_provider_new();
	GdkDisplay *const display = gdk_display_get_default();
#  if GTK_CHECK_VERSION(4,0,0)
	// GdkScreen no longer exists in GTK4.
	// Style context providers are added directly to GdkDisplay instead.
	gtk_style_context_add_provider_for_display(display,
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	GdkScreen *const screen = gdk_display_get_default_screen(display);
	gtk_style_context_add_provider_for_screen(screen,
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#  endif /* !GTK_CHECK_VERSION(4,0,0) */

	static const char css_MessageWidget[] =
		"@define-color gsrp_color_info rgb(61,174,233);\n"
		"@define-color gsrp_color_warning rgb(246,116,0);\n"
		"@define-color gsrp_color_error rgb(218,68,83);\n"
		".gsrp_msgw_info {\n"
		"\tbackground-color: lighter(@gsrp_color_info);\n"
		"\tborder: 2px solid @gsrp_color_info;\n"
		"}\n"
		".gsrp_msgw_warning {\n"
		"\tbackground-color: lighter(@gsrp_color_warning);\n"
		"\tborder: 2px solid @gsrp_color_warning;\n"
		"}\n"
		".gsrp_msgw_question {\n"
		"\tbackground-color: lighter(@gsrp_color_info);\n"	// NOTE: Same as INFO.
		"\tborder: 2px solid @gsrp_color_info;\n"
		"}\n"
		".gsrp_msgw_error {\n"
		"\tbackground-color: lighter(@gsrp_color_error);\n"
		"\tborder: 2px solid @gsrp_color_error;\n"
		"}\n"
		".gsrp_msgw_info_dark {\n"
		"\tbackground-color: darker(@gsrp_color_info);\n"
		"\tborder: 2px solid @gsrp_color_info;\n"
		"}\n"
		".gsrp_msgw_warning_dark {\n"
		"\tbackground-color: darker(@gsrp_color_warning);\n"
		"\tborder: 2px solid @gsrp_color_warning;\n"
		"}\n"
		".gsrp_msgw_question_dark {\n"
		"\tbackground-color: darker(@gsrp_color_info);\n"	// NOTE: Same as INFO.
		"\tborder: 2px solid @gsrp_color_info;\n"
		"}\n"
		".gsrp_msgw_error_dark {\n"
		"\tbackground-color: darker(@gsrp_color_error);\n"
		"\tborder: 2px solid @gsrp_color_error;\n"
		"}\n";

	GTK_CSS_PROVIDER_LOAD_FROM_DATA(GTK_CSS_PROVIDER(provider), css_MessageWidget, -1);
	g_object_unref(provider);
#endif /* GTK_CHECK_VERSION(3,0,0) */
}

static void
rp_message_widget_init(RpMessageWidget *widget)
{
	GtkBox *hbox;
#if GTK_CHECK_VERSION(3,0,0)
	// Make this an HBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_HORIZONTAL);
	hbox = GTK_BOX(widget);
#else /* !GTK_CHECK_VERSION(3,0,0) */
	// Add a GtkEventBox for the inner color.
	widget->evbox_inner = gtk_event_box_new();
	gtk_widget_set_name(widget->evbox_inner, "evbox_inner");
	gtk_container_add(GTK_CONTAINER(widget), widget->evbox_inner);

	// Add a GtkHBox for all the other widgets.
	widget->hbox = gtk_hbox_new(0, 0);
	gtk_widget_set_name(widget->hbox, "hbox");
	gtk_container_add(GTK_CONTAINER(widget->evbox_inner), widget->hbox);
	hbox = GTK_BOX(widget->hbox);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	widget->messageType = GTK_MESSAGE_OTHER;
	widget->image = gtk_image_new();
	gtk_widget_set_name(widget->image, "image");
	widget->label = gtk_label_new(NULL);
	gtk_widget_set_name(widget->label, "label");

	// TODO: Align the GtkImage to the top of the first line
	// if the label has multiple lines.

	widget->close_button = gtk_button_new();
	gtk_widget_set_name(widget->close_button, "close_button");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_button_set_icon_name(GTK_BUTTON(widget->close_button), "dialog-close");
	gtk_button_set_has_frame(GTK_BUTTON(widget->close_button), FALSE);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const imageClose = gtk_image_new_from_icon_name("dialog-close", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_name(imageClose, "imageClose");
	gtk_button_set_image(GTK_BUTTON(widget->close_button), imageClose);
	gtk_button_set_relief(GTK_BUTTON(widget->close_button), GTK_RELIEF_NONE);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if GTK_CHECK_VERSION(4,0,0)
	// TODO: Padding?
	gtk_box_append(hbox, widget->image);
	gtk_box_append(hbox, widget->label);
	gtk_box_append(hbox, widget->close_button);
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  if !GTK_CHECK_VERSION(3,0,0)
	gtk_widget_show(widget->evbox_inner);
	gtk_widget_show(widget->hbox);
#  endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_widget_show(widget->label);
	gtk_widget_show(widget->close_button);

	gtk_box_pack_start(hbox, widget->image, FALSE, FALSE, 4);
	gtk_box_pack_start(hbox, widget->label, FALSE, FALSE, 0);
	gtk_box_pack_end(hbox, widget->close_button, FALSE, FALSE, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	g_signal_connect(widget->close_button, "clicked",
		G_CALLBACK(rp_message_widget_close_button_clicked_handler), widget);
}

GtkWidget*
rp_message_widget_new(void)
{
	return g_object_new(RP_TYPE_MESSAGE_WIDGET, NULL);
}

/** Properties **/

static void
rp_message_widget_set_property(GObject		*object,
			       guint		 prop_id,
			       const GValue	*value,
			       GParamSpec	*pspec)
{
	RpMessageWidget *const widget = RP_MESSAGE_WIDGET(object);

	switch (prop_id) {
		case PROP_TEXT:
			gtk_label_set_text(GTK_LABEL(widget->label), g_value_get_string(value));
			break;

		case PROP_MESSAGE_TYPE:
			rp_message_widget_set_message_type(widget, (GtkMessageType)g_value_get_enum(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_message_widget_get_property(GObject		*object,
			       guint		 prop_id,
			       GValue		*value,
			       GParamSpec	*pspec)
{
	RpMessageWidget *const widget = RP_MESSAGE_WIDGET(object);

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

void
rp_message_widget_set_text(RpMessageWidget *widget, const gchar *str)
{
	gtk_label_set_text(GTK_LABEL(widget->label), str);

	// FIXME: If called from rom_data_view_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_TEXT]);
}

const gchar*
rp_message_widget_get_text(RpMessageWidget *widget)
{
	return gtk_label_get_text(GTK_LABEL(widget->label));
}

void
rp_message_widget_set_message_type(RpMessageWidget *widget, GtkMessageType messageType)
{
	// Update the icon.
	// Background colors based on KMessageWidget.
	typedef struct _IconInfo_t {
		const char icon_name[20];	// XDG icon name
		const char css_class[20];	// CSS class (GTK+ 3.x only)
		uint32_t border_color;		// from KMessageWidget
		uint32_t bg_color;		// lightened version of border_color
	} IconInfo_t;
	static const IconInfo_t iconInfo_tbl[] = {
		{"dialog-information",	"gsrp_msgw_info",	0x3DAEE9, 0x7FD3FF},	// INFO
		{"dialog-warning",	"gsrp_msgw_warning",	0xF67400, 0xFF9B41},	// WARNING
		{"dialog-question",	"gsrp_msgw_question",	0x3DAEE9, 0x7FD3FF},	// QUESTION (same as INFO)
		{"dialog-error",	"gsrp_msgw_error",	0xDA4453, 0xF77E8A},	// ERROR
		{"",			"",			0x000000, 0x000000},	// OTHER
	};

	if (messageType < 0 || messageType >= ARRAY_SIZE(iconInfo_tbl)) {
		// Default to OTHER.
		messageType = GTK_MESSAGE_OTHER;
	}

	// TODO: Update regardless if the system theme changes.
	if (widget->messageType == messageType)
		return;
	widget->messageType = messageType;

	const IconInfo_t *const pIconInfo = &iconInfo_tbl[messageType];

	gtk_widget_set_visible(widget->image, (pIconInfo->icon_name[0] != '\0'));
	if (pIconInfo->icon_name[0] != '\0') {
#if GTK_CHECK_VERSION(4,0,0)
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->image), pIconInfo->icon_name);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		// TODO: Better icon size?
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->image), pIconInfo->icon_name, GTK_ICON_SIZE_BUTTON);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if GTK_CHECK_VERSION(3,0,0)
		// Dark CSS classes
		static const char dark_css_class_tbl[][24] = {
			"gsrp_msgw_info_dark",
			"gsrp_msgw_warning_dark",
			"gsrp_msgw_question_dark",
			"gsrp_msgw_error_dark",
		};

		// Remove all of our CSS classes first.
		GtkStyleContext *const context = gtk_widget_get_style_context(GTK_WIDGET(widget));
		static const IconInfo_t *const pIconInfo_tbl_end = &iconInfo_tbl[ARRAY_SIZE(iconInfo_tbl)];
		for (const IconInfo_t *p = iconInfo_tbl; p < pIconInfo_tbl_end; p++) {
			gtk_style_context_remove_class(context, p->css_class);
		}
		for (unsigned int i = 0; i < ARRAY_SIZE(dark_css_class_tbl); i++) {
			gtk_style_context_remove_class(context, dark_css_class_tbl[i]);
		}

		// Get the text color. If its grayscale value is >= 0.75,
		// assume we're using a dark theme.
		bool dark = false;
		GdkRGBA color;
		gboolean bRet = gtk_style_context_lookup_color(context, "theme_text_color", &color);
		if (bRet) {
			// BT.601 grayscale conversion
			const gfloat grayscale = (color.red   * 0.299f) +
			                         (color.green * 0.587f) +
						 (color.blue  * 0.114f);
			dark = (grayscale >= 0.750f);
		}

		// Add the new CSS class.
		gtk_style_context_add_class(context,
			(likely(!dark) ? pIconInfo->css_class : dark_css_class_tbl[messageType]));
#else /* !GTK_CHECK_VERSION(3,0,0) */
		GdkColor color;
		color.pixel	 = pIconInfo->border_color;
		color.red	 = (color.pixel >> 16) & 0xFF;
		color.green	 = (color.pixel >>  8) & 0xFF;
		color.blue	 =  color.pixel & 0xFF;
		color.red	|= (color.red << 8);
		color.green	|= (color.green << 8);
		color.blue	|= (color.blue << 8);
		gtk_widget_modify_bg(GTK_WIDGET(widget), GTK_STATE_NORMAL, &color);

		color.pixel	 = pIconInfo->bg_color;
		color.red	 = (color.pixel >> 16) & 0xFF;
		color.green	 = (color.pixel >>  8) & 0xFF;
		color.blue	 =  color.pixel & 0xFF;
		color.red	|= (color.red << 8);
		color.green	|= (color.green << 8);
		color.blue	|= (color.blue << 8);
		gtk_widget_modify_bg(GTK_WIDGET(widget->evbox_inner), GTK_STATE_NORMAL, &color);

		gtk_container_set_border_width(GTK_CONTAINER(widget->evbox_inner), 2);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	} else {
#if !GTK_CHECK_VERSION(3,0,0)
		gtk_container_set_border_width(GTK_CONTAINER(widget->evbox_inner), 0);
		gtk_widget_modify_bg(GTK_WIDGET(widget), GTK_STATE_NORMAL, NULL);
		gtk_widget_modify_bg(GTK_WIDGET(widget->evbox_inner), GTK_STATE_NORMAL, NULL);
#endif /* !GTK_CHECK_VERSION(3,0,0) */
	}

	// FIXME: If called from rom_data_view_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_MESSAGE_TYPE]);
}

GtkMessageType
rp_message_widget_get_message_type(RpMessageWidget *widget)
{
	return widget->messageType;
}

/** Signal handlers **/

/**
 * The Close button was clicked.
 * @param button Close button
 * @param widget RpMessageWidget
 */
static void
rp_message_widget_close_button_clicked_handler(GtkButton *button, RpMessageWidget *widget)
{
	// TODO: Animation like KMessageWidget.
	RP_UNUSED(button);
	gtk_widget_set_visible(GTK_WIDGET(widget), false);
}

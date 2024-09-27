/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * MessageWidget.c: Message widget (similar to KMessageWidget)             *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageWidget.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_TEXT,
	PROP_MESSAGE_TYPE,

	// GtkRevealer wrapper properties
	// NOTE: Ordering matches GtkRevealer, even though
	// GtkRevealer has the accessor functions sorted...
	PROP_TRANSITION_TYPE,
	PROP_TRANSITION_DURATION,

	PROP_LAST
} MessageWidgetPropID;

// GtkRevealer was added in GTK 3.10.
// (This is also our minimum GTK3 version.)
#if GTK_CHECK_VERSION(3,9,0)
#  define USE_GTK_REVEALER 1
#endif /* GTK_CHECK_VERSION(3,9,0) */

#if !GTK_CHECK_VERSION(3,9,0)
// GtkRevealerTransitionType enumeration registration
// TODO: rp-gtk-enums instead?
static GType
gtk_revealer_transition_type_get_type(void)
{
	static gsize static_g_enum_type_id;

	if (g_once_init_enter(&static_g_enum_type_id)) {
		static const GEnumValue values[] = {
			{GTK_REVEALER_TRANSITION_TYPE_NONE, "GTK_REVEALER_TRANSITION_TYPE_NONE", "None"},
			{GTK_REVEALER_TRANSITION_TYPE_CROSSFADE, "GTK_REVEALER_TRANSITION_TYPE_CROSSFADE", "Crossfade"},
			{GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT, "GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT", "Right"},
			{GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT, "GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT", "Left"},
			{GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP, "GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP", "Up"},
			{GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN, "GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN", "Down"},
			{0, NULL, NULL}
		};

		GType g_enum_type_id = g_enum_register_static("GtkRevealerTransitionType", values);

		g_once_init_leave(&static_g_enum_type_id, g_enum_type_id);
	}
	return static_g_enum_type_id;
}
#  define GTK_TYPE_REVEALER_TRANSITION_TYPE gtk_revealer_transition_type_get_type()
#endif /* !GTK_CHECK_VERSION(3,9,0) */

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

#ifdef USE_GTK_REVEALER
static void	rp_message_widget_notify_child_revealed(GtkRevealer	*revealer,
							GParamSpec	*pspec,
							RpMessageWidget	*widget);
#endif /* USE_GTK_REVEALER */

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

#ifdef USE_GTK_REVEALER
	GtkWidget *revealer;
#else /* !USE_GTK_REVEALER */
	GtkWidget *evbox_inner;
#endif /* USE_GTK_REVEALER */
	GtkWidget *hbox;

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
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_MESSAGE_TYPE] = g_param_spec_enum(
		"message-type", "Message Type", "Message type.",
		GTK_TYPE_MESSAGE_TYPE, GTK_MESSAGE_OTHER,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

#ifdef USE_GTK_REVEALER
	props[PROP_TRANSITION_TYPE] = g_param_spec_enum(
		"transition-type", NULL, NULL,
		GTK_TYPE_REVEALER_TRANSITION_TYPE,
		GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_TRANSITION_DURATION] = g_param_spec_uint(
		"transition-duration", NULL, NULL,
		0, G_MAXUINT, 250,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
#else /* !USE_GTK_REVEALER */
	props[PROP_TRANSITION_TYPE] = g_param_spec_enum(
		"transition-type", NULL, NULL,
		GTK_TYPE_REVEALER_TRANSITION_TYPE,
		GTK_REVEALER_TRANSITION_TYPE_NONE,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	props[PROP_TRANSITION_DURATION] = g_param_spec_uint(
		"transition-duration", NULL, NULL,
		0, 0, 0,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));
#endif /* USE_GTK_REVEALER */

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

	GTK_CSS_PROVIDER_LOAD_FROM_STRING(GTK_CSS_PROVIDER(provider), css_MessageWidget);
	g_object_unref(provider);
#endif /* GTK_CHECK_VERSION(3,0,0) */
}

static void
rp_message_widget_init(RpMessageWidget *widget)
{
	GtkBox *hbox;

#if GTK_CHECK_VERSION(4,0,0)
	// Hide the MessageWidget initially.
	// It'll be shown when the child is revealed.
	// NOTE: Hide/show is needed even when using GtkRevealer
	// due to parent GtkBox spacing.
	gtk_widget_set_visible(GTK_WIDGET(widget), FALSE);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#ifdef USE_GTK_REVEALER
	// Make this an HBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_HORIZONTAL);

	// Create the GtkRevealer.
	widget->revealer = gtk_revealer_new();
	gtk_widget_set_name(widget->revealer, "revealer");
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(widget), widget->revealer);
	gtk_widget_set_hexpand(widget->revealer, TRUE);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(widget), widget->revealer, TRUE, TRUE, 0);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#else /* !USE_GTK_REVEALER */
	// Add a GtkEventBox for the inner color.
#  if GTK_CHECK_VERSION(4,0,0)
	// NOTE: GTK4 removed GtkEventBox.
	// We should be using GtkRevealer...
	// This code is here for testing purposes only.
	widget->evbox_inner = rp_gtk_hbox_new(0);
	gtk_box_append(GTK_BOX(widget), widget->evbox_inner);
	gtk_widget_set_hexpand(widget->evbox_inner, TRUE);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	widget->evbox_inner = gtk_event_box_new();
#    if GTK_CHECK_VERSION(3,0,0)
	gtk_box_pack_start(GTK_BOX(widget), widget->evbox_inner, TRUE, TRUE, 0);
#    else /* !GTK_CHECK_VERSION(3,0,0) */
	// FIXME: On GTK2, using gtk_box_pack_start() results in the box being too small?
	gtk_container_add(GTK_CONTAINER(widget), widget->evbox_inner);
#    endif /* GTK_CHECK_VERSION(3,0,0) */
#  endif /* GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_set_name(widget->evbox_inner, "evbox_inner");
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Add a GtkHBox for all the other widgets.
	widget->hbox = rp_gtk_hbox_new(4);
	hbox = GTK_BOX(widget->hbox);
	gtk_widget_set_name(widget->hbox, "hbox");

#if GTK_CHECK_VERSION(4,0,0)
	// Extra padding needed on GTK4 for some reason.
	gtk_widget_set_margin_start(widget->hbox, 4);
	gtk_widget_set_margin_end(widget->hbox, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if USE_GTK_REVEALER
	gtk_revealer_set_child(GTK_REVEALER(widget->revealer), widget->hbox);
#else /* !USE_GTK_REVEALER */
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(widget->evbox_inner), widget->hbox);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_container_add(GTK_CONTAINER(widget->evbox_inner), widget->hbox);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#endif /* USE_GTK_REVEALER */

	widget->messageType = GTK_MESSAGE_OTHER;

	widget->image = gtk_image_new();
	gtk_widget_set_name(widget->image, "image");

	// Need to use GtkMisc to ensure the label is left-aligned.
	widget->label = gtk_label_new(NULL);
	gtk_widget_set_name(widget->label, "label");
#if GTK_CHECK_VERSION(3,16,0)
	gtk_label_set_xalign(GTK_LABEL(widget->label), 0.0f);
	gtk_label_set_yalign(GTK_LABEL(widget->label), 0.5f);
#else /* !GTK_CHECK_VERSION(3,16,0) */
	gtk_misc_set_alignment(GTK_MISC(widget->label), 0.0f, 0.5f);
#endif /* GTK_CHECK_VERSION(3,16,0) */
#if GTK_CHECK_VERSION(4,0,0)
	// FIXME: On GTK3, this is causing the label to be center-aligned.
	// On GTK4, this is *required* for the close button to be right-aligned.
	// In GTK3, if this is set, toggling the full "expand" property in the
	// GTK inspector "fixes" the alignment. (GTK3 bug?)
	gtk_widget_set_hexpand(widget->label, TRUE);
#endif /* GTK_CHECK_VERSION(3,0,0) */

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
	gtk_box_pack_start(hbox, widget->image, FALSE, FALSE, 4);
	gtk_box_pack_start(hbox, widget->label, FALSE, FALSE, 0);
	gtk_box_pack_end(hbox, widget->close_button, FALSE, FALSE, 0);
#  if USE_GTK_REVEALER
	gtk_widget_show_all(widget->revealer);
#  else /* USE_GTK_REVEALER */
	gtk_widget_show_all(widget->evbox_inner);
#  endif /* USE_GTK_REVEALER */
#endif /* GTK_CHECK_VERSION(4,0,0) */

	g_signal_connect(widget->close_button, "clicked",
		G_CALLBACK(rp_message_widget_close_button_clicked_handler), widget);
#ifdef USE_GTK_REVEALER
	g_signal_connect(widget->revealer, "notify::child-revealed",
		G_CALLBACK(rp_message_widget_notify_child_revealed), widget);
#endif /* USE_GTK_REVEALER */
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
			rp_message_widget_set_text(widget, g_value_get_string(value));
			break;
		case PROP_MESSAGE_TYPE:
			rp_message_widget_set_message_type(widget, (GtkMessageType)g_value_get_enum(value));
			break;

#ifdef USE_GTK_REVEALER
		case PROP_TRANSITION_TYPE:
			rp_message_widget_set_transition_type(widget, (GtkRevealerTransitionType)g_value_get_enum(value));
			break;
		case PROP_TRANSITION_DURATION:
			rp_message_widget_set_transition_duration(widget, g_value_get_uint(value));
			break;
#else /* !USE_GTK_REVEALER */
		case PROP_TRANSITION_TYPE:
		case PROP_TRANSITION_DURATION:
			// Nothing to do here. (TODO: Error?)
			break;
#endif /* USE_GTK_REVEALER */

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

#ifdef USE_GTK_REVEALER
		case PROP_TRANSITION_TYPE:
			g_value_set_enum(value, gtk_revealer_get_transition_type(GTK_REVEALER(widget->revealer)));
			break;
		case PROP_TRANSITION_DURATION:
			g_value_set_uint(value, gtk_revealer_get_transition_duration(GTK_REVEALER(widget->revealer)));
			break;
#else /* !USE_GTK_REVEALER */
		case PROP_TRANSITION_TYPE:
			g_value_set_enum(value, GTK_REVEALER_TRANSITION_TYPE_NONE);
			break;
		case PROP_TRANSITION_DURATION:
			g_value_set_uint(value, 0);
			break;
#endif /* USE_GTK_REVEALER */

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

void
rp_message_widget_set_text(RpMessageWidget *widget, const gchar *str)
{
	g_return_if_fail(RP_IS_MESSAGE_WIDGET(widget));

	if (g_strcmp0(gtk_label_get_text(GTK_LABEL(widget->label)), str) != 0) {
		gtk_label_set_text(GTK_LABEL(widget->label), str);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_TEXT]);
	}
}

const gchar*
rp_message_widget_get_text(RpMessageWidget *widget)
{
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), NULL);
	return gtk_label_get_text(GTK_LABEL(widget->label));
}

void
rp_message_widget_set_message_type(RpMessageWidget *widget, GtkMessageType messageType)
{
	g_return_if_fail(RP_IS_MESSAGE_WIDGET(widget));

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
		static const IconInfo_t *const pIconInfo_tbl_end = &iconInfo_tbl[ARRAY_SIZE(iconInfo_tbl)];

#  if GTK_CHECK_VERSION(4,0,0)
		// Remove all of our CSS classes first.
		for (const IconInfo_t *p = iconInfo_tbl; p < pIconInfo_tbl_end-1; p++) {
			gtk_widget_remove_css_class(widget->hbox, p->css_class);
		}
		for (unsigned int i = 0; i < ARRAY_SIZE(dark_css_class_tbl); i++) {
			gtk_widget_remove_css_class(widget->hbox, dark_css_class_tbl[i]);
		}

		// Get the text color. If its grayscale value is >= 0.75,
		// assume we're using a dark theme.
		// FIXME: Better way to determine if a dark theme is in use.
		// (gtk_style_context_lookup_color() is deprecated in GTK4)
		bool dark = false;
		GdkRGBA color;
#if GTK_CHECK_VERSION(4,9,1)
		gtk_widget_get_color(GTK_WIDGET(widget->hbox), &color);
#else /* !GTK_CHECK_VERSION(4,9,1) */
		gboolean bRet = gtk_style_context_lookup_color(
			gtk_widget_get_style_context(widget->hbox), "theme_text_color", &color);
		if (bRet)
#endif /* GTK_CHECK_VERSION(4,9,1) */
		{
			// BT.601 grayscale conversion
			const gfloat grayscale = (color.red   * 0.299f) +
			                         (color.green * 0.587f) +
						 (color.blue  * 0.114f);
			dark = (grayscale >= 0.750f);
		}

		// Add the new CSS class.
		gtk_widget_add_css_class(widget->hbox,
			(likely(!dark) ? pIconInfo->css_class : dark_css_class_tbl[messageType]));
#  else /* !GTK_CHECK_VERSION(4,0,0) */
		// Remove all of our CSS classes first.
		GtkStyleContext *const context = gtk_widget_get_style_context(widget->hbox);
		for (const IconInfo_t *p = iconInfo_tbl; p < pIconInfo_tbl_end-1; p++) {
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
#  endif /* GTK_CHECK_VERSION(4,0,0) */
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

	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_MESSAGE_TYPE]);
}

GtkMessageType
rp_message_widget_get_message_type(RpMessageWidget *widget)
{
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), GTK_MESSAGE_OTHER);
	return widget->messageType;
}

gboolean
rp_message_widget_get_child_revealed(RpMessageWidget *widget)
{
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), FALSE);

#ifdef USE_GTK_REVEALER
	return gtk_revealer_get_child_revealed(GTK_REVEALER(widget->revealer));
#else /* !USE_GTK_REVEALER */
	// Not using GtkRevealer, so just get the widget visibility.
	return gtk_widget_get_visible(GTK_WIDGET(widget));
#endif /* USE_GTK_REVEALER */
}

void
rp_message_widget_set_reveal_child(RpMessageWidget *widget, gboolean reveal_child)
{
	// TODO: Add an actual property?
	g_return_if_fail(RP_IS_MESSAGE_WIDGET(widget));

#ifdef USE_GTK_REVEALER
	if (reveal_child) {
		// Make sure the widget is visible.
		gtk_widget_set_visible(GTK_WIDGET(widget), TRUE);
	}
	gtk_revealer_set_reveal_child(GTK_REVEALER(widget->revealer), reveal_child);
#else /* !USE_GTK_REVEALER */
	// Not using GtkRevealer, so just set the widget visibility.
	gtk_widget_set_visible(GTK_WIDGET(widget), reveal_child);
#endif /* USE_GTK_REVEALER */
}

gboolean
rp_message_widget_get_reveal_child(RpMessageWidget *widget)
{
	// TODO: Add property wrapper?
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), FALSE);

#ifdef USE_GTK_REVEALER
	return gtk_revealer_get_reveal_child(GTK_REVEALER(widget->revealer));
#else /* !USE_GTK_REVEALER */
	// Not using GtkRevealer, so just get the widget visibility.
	return gtk_widget_get_visible(GTK_WIDGET(widget));
#endif /* USE_GTK_REVEALER */
}

void
rp_message_widget_set_transition_duration(RpMessageWidget *widget, guint duration)
{
	g_return_if_fail(RP_IS_MESSAGE_WIDGET(widget));

#ifdef USE_GTK_REVEALER
	if (gtk_revealer_get_transition_duration(GTK_REVEALER(widget->revealer)) == duration)
		return;

	gtk_revealer_set_transition_duration(GTK_REVEALER(widget->revealer), duration);
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_TRANSITION_DURATION]);
#else /*! USE_GTK_REVEALER */
	// Not using GtkRevealer, so don't do anything.
	RP_UNUSED(duration);
#endif /* USE_GTK_REVEALER */
}

guint
rp_message_widget_get_transition_duration(RpMessageWidget *widget)
{
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), 0U);

#ifdef USE_GTK_REVEALER
	return gtk_revealer_get_transition_duration(GTK_REVEALER(widget->revealer));
#else /*! USE_GTK_REVEALER */
	// Not using GtkRevealer, so return a default value.
	return 0U;
#endif /* USE_GTK_REVEALER */
}

void
rp_message_widget_set_transition_type(RpMessageWidget *widget, GtkRevealerTransitionType transition)
{
	g_return_if_fail(RP_IS_MESSAGE_WIDGET(widget));

#ifdef USE_GTK_REVEALER
	if (gtk_revealer_get_transition_type(GTK_REVEALER(widget->revealer)) == transition)
		return;

	gtk_revealer_set_transition_type(GTK_REVEALER(widget->revealer), transition);
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_TRANSITION_TYPE]);
#else /*! USE_GTK_REVEALER */
	// Not using GtkRevealer, so don't do anything.
	RP_UNUSED(transition);
#endif /* USE_GTK_REVEALER */
}

GtkRevealerTransitionType
rp_message_widget_get_transition_type(RpMessageWidget *widget)
{
	g_return_val_if_fail(RP_IS_MESSAGE_WIDGET(widget), 0U);

#ifdef USE_GTK_REVEALER
	return gtk_revealer_get_transition_type(GTK_REVEALER(widget->revealer));
#else /*! USE_GTK_REVEALER */
	// Not using GtkRevealer, so return a default value.
	return GTK_REVEALER_TRANSITION_TYPE_NONE;
#endif /* USE_GTK_REVEALER */
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
	RP_UNUSED(button);

#ifdef USE_GTK_REVEALER
	gtk_revealer_set_reveal_child(GTK_REVEALER(widget->revealer), FALSE);
#else /* !USE_GTK_REVEALER */
	// Not using GtkRevealer, so just hide the widget.
	gtk_widget_set_visible(GTK_WIDGET(widget), FALSE);
#endif /* USE_GTK_REVEALER */
}

#ifdef USE_GTK_REVEALER
/**
 * The GtkRevealer child fully-revealed state has changed.
 * @param revealer GtkRevealer
 * @param pspec
 * @param widget RpMessageWidget
 */
static void
rp_message_widget_notify_child_revealed(GtkRevealer *revealer, GParamSpec *pspec, RpMessageWidget *widget)
{
	RP_UNUSED(pspec);

	if (!gtk_revealer_get_child_revealed(revealer)) {
		// Child widget has been hidden.
		// Hide the RpMessageWidget.
		// FIXME: This is a workaround for the spacing between widgets in GtkBox,
		// but it results in an abrupt transition...
		gtk_widget_set_visible(GTK_WIDGET(widget), FALSE);
	}
}
#endif /* USE_GTK_REVEALER */

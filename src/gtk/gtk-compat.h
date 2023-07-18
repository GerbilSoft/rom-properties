/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * gtk-compat.h: GTK+ compatibility functions.                             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "glib-compat.h"
#include <gtk/gtk.h>

// C includes
#include <assert.h>

G_BEGIN_DECLS

/** Functions added in GTK+ 2.14.0 **/

#if !GTK_CHECK_VERSION(2,13,4)
static inline GdkWindow*
gtk_widget_get_window(GtkWidget *widget)
{
	g_return_val_if_fail(widget != NULL, NULL);
	return widget->window;
}
#endif /* GTK_CHECK_VERSION(2,13,4) */

/** Functions added in GTK+ 3.0.0 **/

#if !GTK_CHECK_VERSION(2,91,6)
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
#  if (GTK_CHECK_VERSION(2,13,4) && defined(GSEAL_ENABLE))
	return tree_column->_g_sealed__button;
#  else
	return tree_column->button;
#  endif
}
#endif /* !GTK_CHECK_VERSION(2,91,6) */

/** Functions added in GTK+ 3.2.0 **/

#if !GTK_CHECK_VERSION(3,1,14)
static inline gboolean
gdk_event_get_button(const GdkEvent *event, guint *button)
{
	// from GTK+ source
	gboolean fetched = TRUE;
	guint number = 0;

	g_return_val_if_fail(event != NULL, FALSE);

	switch ((guint)event->any.type)
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
#endif /* !GTK_CHECK_VERSION(3,1,14) */

/** Functions added in GTK+ 3.10.0 **/

#if !GTK_CHECK_VERSION(3,9,12)
static inline GdkEventType
gdk_event_get_event_type(const GdkEvent *event)
{
	g_return_val_if_fail(event != NULL, GDK_NOTHING);

	return event->any.type;
}
#endif /* !GTK_CHECK_VERSION(3,9,12) */

/** Functions added in GTK+ 3.16.0 **/

#if !GTK_CHECK_VERSION(3,15,5) && defined(G_DEFINE_AUTOPTR_CLEANUP_FUNC)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkComboBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkDialog, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkEventBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkHBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GtkVBox, g_object_unref)
#endif /* !GTK_CHECK_VERSION(3,15,5) && defined(G_DEFINE_AUTOPTR_CLEANUP_FUNC) */

/** Functions added in GTK 4.0.0 **/

#if !GTK_CHECK_VERSION(3,89,3)
GtkWidget *gtk_widget_get_first_child(GtkWidget *widget);
#endif /* !GTK_CHECK_VERSION(3,89,3) */

// gtk_widget_get_root() was added in gtk-3.96.0.
// gtk_widget_get_toplvel() was dropped in gtk-3.98.0.
#if GTK_CHECK_VERSION(3,96,0)
#define GTK_WIDGET_GET_TOPLEVEL_FN(name, type, macro) \
static inline type* \
gtk_widget_get_toplevel_##name(GtkWidget *widget) \
{ \
	return macro(gtk_widget_get_root(widget)); \
}
#else /* !GTK_CHECK_VERSION(3,96,0) */
#define GTK_WIDGET_GET_TOPLEVEL_FN(name, type, macro) \
static inline type* \
gtk_widget_get_toplevel_##name(GtkWidget *widget) \
{ \
	return macro(gtk_widget_get_toplevel(widget)); \
}
#endif /* GTK_CHECK_VERSION(3,96,0) */

GTK_WIDGET_GET_TOPLEVEL_FN(widget, GtkWidget, GTK_WIDGET)
GTK_WIDGET_GET_TOPLEVEL_FN(window, GtkWindow, GTK_WINDOW)
GTK_WIDGET_GET_TOPLEVEL_FN(dialog, GtkDialog, GTK_DIALOG)

#if !GTK_CHECK_VERSION(3,98,0)
static inline void
gtk_label_set_wrap(GtkLabel *label, gboolean wrap)
{
	gtk_label_set_line_wrap(label, wrap);
}
#endif /* !GTK_CHECK_VERSION(3,98,0) */

#if !GTK_CHECK_VERSION(3,98,4)
static inline void
gtk_frame_set_child(GtkFrame *frame, GtkWidget *child)
{
	// TODO: Remove the existing child widget?
	gtk_container_add((GtkContainer*)frame, child);
}

static inline void
gtk_window_set_child(GtkWindow *window, GtkWidget *child)
{
	// TODO: Remove the existing child widget?
	gtk_container_add((GtkContainer*)window, child);
}

#  if GTK_CHECK_VERSION(3,1,6)
static inline void
gtk_overlay_set_child(GtkOverlay *overlay, GtkWidget *child)
{
	// TODO: Remove the existing child widget?
	gtk_container_add((GtkContainer*)overlay, child);
}
#  endif /* GTK_CHECK_VERSION(3,1,6) */

static inline void
gtk_scrolled_window_set_child(GtkScrolledWindow *scrolled_window, GtkWidget *child)
{
	// TODO: Remove the existing child widget?
	gtk_container_add((GtkContainer*)scrolled_window, child);
}
#endif /* GTK_CHECK_VERSION(3,98,4) */

#if !GTK_CHECK_VERSION(3,99,1)
// Prior to GTK4, GtkCheckButton inherited from GtkToggleButton.
// In GTK4, the two are now distinct classes.
static inline void
gtk_check_button_set_active(GtkCheckButton *self, gboolean setting)
{
	gtk_toggle_button_set_active((GtkToggleButton*)self, setting);
}
static inline gboolean
gtk_check_button_get_active(GtkCheckButton *self)
{
	return gtk_toggle_button_get_active((GtkToggleButton*)self);
}
#endif /* !GTK_CHECK_VERSION(3,99,1) */

#if !GTK_CHECK_VERSION(3,99,5)
static inline void
gtk_widget_class_set_activate_signal(GtkWidgetClass *widget_class, guint signal_id)
{
	widget_class->activate_signal = signal_id;
}
static inline guint
gtk_widget_class_get_activate_signal(GtkWidgetClass *widget_class)
{
	return widget_class->activate_signal;
}
#endif /* !GTK_CHECK_VERSION(3,99,5) */

/** Functions that changed in GTK 4.0.0 but are otherwise similar enough **/
/** to GTK2/GTK3 that a simple wrapper function or macro can be used.    **/

// GtkCssProvider: Error parameter removed.
// Use the GtkCssProvider::parsing-error signal instead if needed.
#if GTK_CHECK_VERSION(3,89,1)
#  define GTK_CSS_PROVIDER_LOAD_FROM_DATA(provider, data, length) \
	gtk_css_provider_load_from_data((provider), (data), (length))
#else /* !GTK_CHECK_VERSION(3,89,1) */
#  define GTK_CSS_PROVIDER_LOAD_FROM_DATA(provider, data, length) \
	gtk_css_provider_load_from_data((provider), (data), (length), NULL)
#endif /* GTK_CHECK_VERSION(3,89,1) */

// Clipboard
#if GTK_CHECK_VERSION(4,0,0)
static inline void
rp_gtk_main_clipboard_set_text(const char *text)
{
	GValue value = G_VALUE_INIT;
	g_value_init(&value, G_TYPE_STRING);
	g_value_set_string(&value, text);

	GdkDisplay *const display = gdk_display_get_default();
	GdkClipboard *const clipboard = gdk_display_get_clipboard(display);
	gdk_clipboard_set_value(clipboard, &value);

	g_value_unset(&value);
}
#else /* !GTK_CHECK_VERSION(4,0,0) */
static inline void
rp_gtk_main_clipboard_set_text(const char *text)
{
	GtkClipboard *const clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, text, -1);
}
#endif /* GTK_CHECK_VERSION(4,0,0) */

static inline GtkWidget*
rp_gtk_hbox_new(gint spacing)
{
#if GTK_CHECK_VERSION(2,90,1)
	return gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
#else /* GTK_CHECK_VERSION(2,90,1) */
	return gtk_hbox_new(FALSE, spacing);
#endif
}

static inline GtkWidget*
rp_gtk_vbox_new(gint spacing)
{
#if GTK_CHECK_VERSION(2,90,1)
	return gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
#else /* GTK_CHECK_VERSION(2,90,1) */
	return gtk_vbox_new(FALSE, spacing);
#endif
}

// TODo: Find the exact version when GtkButtonBox was removed.
#if !GTK_CHECK_VERSION(4,0,0)
static inline GtkWidget*
rp_gtk_hbutton_box_new(void)
{
#if GTK_CHECK_VERSION(2,90,1)
	return gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else /* GTK_CHECK_VERSION(2,90,1) */
	return gtk_hbutton_box_new();
#endif
}

static inline GtkWidget*
rp_gtk_vbutton_box_new(void)
{
#if GTK_CHECK_VERSION(2,90,1)
	return gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
#else /* GTK_CHECK_VERSION(2,90,1) */
	return gtk_vbutton_box_new();
#endif
}
#endif /* !GTK_CHECK_VERSION(4,0,0) */

/** "?align" shenanigans **/

// FIXME: Cannot set both HALIGN and VALIGN for GTK2...
// Setting HALIGN sets VALIGN to Top.
// Setting VALIGN sets HALIGN to Left.
#if GTK_CHECK_VERSION(3,0,0)
#  define GTK_WIDGET_HALIGN_LEFT(widget)	gtk_widget_set_halign((widget), GTK_ALIGN_START)
#  define GTK_WIDGET_HALIGN_CENTER(widget)	gtk_widget_set_halign((widget), GTK_ALIGN_CENTER)
#  define GTK_WIDGET_HALIGN_RIGHT(widget)	gtk_widget_set_halign((widget), GTK_ALIGN_END)
#else
#  define GTK_WIDGET_HALIGN_LEFT(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 0.0f, 0.0f)
#  define GTK_WIDGET_HALIGN_CENTER(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 0.5f, 0.0f)
#  define GTK_WIDGET_HALIGN_RIGHT(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 1.0f, 0.0f)
#endif

#if GTK_CHECK_VERSION(3,0,0)
#  define GTK_WIDGET_VALIGN_TOP(widget)		gtk_widget_set_valign((widget), GTK_ALIGN_START)
#  define GTK_WIDGET_VALIGN_CENTER(widget)	gtk_widget_set_valign((widget), GTK_ALIGN_CENTER)
#  define GTK_WIDGET_VALIGN_BOTTOM(widget)	gtk_widget_set_valign((widget), GTK_ALIGN_END)
#else
#  define GTK_WIDGET_VALIGN_TOP(widget)		gtk_misc_set_alignment(GTK_MISC(widget), 0.0f, 0.0f)
#  define GTK_WIDGET_VALIGN_CENTER(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 0.0f, 0.5f)
#  define GTK_WIDGET_VALIGN_BOTTOM(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 0.0f, 1.0f)
#endif

#if GTK_CHECK_VERSION(3,16,0)
#  define GTK_LABEL_XALIGN_LEFT(label)		gtk_label_set_xalign(GTK_LABEL(label), 0.0f)
#  define GTK_LABEL_XALIGN_CENTER(label)	gtk_label_set_xalign(GTK_LABEL(label), 0.5f)
#  define GTK_LABEL_XALIGN_RIGHT(label)		gtk_label_set_xalign(GTK_LABEL(label), 1.0f)
#else
#  define GTK_LABEL_XALIGN_LEFT(label)		gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f)
#  define GTK_LABEL_XALIGN_CENTER(label)	gtk_misc_set_alignment(GTK_MISC(label), 0.5f, 0.0f)
#  define GTK_LABEL_XALIGN_RIGHT(label)		gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.0f)
#endif

/** "margin" shenanigans **/

#if GTK_CHECK_VERSION(2,91,0) && !GTK_CHECK_VERSION(3,11,2)
// GTK [3.0,3.12): Has 'margin-*' properties, but uses left/right instead of start/end.
static inline void
gtk_widget_set_margin_start(GtkWidget *widget, gint margin)
{
	gtk_widget_set_margin_left(widget, margin);
}

static inline void
gtk_widget_set_margin_end(GtkWidget *widget, gint margin)
{
	gtk_widget_set_margin_right(widget, margin);
}
#endif /* GTK_CHECK_VERSION(2,91,0) && !GTK_CHECK_VERSION(3,11,2) */

#if GTK_CHECK_VERSION(2,91,0)
/**
 * Set margin for all four sides.
 * @param widget GtkWidget
 */
static inline void
gtk_widget_set_margin(GtkWidget *widget, gint margin)
{
	gtk_widget_set_margin_start(widget, margin);
	gtk_widget_set_margin_end(widget, margin);
	gtk_widget_set_margin_top(widget, margin);
	gtk_widget_set_margin_bottom(widget, margin);
}
#endif /* GTK_CHECK_VERSION(2,91,0) */

/******** GtkDropDown compatibility stuff (GTK4) ********/

// GTK4: Use GtkDropDown and GtkColumnView.
// GTK2, GTK3: Use GtkComboBox and GtkTreeView.
#if GTK_CHECK_VERSION(3,99,0)
#  define USE_GTK_DROP_DOWN 1
#  define USE_GTK_COLUMN_VIEW 1
#  define OUR_COMBO_BOX(obj) GTK_DROP_DOWN(obj)
typedef GtkDropDown OurComboBox;
#else /* !GTK_CHECK_VERSION(3,99,0) */
#  define OUR_COMBO_BOX(obj) GTK_COMBO_BOX(obj)
typedef GtkComboBox OurComboBox;
#endif /* GTK_CHECK_VERSION(3,99,0) */

#ifdef USE_GTK_DROP_DOWN
#  define COMPARE_CBO(widget, defval) \
	(gtk_drop_down_get_selected(GTK_DROP_DOWN(widget)) != (defval))
#  define SET_CBO(widget, value) \
	gtk_drop_down_set_selected(GTK_DROP_DOWN(widget), (value))
#  define GET_CBO(widget) \
	gtk_drop_down_get_selected(GTK_DROP_DOWN(widget))
#else /* !USE_GTK_DROP_DOWN */
#  define COMPARE_CBO(widget, defval) \
	(gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != (defval))
#  define SET_CBO(widget, value) \
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), (value))
#  define GET_CBO(widget) \
	gtk_combo_box_get_active(GTK_COMBO_BOX(widget))
#endif /* USE_GTK_DROP_DOWN */

/* convenience macros for GtkCheckButton */
#define COMPARE_CHK(widget, defval) \
	(gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != (defval))
#define SET_CHK(widget, value) \
	gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), (value))
#define GET_CHK(widget) \
	gtk_check_button_get_active(GTK_CHECK_BUTTON(widget))

/** rom-properties GTK function wrappers **/

/**
 * gtk_check_button_new_with_mnemonic() wrapper that uses '&' for mnemonics.
 * @param label Label with '&' mnemonic.
 * @return GtkCheckButton
 */
GtkWidget *rp_gtk_check_button_new_with_mnemonic(const char *label);

G_END_DECLS

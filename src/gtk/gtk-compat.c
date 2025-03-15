/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * gtk-compat.c: GTK+ compatibility functions.                             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gtk-compat.h"

// C includes
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/** Functions added in GTK 4.0.0 **/

#if !GTK_CHECK_VERSION(3, 89, 3)
GtkWidget *gtk_widget_get_first_child(GtkWidget *widget)
{
	GtkWidget *ret = NULL;

	// Assuming this is a GtkContainer.
	assert(GTK_IS_CONTAINER(widget));
	GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(widget));
	if (!widgetList)
		return ret;

	// NOTE: First widget in the list matches the first widget in the
	// UI file, contrary to the bitfield stuff in RomDataView...
	GList *const widgetIter = g_list_first(widgetList);
	assert(widgetIter != NULL);
	if (widgetIter)
		ret = GTK_WIDGET(widgetIter->data);

	g_list_free(widgetList);
	return ret;
}
#endif /* !GTK_CHECK_VERSION(3, 89, 3) */

/** Functions that changed in GTK 4.0.0 but are otherwise similar enough **/
/** to GTK2/GTK3 that a simple wrapper function or macro can be used.    **/

#if GTK_CHECK_VERSION(4, 0, 0)
void rp_gtk_main_clipboard_set_text(const char *text)
{
	GValue value = G_VALUE_INIT;
	g_value_init(&value, G_TYPE_STRING);
	g_value_set_string(&value, text);

	GdkDisplay *const display = gdk_display_get_default();
	GdkClipboard *const clipboard = gdk_display_get_clipboard(display);
	gdk_clipboard_set_value(clipboard, &value);

	g_value_unset(&value);
}
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

/** rom-properties GTK function wrappers **/

static inline gchar *label_mnemonic_convert(const char *label)
{
	gchar *const str = g_strdup(label);
	gchar *const p = strchr(str, '&');
	if (p) {
		*p = '_';
	}
	return str;
}

/**
 * gtk_check_button_new_with_mnemonic() wrapper that uses '&' for mnemonics.
 * @param label Label with '&' mnemonic.
 * @return GtkCheckButton
 */
GtkWidget *rp_gtk_check_button_new_with_mnemonic(const char *label)
{
	assert(label != NULL);
	if (!label) {
		return gtk_check_button_new_with_mnemonic(NULL);
	}

	gchar *const str = label_mnemonic_convert(label);
	GtkWidget *const checkButton = gtk_check_button_new_with_mnemonic(str);
	g_free(str);
	return checkButton;
}

/**
 * gtk_label_new_with_mnemonic() wrapper that uses '&' for mnemonics.
 * @param label Label with '&' mnemonic.
 * @return GtkLabel
 */
GtkWidget *rp_gtk_label_new_with_mnemonic(const char *label)
{
	assert(label != NULL);
	if (!label) {
		return gtk_label_new_with_mnemonic(NULL);
	}

	gchar *const str = label_mnemonic_convert(label);
	GtkWidget *const gtkLabel = gtk_label_new_with_mnemonic(str);
	g_free(str);
	return gtkLabel;
}

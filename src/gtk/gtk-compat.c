/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * gtk-compat.c: GTK+ compatibility functions.                             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "gtk-compat.h"

// C includes
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/** Functions added in GTK 4.0.0 **/

#if !GTK_CHECK_VERSION(3,89,3)
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
#endif /* !GTK_CHECK_VERSION(3,89,3) */

/** rom-properties GTK function wrappers **/

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

	char *const str = strdup(label);
	char *const p = strchr(str, '&');
	if (p) {
		*p = '_';
	}

	GtkWidget *const checkButton = gtk_check_button_new_with_mnemonic(str);
	free(str);
	return checkButton;
}

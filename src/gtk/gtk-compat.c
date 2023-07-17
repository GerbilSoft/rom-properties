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

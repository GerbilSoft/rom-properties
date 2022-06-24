/***************************************************************************
 * ROM Properties Page shell extension. (GTK)                              *
 * stub-export.c: Exported function for the rp-config stub.                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.h"
#include "ConfigDialog.hpp"

#if !GTK_CHECK_VERSION(3,0,0)
// Make GtkApplication a generic pointer.
typedef void *GtkApplication;
#endif /* !GTK_CHECK_VERSION(3,0,0) */

/**
 * ConfigDialog was closed by clicking the X button.
 * @param dialog ConfigDialog
 * @param event GdkEvent
 * @param user_data
 */
static gboolean
config_dialog_delete_event(ConfigDialog *dialog,
			   GdkEvent     *event,
			   gpointer      user_data)
{
	RP_UNUSED(dialog);
	RP_UNUSED(event);
	RP_UNUSED(user_data);

	// Quit the application.
	gtk_main_quit();

	// Continue processing.
	return FALSE;
}


/**
 * Exported function for the rp-config stub.
 * @param argc
 * @param argv
 * @return 0 on success; non-zero on error.
 */
G_MODULE_EXPORT
int RP_C_API rp_show_config_dialog(int argc, char *argv[])
{
	// NOTE: Not passing command line parameters to GTK+, since
	// g_application_run() returns immediately if any parameters
	// that it doesn't recognize are found. rp-config's parameters
	// are for rp-stub only, and aren't used by ConfigDialog.
	RP_UNUSED(argc);
	RP_UNUSED(argv);

	// NOTE 2: Not using GtkApplication because it requires
	// GtkApplicationWindow, and won't stay around if we're
	// just using GtkDialog like we're doing with ConfigDialog.
	// TODO: Single-instance?

	CHECK_UID_RET(EXIT_FAILURE);
	gtk_init(NULL, NULL);

	// Initialize base i18n.
	rp_i18n_init();

	// Create the ConfigDialog.
	GtkWidget *const dialog = config_dialog_new();
	gtk_widget_show(dialog);

	// Connect signals.
	// We need to ensure the GTK main loop exits when the window is closed.
	g_signal_connect(dialog, "delete-event", G_CALLBACK(config_dialog_delete_event), NULL);

	// TODO: Get the return value?
	gtk_main();
	return 0;
}

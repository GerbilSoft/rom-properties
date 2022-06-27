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

#if GTK_CHECK_VERSION(4,0,0)
static bool stub_quit;

/**
 * ConfigDialog was closed by clicking the X button.
 * @param dialog ConfigDialog
 * @param user_data
 */
static gboolean
config_dialog_close_request(ConfigDialog *dialog, gpointer user_data)
{
	RP_UNUSED(dialog);
	RP_UNUSED(user_data);

	// Quit the application.
	stub_quit = true;

	// Continue processing.
	return FALSE;
}
#else /* !GTK_CHECK_VERSION(4,0,0) */
/**
 * ConfigDialog was closed by clicking the X button.
 * @param dialog ConfigDialog
 * @param event GdkEvent
 * @param user_data
 */
static gboolean
config_dialog_delete_event(ConfigDialog *dialog, GdkEvent *event, gpointer user_data)
{
	RP_UNUSED(dialog);
	RP_UNUSED(event);
	RP_UNUSED(user_data);

	// Quit the application.
	gtk_main_quit();

	// Continue processing.
	return FALSE;
}
#endif /* GTK_CHECK_VERSION(4,0,0) */

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

#if !GLIB_CHECK_VERSION(2,35,1)
        // g_type_init() is automatic as of glib-2.35.1
        // and is marked deprecated.
        g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2,32,0)
        // g_thread_init() is automatic as of glib-2.32.0
        // and is marked deprecated.
        if (!g_thread_supported()) {
                g_thread_init(NULL);
        }
#endif

	CHECK_UID_RET(EXIT_FAILURE);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_init();
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_init(NULL, NULL);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Initialize base i18n.
	rp_i18n_init();

	// Create the ConfigDialog.
	GtkWidget *const dialog = config_dialog_new();
	gtk_widget_show(dialog);

	// Connect signals.
	// We need to ensure the GTK main loop exits when the window is closed.
#if GTK_CHECK_VERSION(4,0,0)
	g_signal_connect(dialog, "close-request", G_CALLBACK(config_dialog_close_request), NULL);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	g_signal_connect(dialog, "delete-event", G_CALLBACK(config_dialog_delete_event), NULL);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// TODO: Get the return value?
#if GTK_CHECK_VERSION(4,0,0)
	stub_quit = false;
	while (!stub_quit) {
		g_main_context_iteration(NULL, TRUE);
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_main();
#endif /* GTK_CHECK_VERSION(4,0,0) */

	return 0;
}

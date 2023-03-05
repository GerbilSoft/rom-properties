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

#if !GTK_CHECK_VERSION(2,90,2)
// GtkApplication was introduced in GTK3.
// For GTK2, make it a generic opaque pointer.
typedef void *GtkApplication;
#endif /* !GTK_CHECK_VERSION(2,90,2) */

#if !GTK_CHECK_VERSION(2,90,2)
/**
 * ConfigDialog was closed by clicking the X button.
 * @param dialog ConfigDialog
 * @param event GdkEvent
 * @param user_data
 */
static gboolean
config_dialog_delete_event(RpConfigDialog *dialog, GdkEvent *event, gpointer user_data)
{
	RP_UNUSED(dialog);
	RP_UNUSED(event);
	RP_UNUSED(user_data);

	// Quit the application.
	gtk_main_quit();

	// Continue processing.
	return FALSE;
}
#endif /* !GTK_CHECK_VERSION(2,90,2) */

/**
 * GtkApplication activate() signal handler.
 * Also used manually for GTK2.
 * @param app GtkApplication (or nullptr on GTK2)
 * @param user_data
 */
static void
app_activate(GtkApplication *app, gpointer user_data)
{
	RP_UNUSED(user_data);

	// Initialize base i18n.
	rp_i18n_init();

	// Create the ConfigDialog.
	GtkWidget *const configDialog = rp_config_dialog_new();
	gtk_widget_set_name(configDialog, "configDialog");
	gtk_widget_set_visible(configDialog, true);
#if GTK_CHECK_VERSION(2,90,2)
	gtk_application_add_window(app, GTK_WINDOW(configDialog));
#else /* GTK_CHECK_VERSION(2,90,2) */
	// GTK2: No GtkApplication to manage the main loop, so we
	// need to to ensure it exits when the window is closed.
	RP_UNUSED(app);
	g_signal_connect(configDialog, "delete-event", G_CALLBACK(config_dialog_delete_event), NULL);
#endif /* !GTK_CHECK_VERSION(2,90,2) */
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
	int status;
#if GTK_CHECK_VERSION(2,90,2)
	GtkApplication *const app = gtk_application_new(
		"com.gerbilsoft.rom-properties.rp-config", G_APPLICATION_FLAGS_NONE);
	// NOTE: GApplication is supposed to set this, but KDE isn't seeing it...
	g_set_prgname("com.gerbilsoft.rom-properties.rp-config");
	g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);

	// NOTE: We aren't setting up command line options in GApplication,
	// so it will complain if argv has any parameters.
	char *argv_new[1] = { argv[0] };
	status = g_application_run(G_APPLICATION(app), 1, argv_new);
#else /* !GTK_CHECK_VERSION(2,90,2) */
	// NOTE: GTK2 doesn't send a startup notification.
	// Not going to implement the Startup Notification protocol manually
	// because GTK2 desktops likely wouldn't support it, anyway.
	gtk_init(NULL, NULL);
	app_activate(NULL, 0);
	gtk_main();
	status = 0;
#endif /* GTK_CHECK_VERSION(2,90,2) */

	return status;
}

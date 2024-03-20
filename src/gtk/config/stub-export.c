/***************************************************************************
 * ROM Properties Page shell extension. (GTK)                              *
 * stub-export.c: Exported function for the rp-config stub.                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.h"

#include "ConfigDialog.hpp"
#include "RomDataView.hpp"

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

/** rp-config **/

/**
 * GtkApplication activate() signal handler.
 * Also used manually for GTK2.
 * @param app GtkApplication (or NULL on GTK2)
 * @param user_data
 */
static void
rp_config_app_activate(GtkApplication *app, gpointer user_data)
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
	g_signal_connect(app, "activate", G_CALLBACK(rp_config_app_activate), NULL);

	// NOTE: We aren't setting up command line options in GApplication,
	// so it will complain if argv has any parameters.
	char *argv_new[1] = { argv[0] };
	status = g_application_run(G_APPLICATION(app), 1, argv_new);
#else /* !GTK_CHECK_VERSION(2,90,2) */
	// NOTE: GTK2 doesn't send a startup notification.
	// Not going to implement the Startup Notification protocol manually
	// because GTK2 desktops likely wouldn't support it, anyway.
	gtk_init(NULL, NULL);
	rp_config_app_activate(NULL, 0);
	gtk_main();
	status = 0;
#endif /* GTK_CHECK_VERSION(2,90,2) */

	return status;
}

/** RomDataView test program **/

/**
 * Dialog response handler
 * @param dialog GtkDialog
 * @param response_id Response ID
 * @param user_data
 */
static void
rp_show_RomDataView_dialog_response_handler(GtkDialog	*dialog,
					    gint	 response_id,
					    gpointer	 user_data)
{
	RP_UNUSED(user_data);

	switch (response_id) {
		case GTK_RESPONSE_OK:
		case GTK_RESPONSE_CANCEL:
			// Close the dialog.
#if GTK_CHECK_VERSION(3,9,8)
			gtk_window_close(GTK_WINDOW(dialog));
#else /* !GTK_CHECK_VERSION(3,9,8) */
			gtk_widget_destroy(GTK_WIDGET(dialog));
#endif /* GTK_CHECK_VERSION(3,9,8) */
			break;

		default:
			break;
	}
}

/**
 * GtkApplication activate() signal handler.
 * Also used manually for GTK2.
 * @param app GtkApplication (or NULL on GTK2)
 * @param user_data URI to open
 */
static void
rp_RomDataView_app_activate(GtkApplication *app, const gchar *uri)
{
	// Initialize base i18n.
	rp_i18n_init();

	// Create a GtkDialog.
	// TODO: Use GtkWindow on GTK4?
	char s_title[40];
	snprintf(s_title, sizeof(s_title), "RomDataView GTK%u test program", (unsigned int)GTK_MAJOR_VERSION);
	GtkWidget *const dialog = gtk_dialog_new_with_buttons(
		s_title,
		NULL,
		0,
		"OK", GTK_RESPONSE_OK,
		NULL);
	gtk_widget_set_name(dialog, "RomDataView-test-dialog");
	gtk_widget_set_visible(dialog, TRUE);

	GtkWidget *const contentArea  = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	// Create a GtkNotebook to simulate Nautilus, Thunar, etc.
	GtkWidget *const notebook = gtk_notebook_new();
	gtk_widget_set_name(notebook, "notebook");
	gtk_widget_set_visible(notebook, TRUE);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(contentArea), notebook);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(contentArea), notebook, TRUE, TRUE, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// NOTE: Thunar has an extra widget between the GtkNotebook and RomDataView.
	// TODO: Have RomDataView check for this instead of expecting it when using RP_DFT_XFCE?
	GtkWidget *const vboxNotebookPage = rp_gtk_vbox_new(4);
	gtk_widget_set_name(vboxNotebookPage, "vboxNotebookPage");
	gtk_widget_set_visible(vboxNotebookPage, TRUE);

	// Create a RomDataView object with the specified URI.
	// TODO: Which RpDescFormatType?
	GtkWidget *const romDataView = rp_rom_data_view_new_with_uri(uri, RP_DFT_XFCE);
	gtk_widget_set_visible(romDataView, TRUE);
	gtk_widget_set_name(romDataView, "romDataView");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(vboxNotebookPage), romDataView);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(vboxNotebookPage), romDataView, TRUE, TRUE, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Label for the tab.
	GtkWidget *const tabLabel = gtk_label_new("ROM Properties");
	gtk_widget_set_name(tabLabel, "tabLabel");

	// Add the RomDataView to the GtkNotebook.
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vboxNotebookPage, tabLabel);

	// Connect the dialog response handler.
	g_signal_connect(dialog, "response", G_CALLBACK(rp_show_RomDataView_dialog_response_handler), NULL);

#if GTK_CHECK_VERSION(2,90,2)
	gtk_application_add_window(app, GTK_WINDOW(dialog));
#else /* GTK_CHECK_VERSION(2,90,2) */
	// GTK2: No GtkApplication to manage the main loop, so we
	// need to to ensure it exits when the window is closed.
	RP_UNUSED(app);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(config_dialog_delete_event), NULL);
#endif /* !GTK_CHECK_VERSION(2,90,2) */
}

/**
 * Exported function for the RomDataView test program stub.
 * @param argc
 * @param argv
 * @return 0 on success; non-zero on error.
 */
G_MODULE_EXPORT
int RP_C_API rp_show_RomDataView_dialog(int argc, char *argv[])
{
	// NOTE: Not passing command line parameters to GTK+, since
	// g_application_run() returns immediately if any parameters
	// that it doesn't recognize are found. rp-config's parameters
	// are for rp-stub only, and aren't used by ConfigDialog.

	// TODO: argv[] needs to be updated such that [0] == argv[0] and [1] == URI.
	// For now, assuming the last element is the URI.
	if (argc < 2) {
		// Not enough parameters...
		fprintf(stderr, "*** GTK%u rp_show_RomDataView_dialog(): ERROR: No URI specified.\n", (unsigned int)GTK_MAJOR_VERSION);
		return EXIT_FAILURE;
	}

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
	g_signal_connect(app, "activate", G_CALLBACK(rp_RomDataView_app_activate), argv[argc-1]);

	// NOTE: We aren't setting up command line options in GApplication,
	// so it will complain if argv has any parameters.
	char *argv_new[1] = { argv[0] };
	status = g_application_run(G_APPLICATION(app), 1, argv_new);
#else /* !GTK_CHECK_VERSION(2,90,2) */
	// NOTE: GTK2 doesn't send a startup notification.
	// Not going to implement the Startup Notification protocol manually
	// because GTK2 desktops likely wouldn't support it, anyway.
	gtk_init(NULL, NULL);
	rp_RomDataView_app_activate(NULL, argv[argc-1]);
	gtk_main();
	status = 0;
#endif /* GTK_CHECK_VERSION(2,90,2) */

	return status;
}

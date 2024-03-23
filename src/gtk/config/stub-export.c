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
#include "xattr/XAttrView.hpp"

#include "RpGtk.h"
#include "gtk-i18n.h"

#if !GTK_CHECK_VERSION(2,90,2)
// GtkApplication was introduced in GTK3.
// For GTK2, make it a generic opaque pointer.
typedef void *GtkApplication;
#endif /* !GTK_CHECK_VERSION(2,90,2) */

#if GTK_CHECK_VERSION(4,0,0)
#  define GTK_WIDGET_SHOW_GTK3(widget)
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  define GTK_WIDGET_SHOW_GTK3(widget) gtk_widget_show(widget)
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if GTK_CHECK_VERSION(2,90,2)
static GtkApplication *app = NULL;
#endif /* GTK_CHECK_VERSION(2,90,2) */
static int status = 0;

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
	gtk_widget_set_visible(configDialog, TRUE);
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
#if GTK_CHECK_VERSION(2,90,2)
	GtkApplication *const app = gtk_application_new(
		"com.gerbilsoft.rom-properties.rp-config", G_APPLICATION_FLAGS_NONE);
	// NOTE: GApplication is supposed to set this, but KDE isn't seeing it...
	g_set_prgname("com.gerbilsoft.rom-properties.rp-config");
	g_signal_connect(app, "activate", G_CALLBACK(rp_config_app_activate), NULL);

	// NOTE: We aren't setting up command line options in GApplication,
	// so it will complain if argv has any parameters.
	char *argv_new[1] = { argv[0] };
	const int gstatus = g_application_run(G_APPLICATION(app), 1, argv_new);
	if (gstatus != 0 && status != 0) {
		status = gstatus;
	}
#else /* !GTK_CHECK_VERSION(2,90,2) */
	// NOTE: GTK2 doesn't send a startup notification.
	// Not going to implement the Startup Notification protocol manually
	// because GTK2 desktops likely wouldn't support it, anyway.
	gtk_init(NULL, NULL);
	rp_config_app_activate(NULL, 0);
	gtk_main();
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
		case GTK_RESPONSE_CLOSE:
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
	fprintf(stderr, "*** GTK" GTK_MAJOR_STR " rp_show_RomDataView_dialog(): Opening URI: '%s'\n", uri);

	// Create a GtkDialog.
	// TODO: Use GtkWindow on GTK4?
	static const char s_title[] = "RomDataView GTK" GTK_MAJOR_STR " test program";
	GtkWidget *const dialog = gtk_dialog_new_with_buttons(
		s_title,
		NULL,
		0,
		GTK_I18N_STR_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_name(dialog, "RomDataView-test-dialog");
	gtk_widget_set_visible(dialog, TRUE);

	GtkWidget *const contentArea  = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	// Create a GtkNotebook to simulate Nautilus, Thunar, etc.
	GtkWidget *const notebook = gtk_notebook_new();
	gtk_widget_set_name(notebook, "notebook");
	GTK_WIDGET_SHOW_GTK3(notebook);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(contentArea), notebook);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(contentArea), notebook, TRUE, TRUE, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// NOTE: Thunar has an extra widget between the GtkNotebook and RomDataView.
	// TODO: Have RomDataView check for this instead of expecting it when using RP_DFT_XFCE?
	GtkWidget *const vboxRomDataView = rp_gtk_vbox_new(4);
	gtk_widget_set_name(vboxRomDataView, "vboxRomDataView");
	GTK_WIDGET_SHOW_GTK3(vboxRomDataView);
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_hexpand(vboxRomDataView, TRUE);
	gtk_widget_set_vexpand(vboxRomDataView, TRUE);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	/** RomDataView **/

	// Create a RomDataView object with the specified URI.
	// TODO: Which RpDescFormatType?
	GtkWidget *const romDataView = rp_rom_data_view_new_with_uri(uri, RP_DFT_XFCE);
	GTK_WIDGET_SHOW_GTK3(romDataView);
	gtk_widget_set_name(romDataView, "romDataView");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(vboxRomDataView), romDataView);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(vboxRomDataView), romDataView, TRUE, TRUE, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Label for the tab.
	GtkWidget *const lblRomDataViewTab = gtk_label_new("ROM Properties");
	gtk_widget_set_name(lblRomDataViewTab, "lblRomDataViewTab");

	// Add the RomDataView to the GtkNotebook.
	int page_idx = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vboxRomDataView, lblRomDataViewTab);
#if GTK_CHECK_VERSION(4,0,0)
	// GtkNotebook took a reference to the tab label,
	// so we don't need to keep our reference.
	g_object_unref(lblRomDataViewTab);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// NOTE: Need to run the idle process in order for RomDataView to process the URI.
	// TODO: Create the RomData object here instead? (would need to convert to .cpp)
	// Also, we'd be able to check for RomData without having to create everything first...
	while (g_main_context_pending(NULL)) {
		g_main_context_iteration(NULL, TRUE);
	}
	if (!rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(romDataView))) {
		// Not a valid RomData object.
		fputs("*** GTK" GTK_MAJOR_STR " rp_show_RomDataView_dialog(): RomData object could not be created for this URI.\n", stderr);
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_idx);
	}

	/** XAttrView **/

	// Create a RomDataView object with the specified URI.
	// TODO: Which RpDescFormatType?
	GtkWidget *const xattrView = rp_xattr_view_new(uri);
	if (rp_xattr_view_has_attributes(RP_XATTR_VIEW(xattrView))) {
		GtkWidget *const vboxXAttrView = rp_gtk_vbox_new(4);
		gtk_widget_set_name(vboxXAttrView, "vboxXAttrView");
		GTK_WIDGET_SHOW_GTK3(vboxXAttrView);
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_set_hexpand(vboxXAttrView, TRUE);
		gtk_widget_set_vexpand(vboxXAttrView, TRUE);
#endif /* GTK_CHECK_VERSION(3,0,0) */

		GTK_WIDGET_SHOW_GTK3(xattrView);
		gtk_widget_set_name(xattrView, "xattrView");
#if GTK_CHECK_VERSION(4,0,0)
		gtk_box_append(GTK_BOX(vboxXAttrView), xattrView);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_box_pack_start(GTK_BOX(vboxXAttrView), xattrView, TRUE, TRUE, 4);
#endif /* GTK_CHECK_VERSION(4,0,0) */

		// Label for the tab.
		GtkWidget *const lblXAttrViewTab = gtk_label_new("xattrs");
		gtk_widget_set_name(lblXAttrViewTab, "lblXAttrViewTab");

		// Add the RomDataView to the GtkNotebook.
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vboxXAttrView, lblXAttrViewTab);
	} else {
		fputs("*** GTK" GTK_MAJOR_STR " rp_show_RomDataView_dialog(): No extended attributes found; not showing xattrs tab.\n", stderr);
		g_object_ref_sink(xattrView);
		g_object_unref(xattrView);
	}

	/** Rest of the dialog **/

	// Make sure we have at least one tab.
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) < 1) {
		fputs("*** GTK" GTK_MAJOR_STR " rp_show_RomDataView_dialog(): No tabs were created; exiting.\n", stderr);
		status = 1;

#if GTK_CHECK_VERSION(3,98,4)
		gtk_window_destroy(GTK_WINDOW(dialog));
#else /* !GTK_CHECK_VERSION(3,98,4) */
		gtk_widget_destroy(dialog);
#endif /* GTK_CHECK_VERSION(3,98,4) */

#if GTK_CHECK_VERSION(2,90,2)
		g_application_quit(G_APPLICATION(app));
#else /* GTK_CHECK_VERSION(2,90,2) */
		// NOTE: Calling gtk_main_quit() for GTK2 here fails:
		// Gtk-CRITICAL **: IA__gtk_main_quit: assertion 'main_loops != NULL' failed
		//gtk_main_quit();
#endif /* GTK_CHECK_VERSION(2,90,2) */

		return;
	}

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
		fputs("*** GTK" GTK_MAJOR_STR " rp_show_RomDataView_dialog(): ERROR: No URI specified.\n", stderr);
		return EXIT_FAILURE;
	}
	const char *const uri = argv[argc-1];

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
#if GTK_CHECK_VERSION(2,90,2)
	app = gtk_application_new("com.gerbilsoft.rom-properties.rp-config", G_APPLICATION_FLAGS_NONE);
	// NOTE: GApplication is supposed to set this, but KDE isn't seeing it...
	g_set_prgname("com.gerbilsoft.rom-properties.rp-config");
	g_signal_connect(app, "activate", G_CALLBACK(rp_RomDataView_app_activate), (gpointer)uri);

	// NOTE: We aren't setting up command line options in GApplication,
	// so it will complain if argv has any parameters.
	char *argv_new[1] = { argv[0] };
	const int gstatus = g_application_run(G_APPLICATION(app), 1, argv_new);
	if (gstatus != 0 && status != 0) {
		status = gstatus;
	}
#else /* !GTK_CHECK_VERSION(2,90,2) */
	// NOTE: GTK2 doesn't send a startup notification.
	// Not going to implement the Startup Notification protocol manually
	// because GTK2 desktops likely wouldn't support it, anyway.
	gtk_init(NULL, NULL);
	rp_RomDataView_app_activate(NULL, uri);
	if (status == 0)
		gtk_main();
#endif /* GTK_CHECK_VERSION(2,90,2) */

	return status;
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"

#include "ConfigDialog.hpp"
#include "RpGtk.hpp"

// for e.g. Config and FileSystem
using namespace LibRpBase;
using namespace LibRpFile;

// Tabs
#include "RpConfigTab.h"
#include "OptionsTab.hpp"

#define CONFIG_DIALOG_RESPONSE_RESET		0
#define CONFIG_DIALOG_RESPONSE_DEFAULTS		1

static void	config_dialog_dispose		(GObject	*object);
static void	config_dialog_finalize		(GObject	*object);

// Signal handlers
static void	config_dialog_response_handler	(ConfigDialog	*dialog,
						 gint		 response_id,
						 gpointer	 user_data);


// ConfigDialog class
struct _ConfigDialogClass {
	GtkDialogClass __parent__;
};

// ConfigDialog instance
struct _ConfigDialog {
	GtkDialog __parent__;

	// Buttons
	GtkWidget *btnReset;
	GtkWidget *btnDefaults;
	GtkWidget *btnApply;

	// GtkNotebook tab widget
	GtkWidget *tabWidget;

	// Tabs
	GtkWidget *tabOptions;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(ConfigDialog, config_dialog,
	GTK_TYPE_DIALOG, static_cast<GTypeFlags>(0), {});

static void
config_dialog_class_init(ConfigDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = config_dialog_dispose;
	gobject_class->finalize = config_dialog_finalize;
}

static void
config_dialog_init(ConfigDialog *dialog)
{
	// g_object_new() guarantees that all values are initialized to 0.
	gtk_window_set_title(GTK_WINDOW(dialog),
		C_("ConfigDialog", "ROM Properties Page Configuration"));

	// Add dialog buttons.
	// NOTE: GTK+ has deprecated icons on buttons, so we won't add them.
	// TODO: Proper ordering for the Apply button?
	// TODO: Spacing between "Defaults" and "Cancel".
	// NOTE: Only need to save Reset, Defaults, and Apply.
	dialog->btnReset = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&Reset")).c_str(),
		CONFIG_DIALOG_RESPONSE_RESET);
	dialog->btnDefaults = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "Defaults")).c_str(),
		CONFIG_DIALOG_RESPONSE_DEFAULTS);

	// TODO: GTK2 alignment.
	// TODO: This merely adds a space. It doesn't force the buttons
	// to the left if the window is resized.
#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *const parent = gtk_widget_get_parent(dialog->btnReset);

	GtkWidget *const lblSpacer = gtk_label_new(nullptr);
	gtk_widget_set_halign(lblSpacer, GTK_ALIGN_FILL);
	gtk_widget_show(lblSpacer);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(parent), lblSpacer);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_container_add(GTK_CONTAINER(parent), lblSpacer);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#endif /* GTK_CHECK_VERSION(3,0,0) */

	// GTK4 no longer has GTK_STOCK_*, so we'll have to provide it ourselves.
	gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&Cancel")).c_str(),
		GTK_RESPONSE_CANCEL);
	dialog->btnApply = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&Apply")).c_str(),
		GTK_RESPONSE_APPLY);
	gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&OK")).c_str(),
		GTK_RESPONSE_OK);

	// Dialog content area.
	GtkWidget *const content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	// Create the GtkNotebook.
	dialog->tabWidget = gtk_notebook_new();
	gtk_widget_show(dialog->tabWidget);
#if GTK_CHECK_VERSION(3,0,0)
	// Add some margin at the bottom.
	// TODO: GTK2 version.
	gtk_widget_set_margin_bottom(dialog->tabWidget, 8);
#endif /* GTK_CHECK_VERSION(3,0,0) */
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(content_area), dialog->tabWidget);
	// TODO: Verify that this works.
	gtk_widget_set_halign(dialog->tabWidget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(dialog->tabWidget, GTK_ALIGN_FILL);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(content_area), dialog->tabWidget, TRUE, TRUE, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Create the tabs.
	GtkWidget *lblTab = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("ConfigDialog", "&Options")).c_str());
	gtk_widget_show(lblTab);
	dialog->tabOptions = options_tab_new();
	g_object_set(dialog->tabOptions, "margin", 8, nullptr);
	gtk_widget_show(dialog->tabOptions);
	gtk_notebook_append_page(GTK_NOTEBOOK(dialog->tabWidget), dialog->tabOptions, lblTab);

	// Reset button is disabled initially.
	gtk_widget_set_sensitive(dialog->btnReset, FALSE);

	// If the first page doesn't have default settings, disable btnDefaults.
	RpConfigTab *const pTab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(GTK_NOTEBOOK(dialog->tabWidget), 0));
	assert(pTab != nullptr);
	if (pTab) {
		gtk_widget_set_sensitive(dialog->btnDefaults, rp_config_tab_has_defaults(pTab));
	}

	// Connect signals.
	g_signal_connect(dialog, "response", G_CALLBACK(config_dialog_response_handler), NULL);
}

static void
config_dialog_dispose(GObject *object)
{
	//ConfigDialog *const dialog = CONFIG_DIALOG(object);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(config_dialog_parent_class)->dispose(object);
}

static void
config_dialog_finalize(GObject *object)
{
	//ConfigDialog *const dialog = CONFIG_DIALOG(object);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(config_dialog_parent_class)->finalize(object);
}

GtkWidget*
config_dialog_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_CONFIG_DIALOG, nullptr));
}

/**
 * Close the dialog.
 * @param dialog ConfigDialog
 */
static void
config_dialog_close(ConfigDialog *dialog)
{
#if GTK_CHECK_VERSION(3,10,0)
	gtk_window_close(GTK_WINDOW(dialog));
#else /* !GTK_CHECK_VERSION(3,10,0) */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	// NOTE: This doesn't send a delete-event.
	// HACK: Call gtk_main_quit() here.
	gtk_main_quit();
#endif /* GTK_CHECK_VERSION(3,10,0) */
}

/**
 * Apply settings in all tabs.
 * @param dialog ConfigDialog
 */
static void
config_dialog_apply(ConfigDialog *dialog)
{
	// Open the configuration file using GKeyFile.
	// TODO: Error handling.
	const Config *const config = Config::instance();
	const char *filename = config->filename();
	assert(filename != nullptr);
	if (!filename) {
		// No configuration filename...
		return;
	}

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(filename);
	if (ret != 0) {
		// rmkdir() failed.
		return;
	}

	// TODO: Open a GKeyFile.
	GKeyFile *const keyFile = nullptr;

	// Save the settings.
	GtkNotebook *const tabWidget = GTK_NOTEBOOK(dialog->tabWidget);
	const int num_pages = gtk_notebook_get_n_pages(tabWidget);
	for (int i = 0; i < num_pages; i++) {
		RpConfigTab *const pTab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(tabWidget, i));
		assert(pTab != nullptr);
		if (pTab) {
			rp_config_tab_save(pTab, keyFile);
		}
	}
}

/**
 * Reset all settings to the current settings.
 * @param dialog ConfigDialog
 */
static void
config_dialog_reset(ConfigDialog *dialog)
{
	GtkNotebook *const tabWidget = GTK_NOTEBOOK(dialog->tabWidget);
	const int num_pages = gtk_notebook_get_n_pages(tabWidget);
	for (int i = 0; i < num_pages; i++) {
		RpConfigTab *const pTab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(tabWidget, i));
		assert(pTab != nullptr);
		if (pTab) {
			rp_config_tab_reset(pTab);
		}
	}
}

/**
 * Load default settings in the current tab.
 * @param dialog ConfigDialog
 */
static void
config_dialog_load_defaults(ConfigDialog *dialog)
{
	GtkNotebook *const tabWidget = GTK_NOTEBOOK(dialog->tabWidget);
	RpConfigTab *const pTab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(
		tabWidget, gtk_notebook_get_current_page(tabWidget)));
	assert(pTab != nullptr);
	if (pTab) {
		rp_config_tab_load_defaults(pTab);
	}
}

/**
 * Dialog response handler.
 * @param dialog ConfigDialog
 * @param response_id Response ID
 * @param user_data
 */
static void
config_dialog_response_handler(ConfigDialog *dialog,
			       gint          response_id,
			       gpointer      user_data)
{
	switch (response_id) {
		case GTK_RESPONSE_OK:
			// The "OK" button was clicked.
			// Save all tabs and close the dialog.
			config_dialog_apply(dialog);
			config_dialog_close(dialog);
			break;

		case GTK_RESPONSE_APPLY:
			// The "Apply" button was clicked.
			// Save all tabs.
			config_dialog_apply(dialog);
			break;

		case GTK_RESPONSE_CANCEL:
			// The "Cancel" button was clicked.
			// Close the dialog.
			config_dialog_close(dialog);
			break;

		case CONFIG_DIALOG_RESPONSE_DEFAULTS:
			// The "Defaults" button was clicked.
			// Load defaults for the current tab.
			config_dialog_load_defaults(dialog);
			break;

		case CONFIG_DIALOG_RESPONSE_RESET:
			// The "Reset" button was clicked.
			// Reset all tabs to the current settings.
			config_dialog_reset(dialog);
			break;

		default:
			break;
	}
}

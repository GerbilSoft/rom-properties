/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "config.gtk.h"

#include "ConfigDialog.hpp"
#include "RpGtk.hpp"

// for e.g. Config and FileSystem
using namespace LibRpBase;
using namespace LibRpFile;

// Tabs
#include "RpConfigTab.hpp"
#include "ImageTypesTab.hpp"
#include "SystemsTab.hpp"
#include "OptionsTab.hpp"
#include "CacheTab.hpp"
#include "AchievementsTab.hpp"
#include "AboutTab.hpp"

#ifdef ENABLE_DECRYPTION
#  include "KeyManagerTab.hpp"
#  include "librpbase/crypto/KeyManager.hpp"
using LibRpBase::KeyManager;
#endif

#define CONFIG_DIALOG_RESPONSE_RESET		0
#define CONFIG_DIALOG_RESPONSE_DEFAULTS		1

static void	config_dialog_dispose		(GObject	*object);

// Signal handlers
static void	config_dialog_response_handler	(ConfigDialog	*dialog,
						 gint		 response_id,
						 gpointer	 user_data);

static void	config_dialog_switch_page	(GtkNotebook	*tabWidget,
						 GtkWidget	*page,
						 guint		 page_num,
						 ConfigDialog	*dialog);

static void	config_dialog_tab_modified	(RpConfigTab	*tab,
						 ConfigDialog	*dialog);


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
	GtkWidget *btnCancel;
	GtkWidget *btnApply;
	GtkWidget *btnOK;

	// GtkNotebook tab widget
	GtkWidget *tabWidget;

	// Tabs
	GtkWidget *tabImageTypes;
	GtkWidget *tabSystems;
	GtkWidget *tabOptions;
	GtkWidget *tabCache;
	GtkWidget *tabAchievements;
#ifdef ENABLE_DECRYPTION
	GtkWidget *tabKeyManager;
#endif /* ENABLE_DECRYPTION */
	GtkWidget *tabAbout;

	// Signal handler IDs
	gulong tabWidget_switch_page;
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
}

static void
config_dialog_init(ConfigDialog *dialog)
{
	// g_object_new() guarantees that all values are initialized to 0.
	gtk_window_set_title(GTK_WINDOW(dialog),
		C_("ConfigDialog", "ROM Properties Page configuration"));
	gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

	// TODO: Custom icon? For now, using "media-flash".
#if GTK_CHECK_VERSION(4,0,0)
	// GTK4 has a very easy way to set an icon using the system theme.
	gtk_window_set_icon_name(GTK_WINDOW(dialog), "media-flash");
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkIconTheme *const iconTheme = gtk_icon_theme_get_default();

	// Set the window icon.
	// TODO: Redo icon if the icon theme changes?
	const uint8_t icon_sizes[] = {16, 32, 48, 64, 128};
	GList *icon_list = nullptr;
	for (int icon_size : icon_sizes) {
		GdkPixbuf *const icon = gtk_icon_theme_load_icon(
			iconTheme, "media-flash", icon_size,
			(GtkIconLookupFlags)0, nullptr);
		if (icon) {
			icon_list = g_list_prepend(icon_list, icon);
		}
	}
	gtk_window_set_icon_list(GTK_WINDOW(dialog), icon_list);
	g_list_free_full(icon_list, g_object_unref);
#endif /* GTK_CHECK_VERSION(4,0,0) */

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

	// GTK4 no longer has GTK_STOCK_*, so we'll have to provide it ourselves.
	dialog->btnCancel = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&Cancel")).c_str(),
		GTK_RESPONSE_CANCEL);
#if GTK_CHECK_VERSION(4,0,0)
	gtk_widget_set_hexpand(dialog->btnCancel, TRUE);
#endif /* GTK_CHECK_VERSION(4,0,0) */
	dialog->btnApply = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&Apply")).c_str(),
		GTK_RESPONSE_APPLY);
	gtk_widget_set_sensitive(dialog->btnApply, FALSE);
	dialog->btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog),
		convert_accel_to_gtk(C_("ConfigDialog|Button", "&OK")).c_str(),
		GTK_RESPONSE_OK);


	// Set button alignment.
	GtkWidget *const parent = gtk_widget_get_parent(dialog->btnReset);
#if GTK_CHECK_VERSION(3,96,0)
	gtk_widget_set_halign(parent, GTK_ALIGN_FILL);
	gtk_box_set_spacing(GTK_BOX(parent), 2);

	// FIXME: This doesn't seem to be working...
	gtk_widget_set_halign(dialog->btnReset, GTK_ALIGN_START);
	gtk_widget_set_halign(dialog->btnDefaults, GTK_ALIGN_START);
	gtk_widget_set_halign(dialog->btnCancel, GTK_ALIGN_END);
	gtk_widget_set_halign(dialog->btnApply, GTK_ALIGN_END);
	gtk_widget_set_halign(dialog->btnOK, GTK_ALIGN_END);
#else /* !GTK_CHECK_VERSION(3,96,0) */
	// GTK2, GTK3: May be GtkButtonBox.
	if (GTK_IS_BUTTON_BOX(parent)) {
		gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(parent), dialog->btnReset, true);
		gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(parent), dialog->btnDefaults, true);
	}
#endif /* GTK_CHECK_VERSION(3,96,0) */

	// Dialog content area.
	GtkWidget *const content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	// Create the GtkNotebook.
	dialog->tabWidget = gtk_notebook_new();
	gtk_widget_set_name(dialog->tabWidget, "tabWidget");
#ifndef RP_USE_GTK_ALIGNMENT
	// NOTE: This doesn't seem to be needed for GTK2.
	// May be a theme-specific thing...
	gtk_widget_set_margin_bottom(dialog->tabWidget, 8);
#endif /* RP_USE_GTK_ALIGNMENT */
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(content_area), dialog->tabWidget);
	// TODO: Verify that this works.
	gtk_widget_set_halign(dialog->tabWidget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(dialog->tabWidget, GTK_ALIGN_FILL);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(content_area), dialog->tabWidget, TRUE, TRUE, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#if GTK_CHECK_VERSION(4,0,0)
#  define GTK_WIDGET_SHOW_GTK3(widget)
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  define GTK_WIDGET_SHOW_GTK3(widget) gtk_widget_show(widget)
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Create the tabs.
#ifndef RP_USE_GTK_ALIGNMENT
	// GTK3/GTK4: Use the 'margin-*' properties and add the pages directly.
#define ADD_TAB(name) do { \
		gtk_widget_set_margin(dialog->tab##name, 8); \
		gtk_notebook_append_page(GTK_NOTEBOOK(dialog->tabWidget), dialog->tab##name, lbl##name); \
	} while (0)
#else
	// GTK2: Need to add GtkAlignment widgets for padding.
#define ADD_TAB(name) do { \
		GtkWidget *const align##name = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f); \
		gtk_widget_set_name(align##name, "align" #name); \
		gtk_alignment_set_padding(GTK_ALIGNMENT(align##name), 8, 8, 8, 8); \
		gtk_container_add(GTK_CONTAINER(align##name), dialog->tab##name); \
		GTK_WIDGET_SHOW_GTK3(align##name); \
		\
		gtk_notebook_append_page(GTK_NOTEBOOK(dialog->tabWidget), align##name, lbl##name); \
	} while (0)
#endif

#define CREATE_TAB(name, label, fn) do { \
		GtkWidget *const lbl##name = gtk_label_new_with_mnemonic( \
			convert_accel_to_gtk(label).c_str()); \
		gtk_widget_set_name(lbl##name, "lbl" #name); \
		GTK_WIDGET_SHOW_GTK3(lbl##name); \
		\
		dialog->tab##name = fn(); \
		gtk_widget_set_name(dialog->tab##name, "tab" #name); \
		GTK_WIDGET_SHOW_GTK3(dialog->tab##name); \
		g_signal_connect(dialog->tab##name, "modified", \
			G_CALLBACK(config_dialog_tab_modified), dialog); \
		\
		ADD_TAB(name); \
	} while (0)

	CREATE_TAB(ImageTypes, C_("ConfigDialog", "&Image Types"), image_types_tab_new);
	CREATE_TAB(Systems, C_("ConfigDialog", "&Systems"), systems_tab_new);
	CREATE_TAB(Options, C_("ConfigDialog", "&Options"), options_tab_new);
	CREATE_TAB(Cache, C_("ConfigDialog", "Thumbnail Cache"), cache_tab_new);
	CREATE_TAB(Achievements, C_("ConfigDialog", "&Achievements"), achievements_tab_new);
#ifdef ENABLE_DECRYPTION
	CREATE_TAB(KeyManager, C_("ConfigDialog", "&Key Manager"), key_manager_tab_new);
#endif /* ENABLE_DECRYPTION */
	CREATE_TAB(About, C_("ConfigDialog", "Abou&t"), about_tab_new);

	// Show the GtkNotebook.
	GTK_WIDGET_SHOW_GTK3(dialog->tabWidget);

	// Reset button is disabled initially.
	gtk_widget_set_sensitive(dialog->btnReset, FALSE);

	// Adjust btnDefaults for the first tab.
	gtk_widget_set_sensitive(dialog->btnDefaults,
		rp_config_tab_has_defaults(RP_CONFIG_TAB(dialog->tabImageTypes)));

	// FIXME: For some reason, GtkNotebook is defaulting to the
	// "Thumbnail Cache" tab on GTK3 after optimizing
	// gtk_widget_show(). Explicitly reset it to 0.
	gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->tabWidget), 0);

	// Connect signals.
	dialog->tabWidget_switch_page = g_signal_connect(
		dialog->tabWidget, "switch-page",
		G_CALLBACK(config_dialog_switch_page), dialog);

	g_signal_connect(dialog, "response", G_CALLBACK(config_dialog_response_handler), NULL);
}

static void
config_dialog_dispose(GObject *object)
{
	ConfigDialog *const dialog = CONFIG_DIALOG(object);

	// Disconnect GtkNotebook's signals.
	// Otherwise, it ends up trying to adjust btnDefaults after
	// the widget is already destroyed.
	// NOTE: g_clear_signal_handler() was added in glib-2.62.
	if (dialog->tabWidget_switch_page > 0) {
		g_signal_handler_disconnect(dialog->tabWidget, dialog->tabWidget_switch_page);
		dialog->tabWidget_switch_page = 0;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(config_dialog_parent_class)->dispose(object);
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
#if GTK_CHECK_VERSION(3,9,8)
	gtk_window_close(GTK_WINDOW(dialog));
#else /* !GTK_CHECK_VERSION(3,9,8) */
	gtk_widget_destroy(GTK_WIDGET(dialog));
	// NOTE: This doesn't send a delete-event.
	// HACK: Call gtk_main_quit() here.
	gtk_main_quit();
#endif /* GTK_CHECK_VERSION(3,9,8) */
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

	// NOTE: Ignoring load errors.
	// We're going to save anyway, even if we can't load the
	// existing file.
	GKeyFile *keyFile = g_key_file_new();
	g_key_file_load_from_file(keyFile, filename,
		(GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
		nullptr);

	// Save the settings.
	rp_config_tab_save(RP_CONFIG_TAB(dialog->tabImageTypes), keyFile);
	rp_config_tab_save(RP_CONFIG_TAB(dialog->tabSystems), keyFile);
	rp_config_tab_save(RP_CONFIG_TAB(dialog->tabOptions), keyFile);

	// Commit the changes.
	// NOTE: g_key_file_save_to_file() was added in glib-2.40.
	// We'll use g_key_file_to_data() instead.
	gsize length = 0;
	gchar *keyFileData = g_key_file_to_data(keyFile, &length, nullptr);
	if (!keyFileData) {
		// Failed to get the key file data.
		g_key_file_unref(keyFile);
		return;
	}
	FILE *f_conf = fopen(filename, "w");
	if (!f_conf) {
		// Failed to open the configuration file for writing.
		g_free(keyFileData);
		g_key_file_unref(keyFile);
		return;
	}
	fwrite(keyFileData, 1, length, f_conf);
	fclose(f_conf);
	g_free(keyFileData);
	g_key_file_unref(keyFile);

#ifdef ENABLE_DECRYPTION
	// KeyManager needs to save to keys.conf.
	const KeyManager *const keyManager = KeyManager::instance();
	filename = keyManager->filename();
	assert(filename != nullptr);
	if (filename) {
		keyFile = g_key_file_new();

		// NOTE: Ignoring load errors.
		// We're going to save anyway, even if we can't load the
		// existing file.
		g_key_file_load_from_file(keyFile, filename,
			(GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
			nullptr);

		// Save the keys.
		rp_config_tab_save(RP_CONFIG_TAB(dialog->tabKeyManager), keyFile);

		// Commit the changes.
		// NOTE: g_key_file_save_to_file() was added in glib-2.40.
		// We'll use g_key_file_to_data() instead.
		gsize length = 0;
		gchar *keyFileData = g_key_file_to_data(keyFile, &length, nullptr);
		if (!keyFileData) {
			// Failed to get the key file data.
			g_key_file_unref(keyFile);
			return;
		}
		FILE *f_conf = fopen(filename, "w");
		if (!f_conf) {
			// Failed to open the configuration file for writing.
			g_free(keyFileData);
			g_key_file_unref(keyFile);
			return;
		}
		fwrite(keyFileData, 1, length, f_conf);
		fclose(f_conf);
		g_free(keyFileData);
		g_key_file_unref(keyFile);
	}
#endif /* ENABLE_DECRYPTION */

	// Disable the "Apply" and "Reset" buttons.
	gtk_widget_set_sensitive(dialog->btnApply, FALSE);
	gtk_widget_set_sensitive(dialog->btnReset, FALSE);
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
		RpConfigTab *const tab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(tabWidget, i));
		assert(tab != nullptr);
		if (tab) {
			rp_config_tab_reset(tab);
		}
	}

	// Disable the "Apply" and "Reset" buttons.
	gtk_widget_set_sensitive(dialog->btnApply, FALSE);
	gtk_widget_set_sensitive(dialog->btnReset, FALSE);
}

/**
 * Load default settings in the current tab.
 * @param dialog ConfigDialog
 */
static void
config_dialog_load_defaults(ConfigDialog *dialog)
{
	GtkNotebook *const tabWidget = GTK_NOTEBOOK(dialog->tabWidget);
	RpConfigTab *const tab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(
		tabWidget, gtk_notebook_get_current_page(tabWidget)));
	assert(tab != nullptr);
	if (tab) {
		rp_config_tab_load_defaults(tab);
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
	RP_UNUSED(user_data);

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

/**
 * The selected tab has been changed.
 * @param tabWidget
 * @param page
 * @param page_num
 * @param dialog
 */
static void
config_dialog_switch_page(GtkNotebook *tabWidget, GtkWidget *page, guint page_num, ConfigDialog *dialog)
{
#ifndef RP_USE_GTK_ALIGNMENT
	g_return_if_fail(RP_CONFIG_IS_TAB(page));
#else /* RP_USE_GTK_ALIGNMENT */
	g_return_if_fail(GTK_IS_ALIGNMENT(page));
#endif /* RP_USE_GTK_ALIGNMENT */
	RP_UNUSED(tabWidget);
	RP_UNUSED(page_num);

#ifndef RP_USE_GTK_ALIGNMENT
	RpConfigTab *const tab = RP_CONFIG_TAB(page);
	assert(tab != nullptr);
#else /* RP_USE_GTK_ALIGNMENT */
	GtkAlignment *const align = GTK_ALIGNMENT(page);
	assert(align != nullptr);
	RpConfigTab *const tab = RP_CONFIG_TAB(gtk_bin_get_child(GTK_BIN(align)));
	assert(tab != nullptr);
#endif /* RP_USE_GTK_ALIGNMENT */
	if (tab) {
		gtk_widget_set_sensitive(dialog->btnDefaults, rp_config_tab_has_defaults(tab));
	}
}

/**
 * A tab has been modified.
 */
static void
config_dialog_tab_modified(RpConfigTab *tab, ConfigDialog *dialog)
{
	RP_UNUSED(tab);

	// Enable the "Apply" and "Reset" buttons.
	gtk_widget_set_sensitive(dialog->btnApply, TRUE);
	gtk_widget_set_sensitive(dialog->btnReset, TRUE);
}

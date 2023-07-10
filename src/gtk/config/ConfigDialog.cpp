/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ConfigDialog.hpp: Configuration dialog.                                 *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "config.gtk.h"

#include "ConfigDialog.hpp"
#include "RpGtk.hpp"
#include "gtk-i18n.h"

#include <gdk/gdkkeysyms.h>

// for e.g. Config and FileSystem
using namespace LibRpBase;
using namespace LibRpFile;

// Tabs
#include "RpConfigTab.h"
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

// Using GtkDialog on GTK2/GTK3.
// For GTK4, using GtkWindow.
#if !GTK_CHECK_VERSION(4,0,0)
#  define USE_GTK_DIALOG 1
#endif /* !GTK_CHECK_VERSION(4,0,0) */

#define CONFIG_DIALOG_RESPONSE_RESET		0
#define CONFIG_DIALOG_RESPONSE_DEFAULTS		1

static void	rp_config_dialog_dispose		(GObject	*object);

// Signal handlers
static void	rp_config_dialog_close			(RpConfigDialog *dialog,
							 gpointer	 user_data);

static void	rp_config_dialog_response_handler	(RpConfigDialog	*dialog,
							 gint		 response_id,
							 gpointer	 user_data);
#ifndef USE_GTK_DIALOG
static void	rp_config_dialog_button_handler		(GtkButton	*button,
							 RpConfigDialog	*dialog);
#endif /* !USE_GTK_DIALOG */

static void	rp_config_dialog_switch_page		(GtkNotebook	*tabWidget,
							 GtkWidget	*page,
							 guint		 page_num,
							 RpConfigDialog	*dialog);

static void	rp_config_dialog_tab_modified		(RpConfigTab	*tab,
							 RpConfigDialog	*dialog);

#ifndef USE_GTK_DIALOG
/* Signal identifiers */
typedef enum {
	SIGNAL_CLOSE,	// Dialog close request (Escape key)

	SIGNAL_LAST
} RpConfigDialogSignalID;

static GQuark response_id_quark;
static guint signals[SIGNAL_LAST];
#endif /* !USE_GTK_DIALOG */

#ifdef USE_GTK_DIALOG
typedef GtkDialogClass superclass;
typedef GtkDialog super;
#  define GTK_TYPE_SUPER GTK_TYPE_DIALOG
#else /* !USE_GTK_DIALOG */
typedef GtkWindowClass superclass;
typedef GtkWindow super;
#  define GTK_TYPE_SUPER GTK_TYPE_WINDOW
#endif /* USE_GTK_DIALOG */

// ConfigDialog class
struct _RpConfigDialogClass {
	superclass __parent__;
};

// ConfigDialog instance
struct _RpConfigDialog {
	super __parent__;

#ifndef USE_GTK_DIALOG
	GtkWidget *vboxDialog;
	GtkWidget *buttonBox;
#endif /* USE_GTK_DIALOG */

	// Buttons
	GtkWidget *btnReset;
	GtkWidget *btnDefaults;
	GtkWidget *btnCancel;
	GtkWidget *btnApply;
	GtkWidget *btnOK;

	// GtkNotebook tab widget
	GtkWidget *tabWidget;

	// Signal handler IDs
	gulong tabWidget_switch_page;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpConfigDialog, rp_config_dialog,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rp_config_dialog_class_init(RpConfigDialogClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_config_dialog_dispose;

#ifndef USE_GTK_DIALOG
	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	response_id_quark = g_quark_from_string("response-id");

	/** Signals **/

	signals[SIGNAL_CLOSE] = g_signal_new("close",
		G_OBJECT_CLASS_TYPE(gobject_class),
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);

	/** Escape key handling **/
	gtk_widget_class_add_binding_signal(GTK_WIDGET_CLASS(klass), GDK_KEY_Escape, (GdkModifierType)0, "close", nullptr);
#endif /* USE_GTK_DIALOG */
}

static void
rp_config_dialog_init(RpConfigDialog *dialog)
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
	for (const int icon_size : icon_sizes) {
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

#if GTK_CHECK_VERSION(4,0,0)
#  define GTK_WIDGET_SHOW_GTK3(widget)
#else /* !GTK_CHECK_VERSION(4,0,0) */
#  define GTK_WIDGET_SHOW_GTK3(widget) gtk_widget_show(widget)
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Dialog content area
#ifdef USE_GTK_DIALOG
	GtkWidget *const content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else /* !USE_GTK_DIALOG */
	dialog->vboxDialog = rp_gtk_vbox_new(0);
	gtk_widget_set_name(dialog->vboxDialog, "vboxDialog");
	gtk_window_set_child(GTK_WINDOW(dialog), dialog->vboxDialog);
	GTK_WIDGET_SHOW_GTK3(dialog->vboxDialog);
#endif /* USE_GTK_DIALOG */

	// Create the GtkNotebook.
	dialog->tabWidget = gtk_notebook_new();
	gtk_widget_set_name(dialog->tabWidget, "tabWidget");
#ifndef RP_USE_GTK_ALIGNMENT
	// NOTE: This doesn't seem to be needed for GTK2.
	// May be a theme-specific thing...
	gtk_widget_set_margin_bottom(dialog->tabWidget, 8);
#endif /* RP_USE_GTK_ALIGNMENT */
#if USE_GTK_DIALOG
	gtk_container_add(GTK_CONTAINER(content_area), dialog->tabWidget);
#else /* !USE_GTK_DIALOG */
	gtk_box_append(GTK_BOX(dialog->vboxDialog), dialog->tabWidget);
	// TODO: Verify that this works.
	gtk_widget_set_halign(dialog->tabWidget, GTK_ALIGN_FILL);
	gtk_widget_set_valign(dialog->tabWidget, GTK_ALIGN_FILL);
#endif /* USE_GTK_DIALOG */

	// Tab information
	struct TabInfo_t {
		// Tab title (localized, using Qt/Win32 accelerators)
		const char *title;

		// Constructor function
		GtkWidget* (*ctor_fn)(void);

		// Object names
		const char *lbl_name;
		const char *tab_name;
#ifdef RP_USE_GTK_ALIGNMENT
		const char *align_name;
#endif /* RP_USE_GTK_ALIGNMENT */
	};

#ifdef RP_USE_GTK_ALIGNMENT
#  define TAB_INFO(klass, tab_title, ctor) \
	{ tab_title, ctor, "lbl" #klass, "tab" #klass, "align" #klass }
#else /* !RP_USE_GTK_ALIGNMENT */
#  define TAB_INFO(klass, tab_title, ctor) \
	{ tab_title, ctor, "lbl" #klass, "tab" #klass }
#endif /* RP_USE_GTK_ALIGNMENT */

	static const TabInfo_t tabInfo_tbl[] = {
		TAB_INFO(ImageTypes,	NOP_C_("ConfigDialog", "&Image Types"),		rp_image_types_tab_new),
		TAB_INFO(Systems,	NOP_C_("ConfigDialog", "&Systems"),		rp_systems_tab_new),
		TAB_INFO(Options,	NOP_C_("ConfigDialog", "&Options"),		rp_options_tab_new),
		TAB_INFO(Cache,		NOP_C_("ConfigDialog", "Thumbnail Cache"),	rp_cache_tab_new),
		TAB_INFO(Achievements,	NOP_C_("ConfigDialog", "&Achievements"),	rp_achievements_tab_new),
#ifdef ENABLE_DECRYPTION
		TAB_INFO(KeyManager,	NOP_C_("ConfigDialog", "&Key Manager"),		rp_key_manager_tab_new),
#endif /* ENABLE_DECRYPTION */
		TAB_INFO(About,		NOP_C_("ConfigDialog", "Abou&t"),		rp_about_tab_new),
	};

	// Create the tabs.
	for (const auto &tabInfo : tabInfo_tbl) {
		GtkWidget *const tab_label = gtk_label_new_with_mnemonic(
			convert_accel_to_gtk(dpgettext_expr(RP_I18N_DOMAIN, "ConfigDialog", tabInfo.title)).c_str());
		gtk_widget_set_name(tab_label, tabInfo.lbl_name);
		GTK_WIDGET_SHOW_GTK3(tab_label);

		GtkWidget *const tab = tabInfo.ctor_fn();
		gtk_widget_set_name(tab, tabInfo.tab_name);
		GTK_WIDGET_SHOW_GTK3(tab);
		g_signal_connect(tab, "modified", G_CALLBACK(rp_config_dialog_tab_modified), dialog);

		// Add the tab to the GtkNotebook.
#ifndef RP_USE_GTK_ALIGNMENT
		// GTK3/GTK4: Use the 'margin-*' properties and add the pages directly.
		gtk_widget_set_margin(tab, 8);
		gtk_notebook_append_page(GTK_NOTEBOOK(dialog->tabWidget), tab, tab_label);
#else /* RP_USE_GTK_ALIGNMENT */
		// GTK2: Need to add GtkAlignment widgets for padding.
		GtkWidget *const alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
		gtk_widget_set_name(alignment, tabInfo.align_name);
		gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 8, 8, 8, 8);
		gtk_container_add(GTK_CONTAINER(alignment), tab);
		GTK_WIDGET_SHOW_GTK3(alignment);

		gtk_notebook_append_page(GTK_NOTEBOOK(dialog->tabWidget), alignment, tab_label);
#endif /* RP_USE_GTK_ALIGNMENT */
	}

	// Show the GtkNotebook.
	GTK_WIDGET_SHOW_GTK3(dialog->tabWidget);

	// FIXME: For some reason, GtkNotebook is defaulting to the
	// "Thumbnail Cache" tab on GTK3 after optimizing
	// gtk_widget_show(). Explicitly reset it to 0.
	gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->tabWidget), 0);

	// Connect the tab switch signal.
	dialog->tabWidget_switch_page = g_signal_connect(
		dialog->tabWidget, "switch-page",
		G_CALLBACK(rp_config_dialog_switch_page), dialog);

	// Dialog button box
	// NOTE: Using GtkButtonBox on GTK2/GTK3 for "secondary" button functionality.
	// NOTE: GTK+ has deprecated icons on buttons, so we won't add them.
	// TODO: Proper ordering for the Apply button?
	const std::string s_reset    = convert_accel_to_gtk(C_("ConfigDialog", "&Reset"));
	const std::string s_defaults = convert_accel_to_gtk(C_("ConfigDialog", "Defaults"));

#if USE_GTK_DIALOG
	// Secondary buttons
	dialog->btnReset = gtk_dialog_add_button(GTK_DIALOG(dialog),
		s_reset.c_str(), CONFIG_DIALOG_RESPONSE_RESET);
	dialog->btnDefaults = gtk_dialog_add_button(GTK_DIALOG(dialog),
		s_defaults.c_str(), CONFIG_DIALOG_RESPONSE_DEFAULTS);

	// GTK4 no longer has GTK_STOCK_*, so we'll have to provide it ourselves.
	dialog->btnCancel = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_I18N_STR_CANCEL, GTK_RESPONSE_CANCEL);
	dialog->btnApply = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_I18N_STR_APPLY, GTK_RESPONSE_APPLY);
	dialog->btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_I18N_STR_OK, GTK_RESPONSE_OK);

	// Set button alignment
	GtkWidget *const buttonBox = gtk_widget_get_parent(dialog->btnReset);
	assert(GTK_IS_BUTTON_BOX(buttonBox));
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(buttonBox), dialog->btnReset, true);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(buttonBox), dialog->btnDefaults, true);

	// Connect the dialog response handler.
	g_signal_connect(dialog, "response", G_CALLBACK(rp_config_dialog_response_handler), nullptr);
#else /* !USE_GTK_DIALOG */
	dialog->buttonBox = rp_gtk_hbox_new(2);
	gtk_widget_set_name(dialog->buttonBox, "buttonBox");

	// Secondary buttons
	dialog->btnReset = gtk_button_new_with_mnemonic(s_reset.c_str());
	g_object_set_qdata(G_OBJECT(dialog->btnReset), response_id_quark,
		GINT_TO_POINTER(CONFIG_DIALOG_RESPONSE_RESET));
	dialog->btnDefaults = gtk_button_new_with_mnemonic(s_defaults.c_str());
	g_object_set_qdata(G_OBJECT(dialog->btnDefaults), response_id_quark,
		GINT_TO_POINTER(CONFIG_DIALOG_RESPONSE_DEFAULTS));

	// Primary buttons
	// GTK4 no longer has GTK_STOCK_*, so we'll have to provide it ourselves.
	dialog->btnCancel = gtk_button_new_with_mnemonic(GTK_I18N_STR_CANCEL);
	g_object_set_qdata(G_OBJECT(dialog->btnCancel), response_id_quark,
		GINT_TO_POINTER(GTK_RESPONSE_CANCEL));
	gtk_widget_set_hexpand(dialog->btnCancel, TRUE);
	dialog->btnApply = gtk_button_new_with_mnemonic(GTK_I18N_STR_APPLY);
	g_object_set_qdata(G_OBJECT(dialog->btnApply), response_id_quark,
		GINT_TO_POINTER(GTK_RESPONSE_APPLY));
	dialog->btnOK = gtk_button_new_with_mnemonic(GTK_I18N_STR_OK);
	g_object_set_qdata(G_OBJECT(dialog->btnOK), response_id_quark,
		GINT_TO_POINTER(GTK_RESPONSE_OK));

	// Set button alignment.
	gtk_widget_set_halign(dialog->buttonBox, GTK_ALIGN_FILL);

	gtk_box_append(GTK_BOX(dialog->buttonBox), dialog->btnReset);
	gtk_box_append(GTK_BOX(dialog->buttonBox), dialog->btnDefaults);
	gtk_box_append(GTK_BOX(dialog->buttonBox), dialog->btnCancel);
	gtk_box_append(GTK_BOX(dialog->buttonBox), dialog->btnApply);
	gtk_box_append(GTK_BOX(dialog->buttonBox), dialog->btnOK);

	// FIXME: This doesn't seem to be working...
	gtk_widget_set_halign(dialog->btnReset, GTK_ALIGN_START);
	gtk_widget_set_halign(dialog->btnDefaults, GTK_ALIGN_START);
	gtk_widget_set_halign(dialog->btnCancel, GTK_ALIGN_END);
	gtk_widget_set_halign(dialog->btnApply, GTK_ALIGN_END);
	gtk_widget_set_halign(dialog->btnOK, GTK_ALIGN_END);

	gtk_box_append(GTK_BOX(dialog->vboxDialog), dialog->buttonBox);

	// Connect the button signals.
	g_signal_connect(dialog->btnReset,    "clicked", G_CALLBACK(rp_config_dialog_button_handler), dialog);
	g_signal_connect(dialog->btnDefaults, "clicked", G_CALLBACK(rp_config_dialog_button_handler), dialog);
	g_signal_connect(dialog->btnCancel,   "clicked", G_CALLBACK(rp_config_dialog_button_handler), dialog);
	g_signal_connect(dialog->btnApply,    "clicked", G_CALLBACK(rp_config_dialog_button_handler), dialog);
	g_signal_connect(dialog->btnOK,       "clicked", G_CALLBACK(rp_config_dialog_button_handler), dialog);
#endif /* USE_GTK_DIALOG */

	// Disbale the "Apply" button initially.
	gtk_widget_set_sensitive(dialog->btnApply, FALSE);

	// Reset button is disabled initially.
	gtk_widget_set_sensitive(dialog->btnReset, FALSE);

	// Adjust btnDefaults for the first tab.
	GtkWidget *tab_0 = gtk_notebook_get_nth_page(GTK_NOTEBOOK(dialog->tabWidget), 0);
	assert(tab_0 != nullptr);
#ifdef RP_USE_GTK_ALIGNMENT
	// tab_0 is a GtkAlignment. Get the actual RP_CONFIG_TAB.
	tab_0 = gtk_bin_get_child(GTK_BIN(tab_0));
	assert(tab_0 != nullptr);
#endif /* RP_USE_GTK_ALIGNMENT */
	gtk_widget_set_sensitive(dialog->btnDefaults,
		rp_config_tab_has_defaults(RP_CONFIG_TAB(tab_0)));

	// Escape key handler
	g_signal_connect(dialog, "close", G_CALLBACK(rp_config_dialog_close), nullptr);
}

static void
rp_config_dialog_dispose(GObject *object)
{
	RpConfigDialog *const dialog = RP_CONFIG_DIALOG(object);

	// Disconnect GtkNotebook's signals.
	// Otherwise, it ends up trying to adjust btnDefaults after
	// the widget is already destroyed.
	// NOTE: g_clear_signal_handler() was added in glib-2.62.
	if (dialog->tabWidget_switch_page > 0) {
		g_signal_handler_disconnect(dialog->tabWidget, dialog->tabWidget_switch_page);
		dialog->tabWidget_switch_page = 0;
	}

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_config_dialog_parent_class)->dispose(object);
}

GtkWidget*
rp_config_dialog_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_CONFIG_DIALOG, nullptr));
}

/**
 * Close the dialog.
 * NOTE: Also used as a signal handler.
 * @param dialog ConfigDialog
 * @param user_data
 */
static void
rp_config_dialog_close(RpConfigDialog *dialog, gpointer user_data)
{
	RP_UNUSED(user_data);

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
rp_config_dialog_apply(RpConfigDialog *dialog)
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
	// NOTE: Saving KeyManagerTab for later.
#ifdef ENABLE_DECRYPTION
	RpConfigTab *tabKeyManager = nullptr;
#endif /* ENABLE_DECRYPTION */
	const gint n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(dialog->tabWidget));
	for (gint i = 0; i < n_pages; i++) {
		RpConfigTab *const tab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(GTK_NOTEBOOK(dialog->tabWidget), i));
		assert(tab != nullptr);
#ifdef ENABLE_DECRYPTION
		if (RP_IS_KEY_MANAGER_TAB(tab)) {
			// Found KeyManagerTab.
			assert(tabKeyManager == nullptr);
			tabKeyManager = tab;
		} else
#endif /* ENABLE_DECRYPTION */
		{
			// rp_config_tab_save() checks tab's type,
			// so no need to check RP_IS_CONFIG_TAB() here.
			rp_config_tab_save(tab, keyFile);
		}
	}

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
	assert(tabKeyManager != nullptr);
	if (filename && tabKeyManager) {
		keyFile = g_key_file_new();

		// NOTE: Ignoring load errors.
		// We're going to save anyway, even if we can't load the
		// existing file.
		g_key_file_load_from_file(keyFile, filename,
			(GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
			nullptr);

		// Save the keys.
		// rp_config_tab_save() checks tab's type,
		// so no need to check RP_IS_CONFIG_TAB() here.
		rp_config_tab_save(tabKeyManager, keyFile);

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
rp_config_dialog_reset(RpConfigDialog *dialog)
{
	GtkNotebook *const tabWidget = GTK_NOTEBOOK(dialog->tabWidget);
	const int num_pages = gtk_notebook_get_n_pages(tabWidget);
	for (int i = 0; i < num_pages; i++) {
		RpConfigTab *const tab = RP_CONFIG_TAB(gtk_notebook_get_nth_page(tabWidget, i));
		assert(tab != nullptr);
		rp_config_tab_reset(tab);
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
rp_config_dialog_load_defaults(RpConfigDialog *dialog)
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
 * Dialog response handler
 * @param dialog ConfigDialog
 * @param response_id Response ID
 * @param user_data
 */
static void
rp_config_dialog_response_handler(RpConfigDialog	*dialog,
				  gint			 response_id,
				  gpointer		 user_data)
{
	RP_UNUSED(user_data);

	switch (response_id) {
		case GTK_RESPONSE_OK:
			// The "OK" button was clicked.
			// Save all tabs and close the dialog.
			rp_config_dialog_apply(dialog);
			rp_config_dialog_close(dialog, nullptr);
			break;

		case GTK_RESPONSE_APPLY:
			// The "Apply" button was clicked.
			// Save all tabs.
			rp_config_dialog_apply(dialog);
			break;

		case GTK_RESPONSE_CANCEL:
			// The "Cancel" button was clicked.
			// Close the dialog.
			rp_config_dialog_close(dialog, nullptr);
			break;

		case CONFIG_DIALOG_RESPONSE_DEFAULTS:
			// The "Defaults" button was clicked.
			// Load defaults for the current tab.
			rp_config_dialog_load_defaults(dialog);
			break;

		case CONFIG_DIALOG_RESPONSE_RESET:
			// The "Reset" button was clicked.
			// Reset all tabs to the current settings.
			rp_config_dialog_reset(dialog);
			break;

		default:
			break;
	}
}

#ifndef USE_GTK_DIALOG
/**
 * Dialog button handler (non-GtkDialog)
 * @param button GtkButton (check response_id_quark for the response ID)
 * @param dialog RpConfigDialog
 */
static void
rp_config_dialog_button_handler(GtkButton	*button,
				RpConfigDialog	*dialog)
{
	const gint response_id = GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(button), response_id_quark));

	rp_config_dialog_response_handler(dialog, response_id, nullptr);
}
#endif /* USE_GTK_DIALOG */

/**
 * The selected tab has been changed.
 * @param tabWidget
 * @param page
 * @param page_num
 * @param dialog
 */
static void
rp_config_dialog_switch_page(GtkNotebook *tabWidget, GtkWidget *page, guint page_num, RpConfigDialog *dialog)
{
#ifndef RP_USE_GTK_ALIGNMENT
	g_return_if_fail(RP_IS_CONFIG_TAB(page));
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
rp_config_dialog_tab_modified(RpConfigTab *tab, RpConfigDialog *dialog)
{
	RP_UNUSED(tab);

	// Enable the "Apply" and "Reset" buttons.
	gtk_widget_set_sensitive(dialog->btnApply, TRUE);
	gtk_widget_set_sensitive(dialog->btnReset, TRUE);
}

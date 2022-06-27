/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

#include "LanguageComboBox.hpp"

// librpbase
using namespace LibRpBase;

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
// TODO: Combine with the KDE version.
// NOTE: GTK LanguageComboBox uses a NULL-terminated pal_lc[] array.
static const uint32_t pal_lc[] = {'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru', 0};
static const int pal_lc_idx_def = 2;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// OptionsTab class
struct _OptionsTabClass {
	superclass __parent__;
};

// OptionsTab instance
struct _OptionsTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

	// Downloads
	GtkWidget *chkExtImgDownloadEnabled;
	GtkWidget *chkUseIntIconForSmallSizes;
	GtkWidget *chkDownloadHighResScans;
	GtkWidget *chkStoreFileOriginInfo;
	GtkWidget *cboGameTDBPAL;

	// Options
	GtkWidget *chkShowDangerousPermissionsOverlayIcon;
	GtkWidget *chkEnableThumbnailOnNetworkFS;
};

// Interface initialization
static void	options_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	options_tab_has_defaults			(OptionsTab	*tab);
static void	options_tab_reset				(OptionsTab	*tab);
static void	options_tab_load_defaults			(OptionsTab	*tab);
static void	options_tab_save				(OptionsTab	*tab,
								 GKeyFile       *keyFile);

// "modified" signal handler for UI widgets
static void	options_tab_modified_handler			(GtkWidget	*widget,
								 OptionsTab	*tab);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(OptionsTab, options_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			options_tab_rp_config_tab_interface_init));

static void
options_tab_class_init(OptionsTabClass *klass)
{
	RP_UNUSED(klass);
}

static void
options_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))options_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))options_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))options_tab_load_defaults;
	iface->save = (__typeof__(iface->save))options_tab_save;
}

static void
options_tab_init(OptionsTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Create the "Downloads" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraDownloads = gtk_frame_new(C_("SystemsTab", "Downloads"));
	GtkWidget *const vboxDownloads = RP_gtk_vbox_new(6);
	gtk_widget_set_margin(vboxDownloads, 6);
	gtk_frame_set_child(GTK_FRAME(fraDownloads), vboxDownloads);

	// "Downloads" checkboxes.
	tab->chkExtImgDownloadEnabled = gtk_check_button_new_with_label(
		C_("OptionsTab", "Enable external image downloads."));
	tab->chkUseIntIconForSmallSizes = gtk_check_button_new_with_label(
		C_("OptionsTab", "Always use the internal icon (if present) for small sizes."));
	tab->chkDownloadHighResScans = gtk_check_button_new_with_label(
		C_("OptionsTab", "Download high-resolution scans if viewing large thumbnails.\n"
			"This may increase bandwidth usage."));
	tab->chkStoreFileOriginInfo = gtk_check_button_new_with_label(
		C_("OptionsTab", "Store cached file origin information using extended attributes.\n"
			"This helps to identify where cached files were downloaded from."));

	// GameTDB PAL hbox.
	GtkWidget *const hboxGameTDBPAL = RP_gtk_hbox_new(6);
	GtkWidget *const lblGameTDBPAL = gtk_label_new(C_("OptionsTab", "Language for PAL titles on GameTDB:"));
	tab->cboGameTDBPAL = language_combo_box_new();
	language_combo_box_set_force_pal(LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL), true);
	language_combo_box_set_lcs(LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL), pal_lc);

	// Create the "Options" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraOptions = gtk_frame_new(C_("SystemsTab", "Options"));
	GtkWidget *const vboxOptions = RP_gtk_vbox_new(6);
	gtk_widget_set_margin(vboxOptions, 6);
	gtk_frame_set_child(GTK_FRAME(fraOptions), vboxOptions);

	// "Options" checkboxes.
	tab->chkShowDangerousPermissionsOverlayIcon = gtk_check_button_new_with_label(
		C_("OptionsTab", "Show a security overlay icon for ROM images with\n\"dangerous\" permissions."));
	tab->chkEnableThumbnailOnNetworkFS = gtk_check_button_new_with_label(
		C_("OptionsTab", "Enable thumbnailing and metadata extraction on network\n"
			"file systems. This may slow down file browsing."));

	// Connect the signal handlers for the checkboxes.
	// NOTE: Signal handlers are triggered if the value is
	// programmatically edited, unlike Qt, so we'll need to
	// inhibit handling when loading settings.
	g_signal_connect(tab->chkExtImgDownloadEnabled, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);
	g_signal_connect(tab->chkUseIntIconForSmallSizes, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);
	g_signal_connect(tab->chkDownloadHighResScans, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);
	g_signal_connect(tab->chkStoreFileOriginInfo, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);
	g_signal_connect(tab->cboGameTDBPAL, "changed",
		G_CALLBACK(options_tab_modified_handler), tab);

	g_signal_connect(tab->chkShowDangerousPermissionsOverlayIcon, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);
	g_signal_connect(tab->chkEnableThumbnailOnNetworkFS, "toggled",
		G_CALLBACK(options_tab_modified_handler), tab);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDownloads);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkExtImgDownloadEnabled);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkUseIntIconForSmallSizes);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkDownloadHighResScans);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkStoreFileOriginInfo);

	gtk_box_append(GTK_BOX(vboxDownloads), hboxGameTDBPAL);
	gtk_box_append(GTK_BOX(hboxGameTDBPAL), lblGameTDBPAL);
	gtk_box_append(GTK_BOX(hboxGameTDBPAL), tab->cboGameTDBPAL);

	gtk_box_append(GTK_BOX(tab), fraOptions);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(fraDownloads);
	gtk_widget_show(vboxDownloads);
	gtk_widget_show(tab->chkExtImgDownloadEnabled);
	gtk_widget_show(tab->chkUseIntIconForSmallSizes);
	gtk_widget_show(tab->chkDownloadHighResScans);
	gtk_widget_show(tab->chkStoreFileOriginInfo);

	gtk_widget_show(hboxGameTDBPAL);
	gtk_widget_show(lblGameTDBPAL);
	gtk_widget_show(tab->cboGameTDBPAL);

	gtk_widget_show(fraOptions);
	gtk_widget_show(vboxOptions);
	gtk_widget_show(tab->chkShowDangerousPermissionsOverlayIcon);
	gtk_widget_show(tab->chkEnableThumbnailOnNetworkFS);

	gtk_box_pack_start(GTK_BOX(tab), fraDownloads, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkExtImgDownloadEnabled, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkUseIntIconForSmallSizes, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkDownloadHighResScans, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkStoreFileOriginInfo, false, false, 0);

	gtk_box_pack_start(GTK_BOX(vboxDownloads), hboxGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), lblGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), tab->cboGameTDBPAL, false, false, 0);

	gtk_box_pack_start(GTK_BOX(tab), fraOptions, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkShowDangerousPermissionsOverlayIcon, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkEnableThumbnailOnNetworkFS, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	options_tab_reset(tab);
}

GtkWidget*
options_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_OPTIONS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
options_tab_has_defaults(OptionsTab *tab)
{
	g_return_val_if_fail(IS_OPTIONS_TAB(tab), false);
	return true;
}

static void
options_tab_reset(OptionsTab *tab)
{
	g_return_if_fail(IS_OPTIONS_TAB(tab));

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	tab->inhibit = true;

	// Downloads
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled),
		config->extImgDownloadEnabled());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
		config->useIntIconForSmallSizes());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkDownloadHighResScans),
		config->downloadHighResScans());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo),
		config->storeFileOriginInfo());

	// Options
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon),
		config->showDangerousPermissionsOverlayIcon());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS),
		config->enableThumbnailOnNetworkFS());

	// PAL language code
	const uint32_t lc = config->palLanguageForGameTDB();
	int idx = 0;
	for (; idx < ARRAY_SIZE_I(pal_lc)-1; idx++) {
		if (pal_lc[idx] == lc)
			break;
	}
	if (idx >= ARRAY_SIZE_I(pal_lc)) {
		// Out of range. Default to 'en'.
		idx = pal_lc_idx_def;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboGameTDBPAL), idx);

	tab->changed = false;
	tab->inhibit = false;
}

static void
options_tab_load_defaults(OptionsTab *tab)
{
	g_return_if_fail(IS_OPTIONS_TAB(tab));
	tab->inhibit = true;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.

	// Downloads
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool downloadHighResScans_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const int palLanguageForGameTDB_default = pal_lc_idx_def;	// cboGameTDBPAL index ('en')

	// Options
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS_default = false;
	bool isDefChanged = false;

#define COMPARE_CHK(widget, defval) \
	(gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != (defval))
#define COMPARE_CBO(widget, defval) \
	(gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != (defval))

	// Downloads
	if (COMPARE_CHK(tab->chkExtImgDownloadEnabled, extImgDownloadEnabled_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled),
			extImgDownloadEnabled_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkUseIntIconForSmallSizes, useIntIconForSmallSizes_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
			extImgDownloadEnabled_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkUseIntIconForSmallSizes, useIntIconForSmallSizes_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
			useIntIconForSmallSizes_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkDownloadHighResScans, downloadHighResScans_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkDownloadHighResScans),
			downloadHighResScans_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkStoreFileOriginInfo, storeFileOriginInfo_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo),
			storeFileOriginInfo_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboGameTDBPAL, palLanguageForGameTDB_default)) {
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(tab->cboGameTDBPAL),
			palLanguageForGameTDB_default);
		isDefChanged = true;
	}

	// Options
	if (COMPARE_CHK(tab->chkShowDangerousPermissionsOverlayIcon, showDangerousPermissionsOverlayIcon_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon),
			showDangerousPermissionsOverlayIcon_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkEnableThumbnailOnNetworkFS, enableThumbnailOnNetworkFS_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS),
			enableThumbnailOnNetworkFS_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", NULL);
	}
	tab->inhibit = false;
}

static void
options_tab_save(OptionsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_OPTIONS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.

	// Downloads
	g_key_file_set_boolean(keyFile, "Downloads", "ExtImageDownload",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled)));
	g_key_file_set_boolean(keyFile, "Downloads", "UseIntIconForSmallSizes",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes)));
	g_key_file_set_boolean(keyFile, "Downloads", "DownloadHighResScans",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkDownloadHighResScans)));
	g_key_file_set_boolean(keyFile, "Downloads", "StoreFileOriginInfo",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo)));
	g_key_file_set_string(keyFile, "Downloads", "PalLanguageForGameTDB",
		SystemRegion::lcToString(language_combo_box_get_selected_lc(
			LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL))).c_str());

	// Options
	g_key_file_set_boolean(keyFile, "Options", "ShowDangerousPermissionsOverlayIcon",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon)));
	g_key_file_set_boolean(keyFile, "Options", "EnableThumbnailOnNetworkFS",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS)));

	// Configuration saved.
	tab->changed = false;
}

/** Signal handlers **/

/**
 * "modified" signal handler for UI widgets
 * @param widget Widget sending the signal
 * @param tab OptionsTab
 */
static void
options_tab_modified_handler(GtkWidget *widget, OptionsTab *tab)
{
	RP_UNUSED(widget);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}

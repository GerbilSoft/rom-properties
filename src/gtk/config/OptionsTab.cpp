/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsTab.hpp"

#include "RpGtk.hpp"
#include "gtk-compat.h"

#include "LanguageComboBox.hpp"

static void	options_tab_dispose		(GObject	*object);
static void	options_tab_finalize		(GObject	*object);

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

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(OptionsTab, options_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
options_tab_class_init(OptionsTabClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = options_tab_dispose;
	gobject_class->finalize = options_tab_finalize;
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
	GtkWidget *const fraDownloads = gtk_frame_new(C_("ConfigDialog", "Downloads"));
	GtkWidget *const vboxDownloads = RP_gtk_vbox_new(6);
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
#if GTK_CHECK_VERSION(3,12,0)
	// Add some margins to the sides.
	// TODO: GTK2 version.
	gtk_widget_set_margin_start(hboxGameTDBPAL, 6);
	gtk_widget_set_margin_end(hboxGameTDBPAL, 6);
	gtk_widget_set_margin_bottom(hboxGameTDBPAL, 6);
#elif GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_margin_left(hboxGameTDBPAL, 6);
	gtk_widget_set_margin_right(hboxGameTDBPAL, 6);
	gtk_widget_set_margin_bottom(hboxGameTDBPAL, 6);
#endif

	// Create the "Options" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraOptions = gtk_frame_new(C_("ConfigDialog", "Options"));
	GtkWidget *const vboxOptions = RP_gtk_vbox_new(6);
	gtk_frame_set_child(GTK_FRAME(fraOptions), vboxOptions);

	// "Options" checkboxes.
	tab->chkShowDangerousPermissionsOverlayIcon = gtk_check_button_new_with_label(
		C_("OptionsTab", "Show a security overlay icon for ROM images with\n\"dangerous\" permissions."));
	tab->chkEnableThumbnailOnNetworkFS = gtk_check_button_new_with_label(
		C_("OptionsTab", "Enable thumbnailing and metadata extraction on network\n"
			"file systems. This may slow down file browsing."));

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
	gtk_widget_show(vboxDownloads);
	gtk_widget_show(tab->chkExtImgDownloadEnabled);
	gtk_widget_show(tab->chkUseIntIconForSmallSizes);
	gtk_widget_show(tab->chkDownloadHighResScans);
	gtk_widget_show(tab->chkStoreFileOriginInfo);

	gtk_widget_show(hboxGameTDBPAL);
	gtk_widget_show(lblGameTDBPAL);
	gtk_widget_show(tab->cboGameTDBPAL);

	gtk_widget_show(vboxOptions);
	gtk_widget_show(tab->chkShowDangerousPermissionsOverlayIcon);
	gtk_widget_show(tab->chkEnableThumbnailOnNetworkFS);

	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkExtImgDownloadEnabled, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkUseIntIconForSmallSizes, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkDownloadHighResScans, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkStoreFileOriginInfo, false, false, 0);

	gtk_box_pack_start(GTK_BOX(vboxDownloads), hboxGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), lblGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), tab->cboGameTDBPAL, false, false, 0);

	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkShowDangerousPermissionsOverlayIcon, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkEnableThumbnailOnNetworkFS, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Add the frames to the main VBox.
	// TODO: Fill?
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDownloads);
	gtk_box_append(GTK_BOX(tab), fraOptions);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(fraDownloads);
	gtk_widget_show(fraOptions);

	gtk_box_pack_start(GTK_BOX(tab), fraDownloads, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), fraOptions, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

static void
options_tab_dispose(GObject *object)
{
	//OptionsTab *const dialog = OPTIONS_TAB(object);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(options_tab_parent_class)->dispose(object);
}

static void
options_tab_finalize(GObject *object)
{
	//OptionsTab *const dialog = OPTIONS_TAB(object);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(options_tab_parent_class)->finalize(object);
}

GtkWidget*
options_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_OPTIONS_TAB, nullptr));
}

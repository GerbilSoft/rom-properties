/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * SystemsTab.cpp: Systems tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SystemsTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
using namespace LibRpBase;

// C++ STL classes
using std::string;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// SystemsTab class
struct _SystemsTabClass {
	superclass __parent__;
};

// SystemsTab instance
struct _SystemsTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

	GtkWidget *cboDMG;
	GtkWidget *cboSGB;
	GtkWidget *cboCGB;
};

// Interface initialization
static void	systems_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	systems_tab_has_defaults			(SystemsTab	*tab);
static void	systems_tab_reset				(SystemsTab	*tab);
static void	systems_tab_load_defaults			(SystemsTab	*tab);
static void	systems_tab_save				(SystemsTab	*tab,
								 GKeyFile       *keyFile);

// "modified" signal handler for UI widgets
static void	systems_tab_modified_handler			(GtkWidget	*widget,
								 SystemsTab	*tab);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(SystemsTab, systems_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			systems_tab_rp_config_tab_interface_init));

static void
systems_tab_class_init(SystemsTabClass *klass)
{
	RP_UNUSED(klass);
}

static void
systems_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))systems_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))systems_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))systems_tab_load_defaults;
	iface->save = (__typeof__(iface->save))systems_tab_save;
}

static void
systems_tab_init(SystemsTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Create the "Game Boy Title Screens" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraDMG = gtk_frame_new(C_("SystemsTab", "Game Boy Title Screens"));
	GtkWidget *const vboxDMG = RP_gtk_vbox_new(6);
	gtk_widget_set_margin(vboxDMG, 6);
	gtk_frame_set_child(GTK_FRAME(fraDMG), vboxDMG);

	GtkWidget *const lblDMGDescription = gtk_label_new(
		C_("SystemsTab", "Select the Game Boy model to use for title screens for different types of\nGame Boy ROM images."));
	GTK_LABEL_XALIGN_LEFT(lblDMGDescription);
	gtk_label_set_wrap(GTK_LABEL(lblDMGDescription), TRUE);

	GtkWidget *const lblDMG = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "Game &Boy:")).c_str());
	GtkWidget *const lblSGB = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "&Super Game Boy:")).c_str());
	GtkWidget *const lblCGB = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "Game Boy &Color:")).c_str());

	const string s_DMG = convert_accel_to_gtk(C_("SystemsTab", "Game Boy"));
	const string s_SGB = convert_accel_to_gtk(C_("SystemsTab", "Super Game Boy"));
	const string s_CGB = convert_accel_to_gtk(C_("SystemsTab", "Game Boy Color"));

	// GtkListStore models for the combo boxes
	GtkListStore *const lstDMG = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstDMG, nullptr, 0, 0, s_DMG.c_str(), -1);
	gtk_list_store_insert_with_values(lstDMG, nullptr, 1, 0, s_CGB.c_str(), -1);
	// NOTE: SGB and CGB have the same lists.
	GtkListStore *const lstSGB = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 0, 0, s_DMG.c_str(), -1);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 1, 0, s_SGB.c_str(), -1);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 2, 0, s_CGB.c_str(), -1);

	tab->cboDMG = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstDMG));
	g_object_unref(lstDMG);	// TODO: Is this correct?
	tab->cboSGB = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstSGB));
	tab->cboCGB = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstSGB));
	g_object_unref(lstSGB);	// TODO: Is this correct?

	// TODO: Label alignments based on DE?
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblDMG), tab->cboDMG);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblSGB), tab->cboSGB);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblCGB), tab->cboCGB);
	GTK_LABEL_XALIGN_RIGHT(lblDMG);
	GTK_LABEL_XALIGN_RIGHT(lblSGB);
	GTK_LABEL_XALIGN_RIGHT(lblCGB);

	// Create the cell renderers.
	// NOTE: Using GtkComboBoxText would make this somewhat easier,
	// but then we can't share the SGB/CGB GtkListStores.
	GtkCellRenderer *column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tab->cboDMG), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tab->cboDMG),
		column, "text", 0, nullptr);

	column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tab->cboSGB), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tab->cboSGB),
		column, "text", 0, nullptr);

	column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tab->cboCGB), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tab->cboCGB),
		column, "text", 0, nullptr);

	// Connect the signal handlers for the comboboxes.
	// NOTE: Signal handlers are triggered if the value is
	// programmatically edited, unlike Qt, so we'll need to
	// inhibit handling when loading settings.
	g_signal_connect(tab->cboDMG, "changed",
		G_CALLBACK(systems_tab_modified_handler), tab);
	g_signal_connect(tab->cboSGB, "changed",
		G_CALLBACK(systems_tab_modified_handler), tab);
	g_signal_connect(tab->cboCGB, "changed",
		G_CALLBACK(systems_tab_modified_handler), tab);

	// GtkGrid/GtkTable
#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *const table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 2);
	gtk_grid_set_column_spacing(GTK_GRID(table), 8);

	// TODO: GTK_FILL
	gtk_grid_attach(GTK_GRID(table), lblDMG, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboDMG, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(table), lblSGB, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboSGB, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(table), lblCGB, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboCGB, 1, 2, 1, 1);
#else /* GTK_CHECK_VERSION(3,0,0) */
	GtkWidget *const table = gtk_table_new(3, 2, false);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	gtk_table_attach(GTK_TABLE(table), lblDMG, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboDMG, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), lblSGB, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboSGB, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), lblCGB, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboCGB, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
#endif /* GTK_CHECK_VERSION(3,0,0) */

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDMG);

	gtk_box_append(GTK_BOX(vboxDMG), lblDMGDescription);
	gtk_box_append(GTK_BOX(vboxDMG), table);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(vboxDMG);
	gtk_widget_show(lblDMGDescription);

	gtk_widget_show(table);
	gtk_widget_show(lblDMG);
	gtk_widget_show(tab->cboDMG);
	gtk_widget_show(lblSGB);
	gtk_widget_show(tab->cboSGB);
	gtk_widget_show(lblCGB);
	gtk_widget_show(tab->cboCGB);

	gtk_box_pack_start(GTK_BOX(vboxDMG), lblDMGDescription, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDMG), table, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Add the frames to the main VBox.
	// TODO: Fill?
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDMG);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(fraDMG);
	gtk_box_pack_start(GTK_BOX(tab), fraDMG, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	systems_tab_reset(tab);
}

GtkWidget*
systems_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_SYSTEMS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
systems_tab_has_defaults(SystemsTab *tab)
{
	g_return_val_if_fail(IS_SYSTEMS_TAB(tab), false);
	return true;
}

static void
systems_tab_reset(SystemsTab *tab)
{
	g_return_if_fail(IS_SYSTEMS_TAB(tab));

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	tab->inhibit = true;

	// Special handling: DMG as SGB doesn't really make sense,
	// so handle it as DMG.
	const Config::DMG_TitleScreen_Mode tsMode =
		config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_DMG);
	switch (tsMode) {
		case Config::DMG_TitleScreen_Mode::DMG_TS_DMG:
		case Config::DMG_TitleScreen_Mode::DMG_TS_SGB:
		default:
			gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboDMG), 0);
			break;
		case Config::DMG_TitleScreen_Mode::DMG_TS_CGB:
			gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboDMG), 1);
			break;
	}

	// The SGB and CGB dropdowns have all three.
	gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboSGB),
		(int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_SGB));
	gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboCGB),
		(int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_CGB));

	tab->changed = false;
	tab->inhibit = false;
}

static void
systems_tab_load_defaults(SystemsTab *tab)
{
	g_return_if_fail(IS_SYSTEMS_TAB(tab));
	tab->inhibit = true;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const int8_t idxDMG_default = 0;
	static const int8_t idxSGB_default = 1;
	static const int8_t idxCGB_default = 2;
	bool isDefChanged = false;

#define COMPARE_CBO(widget, defval) \
	(gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != (defval))

	if (COMPARE_CBO(tab->cboDMG, idxDMG_default)) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboDMG), idxDMG_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboSGB, idxSGB_default)) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboSGB), idxSGB_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboCGB, idxCGB_default)) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(tab->cboCGB), idxCGB_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", NULL);
	}
	tab->inhibit = false;
}

static void
systems_tab_save(SystemsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_SYSTEMS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.

	static const char s_dmg_dmg[][4] = {"DMG", "CGB"};
	const int idxDMG = gtk_combo_box_get_active(GTK_COMBO_BOX(tab->cboDMG));
	assert(idxDMG >= 0);
	assert(idxDMG < ARRAY_SIZE_I(s_dmg_dmg));
	if (idxDMG >= 0 && idxDMG < ARRAY_SIZE_I(s_dmg_dmg)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "DMG", s_dmg_dmg[idxDMG]);
	}

	static const char s_dmg_other[][4] = {"DMG", "SGB", "CGB"};
	const int idxSGB = gtk_combo_box_get_active(GTK_COMBO_BOX(tab->cboSGB));
	const int idxCGB = gtk_combo_box_get_active(GTK_COMBO_BOX(tab->cboCGB));

	assert(idxSGB >= 0);
	assert(idxSGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxSGB >= 0 && idxSGB < ARRAY_SIZE_I(s_dmg_other)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "SGB", s_dmg_other[idxSGB]);
	}
	assert(idxCGB >= 0);
	assert(idxCGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxCGB >= 0 && idxCGB < ARRAY_SIZE_I(s_dmg_other)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "CGB", s_dmg_other[idxCGB]);
	}

	// Configuration saved.
	tab->changed = false;
}

/** Signal handlers **/

/**
 * "modified" signal handler for UI widgets
 * @param widget Widget sending the signal
 * @param tab SystemsTab
 */
static void
systems_tab_modified_handler(GtkWidget *widget, SystemsTab *tab)
{
	RP_UNUSED(widget);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}

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

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// SystemsTab class
struct _RpSystemsTabClass {
	superclass __parent__;
};

// SystemsTab instance
struct _RpSystemsTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

	GtkWidget *cboDMG;
	GtkWidget *cboSGB;
	GtkWidget *cboCGB;
};

// Interface initialization
static void	rp_systems_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	rp_systems_tab_has_defaults			(RpSystemsTab	*tab);
static void	rp_systems_tab_reset				(RpSystemsTab	*tab);
static void	rp_systems_tab_load_defaults			(RpSystemsTab	*tab);
static void	rp_systems_tab_save				(RpSystemsTab	*tab,
								 GKeyFile       *keyFile);

#ifdef USE_GTK_DROP_DOWN
static void	rp_systems_tab_notify_selected_handler		(GtkDropDown	*cbo,
								 GParamSpec	*pspec,
								 RpSystemsTab	*tab);
#else /* !USE_GTK_DROP_DOWN */
// "modified" signal handler for UI widgets
static void	rp_systems_tab_modified_handler			(GtkComboBox	*cbo,
								 RpSystemsTab	*tab);
#endif /* USE_GTK_DROP_DOWN */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpSystemsTab, rp_systems_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_systems_tab_rp_config_tab_interface_init));

static void
rp_systems_tab_class_init(RpSystemsTabClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_systems_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_systems_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_systems_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))rp_systems_tab_load_defaults;
	iface->save = (__typeof__(iface->save))rp_systems_tab_save;
}

static void
rp_systems_tab_init(RpSystemsTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Create the "Game Boy Title Screens" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraDMG = gtk_frame_new(C_("SystemsTab", "Game Boy Title Screens"));
	gtk_widget_set_name(fraDMG, "fraDMG");
	GtkWidget *const vboxDMG = rp_gtk_vbox_new(6);
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_margin(vboxDMG, 6);
	gtk_frame_set_child(GTK_FRAME(fraDMG), vboxDMG);
#else /* !GTK_CHECK_VERSION(2,91,0) */
	GtkWidget *const alignDMG = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignDMG, "alignDMG");
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignDMG), 6, 6, 6, 6);
	gtk_widget_show(alignDMG);
	gtk_container_add(GTK_CONTAINER(alignDMG), vboxDMG);
	gtk_frame_set_child(GTK_FRAME(fraDMG), alignDMG);
#endif /* GTK_CHECK_VERSION(2,91,0) */

	// FIXME: Better wrapping that doesn't require manual newlines.
	GtkWidget *const lblDMGDescription = gtk_label_new(
		C_("SystemsTab", "Select the Game Boy model to use for title screens for different types of\nGame Boy ROM images."));
	gtk_widget_set_name(lblDMGDescription, "lblDMGDescription");
	GTK_LABEL_XALIGN_LEFT(lblDMGDescription);
	gtk_label_set_wrap(GTK_LABEL(lblDMGDescription), TRUE);

	GtkWidget *const lblDMG = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "Game &Boy:")).c_str());
	GtkWidget *const lblSGB = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "&Super Game Boy:")).c_str());
	GtkWidget *const lblCGB = gtk_label_new_with_mnemonic(
		convert_accel_to_gtk(C_("SystemsTab", "Game Boy &Color:")).c_str());

	gtk_widget_set_name(lblDMG, "lblDMG");
	gtk_widget_set_name(lblSGB, "lblSGB");
	gtk_widget_set_name(lblCGB, "lblCGB");

	const char *const s_DMG = C_("SystemsTab", "Game Boy");
	const char *const s_SGB = C_("SystemsTab", "Super Game Boy");
	const char *const s_CGB = C_("SystemsTab", "Game Boy Color");

#ifdef USE_GTK_DROP_DOWN
	// GtkStringList models for the GtkDropDowns
	GtkStringList *const lstDMG = gtk_string_list_new(nullptr);
	gtk_string_list_append(lstDMG, s_DMG);
	gtk_string_list_append(lstDMG, s_CGB);
	// NOTE: SGB and CGB have the same lists.
	GtkStringList *const lstSGB = gtk_string_list_new(nullptr);
	gtk_string_list_append(lstSGB, s_DMG);
	gtk_string_list_append(lstSGB, s_SGB);
	gtk_string_list_append(lstSGB, s_CGB);

	// NOTE: gtk_drop_down_new() takes ownership of the GListModel.
	// For cboCGB, we'll need to take an additional reference.
	tab->cboDMG = gtk_drop_down_new(G_LIST_MODEL(lstDMG), nullptr);
	tab->cboSGB = gtk_drop_down_new(G_LIST_MODEL(lstSGB), nullptr);
	tab->cboCGB = gtk_drop_down_new(G_LIST_MODEL(g_object_ref(lstSGB)), nullptr);
#else /* !USE_GTK_DROP_DOWN */
	// GtkListStore models for the combo boxes
	GtkListStore *const lstDMG = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstDMG, nullptr, 0, 0, s_DMG, -1);
	gtk_list_store_insert_with_values(lstDMG, nullptr, 1, 0, s_CGB, -1);
	// NOTE: SGB and CGB have the same lists.
	GtkListStore *const lstSGB = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 0, 0, s_DMG, -1);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 1, 0, s_SGB, -1);
	gtk_list_store_insert_with_values(lstSGB, nullptr, 2, 0, s_CGB, -1);

	// GtkComboBox takes a reference to the GtkListStore.
	// We'll need to remove our own reference afterwards.
	tab->cboDMG = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstDMG));
	g_object_unref(lstDMG);
	tab->cboSGB = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstSGB));
	tab->cboCGB = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstSGB));
	g_object_unref(lstSGB);

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
#endif /* USE_GTK_DROP_DOWN */

	gtk_widget_set_name(tab->cboDMG, "cboDMG");
	gtk_widget_set_name(tab->cboSGB, "cboSGB");
	gtk_widget_set_name(tab->cboCGB, "cboCGB");

	// TODO: Label alignments based on DE?
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblDMG), tab->cboDMG);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblSGB), tab->cboSGB);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lblCGB), tab->cboCGB);
	GTK_LABEL_XALIGN_RIGHT(lblDMG);
	GTK_LABEL_XALIGN_RIGHT(lblSGB);
	GTK_LABEL_XALIGN_RIGHT(lblCGB);

	// Connect the signal handlers for the comboboxes.
	// NOTE: Signal handlers are triggered if the value is
	// programmatically edited, unlike Qt, so we'll need to
	// inhibit handling when loading settings.
#ifdef USE_GTK_DROP_DOWN
	g_signal_connect(tab->cboDMG, "notify::selected", G_CALLBACK(rp_systems_tab_notify_selected_handler), tab);
	g_signal_connect(tab->cboSGB, "notify::selected", G_CALLBACK(rp_systems_tab_notify_selected_handler), tab);
	g_signal_connect(tab->cboCGB, "notify::selected", G_CALLBACK(rp_systems_tab_notify_selected_handler), tab);
#else /* !USE_GTK_DROP_DOWN */
	g_signal_connect(tab->cboDMG, "changed", G_CALLBACK(rp_systems_tab_modified_handler), tab);
	g_signal_connect(tab->cboSGB, "changed", G_CALLBACK(rp_systems_tab_modified_handler), tab);
	g_signal_connect(tab->cboCGB, "changed", G_CALLBACK(rp_systems_tab_modified_handler), tab);
#endif /* USE_GTK_DROP_DOWN */

	// GtkGrid/GtkTable
#ifdef USE_GTK_GRID
	GtkWidget *const table = gtk_grid_new();
	gtk_widget_set_name(table, "table");
	gtk_grid_set_row_spacing(GTK_GRID(table), 2);
	gtk_grid_set_column_spacing(GTK_GRID(table), 8);

	// TODO: GTK_FILL
	gtk_grid_attach(GTK_GRID(table), lblDMG, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboDMG, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(table), lblSGB, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboSGB, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(table), lblCGB, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(table), tab->cboCGB, 1, 2, 1, 1);
#else /* !USE_GTK_GRID */
	GtkWidget *const table = gtk_table_new(3, 2, false);
	gtk_widget_set_name(table, "table");
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	gtk_table_attach(GTK_TABLE(table), lblDMG, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboDMG, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), lblSGB, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboSGB, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), lblCGB, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_table_attach(GTK_TABLE(table), tab->cboCGB, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

#if !GTK_CHECK_VERSION(3,0,0)
	// GTK2: Vertically-center the labels. (Not needed for GTK3/GTK4...)
	gfloat xalign;
	gtk_misc_get_alignment(GTK_MISC(lblDMG), &xalign, nullptr);
	gtk_misc_set_alignment(GTK_MISC(lblDMG), xalign, 0.5f);
	gtk_misc_get_alignment(GTK_MISC(lblSGB), &xalign, nullptr);
	gtk_misc_set_alignment(GTK_MISC(lblSGB), xalign, 0.5f);
	gtk_misc_get_alignment(GTK_MISC(lblCGB), &xalign, nullptr);
	gtk_misc_set_alignment(GTK_MISC(lblCGB), xalign, 0.5f);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

#endif /* USE_GTK_GRID */

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDMG);

	gtk_box_append(GTK_BOX(vboxDMG), lblDMGDescription);
	gtk_box_append(GTK_BOX(vboxDMG), table);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(table);
	gtk_widget_show(lblDMG);
	gtk_widget_show(tab->cboDMG);
	gtk_widget_show(lblSGB);
	gtk_widget_show(tab->cboSGB);
	gtk_widget_show(lblCGB);
	gtk_widget_show(tab->cboCGB);

	gtk_box_pack_start(GTK_BOX(tab), fraDMG, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDMG), lblDMGDescription, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDMG), table, false, false, 0);

	gtk_widget_show_all(fraDMG);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	rp_systems_tab_reset(tab);
}

GtkWidget*
rp_systems_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_SYSTEMS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_systems_tab_has_defaults(RpSystemsTab *tab)
{
	g_return_val_if_fail(RP_IS_SYSTEMS_TAB(tab), FALSE);
	return TRUE;
}

static void
rp_systems_tab_reset(RpSystemsTab *tab)
{
	g_return_if_fail(RP_IS_SYSTEMS_TAB(tab));

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
			SET_CBO(tab->cboDMG, 0);
			break;
		case Config::DMG_TitleScreen_Mode::DMG_TS_CGB:
			SET_CBO(tab->cboDMG, 1);
			break;
	}

	// The SGB and CGB dropdowns have all three.
	SET_CBO(tab->cboSGB, (int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_SGB));
	SET_CBO(tab->cboCGB, (int)config->dmgTitleScreenMode(Config::DMG_TitleScreen_Mode::DMG_TS_CGB));

	tab->changed = false;
	tab->inhibit = false;
}

static void
rp_systems_tab_load_defaults(RpSystemsTab *tab)
{
	g_return_if_fail(RP_IS_SYSTEMS_TAB(tab));
	tab->inhibit = true;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.
	static const int8_t idxDMG_default = 0;
	static const int8_t idxSGB_default = 1;
	static const int8_t idxCGB_default = 2;
	bool isDefChanged = false;

	if (COMPARE_CBO(tab->cboDMG, idxDMG_default)) {
		SET_CBO(tab->cboDMG, idxDMG_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboSGB, idxSGB_default)) {
		SET_CBO(tab->cboSGB, idxSGB_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboCGB, idxCGB_default)) {
		SET_CBO(tab->cboCGB, idxCGB_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", NULL);
	}
	tab->inhibit = false;
}

static void
rp_systems_tab_save(RpSystemsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_SYSTEMS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.

	static const char s_dmg_dmg[][4] = {"DMG", "CGB"};
	static const char s_dmg_other[][4] = {"DMG", "SGB", "CGB"};

	const int idxDMG = GET_CBO(tab->cboDMG);
	assert(idxDMG >= 0);
	assert(idxDMG < ARRAY_SIZE_I(s_dmg_dmg));
	if (idxDMG >= 0 && idxDMG < ARRAY_SIZE_I(s_dmg_dmg)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "DMG", s_dmg_dmg[idxDMG]);
	}

	const int idxSGB = GET_CBO(tab->cboSGB);
	assert(idxSGB >= 0);
	assert(idxSGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxSGB >= 0 && idxSGB < ARRAY_SIZE_I(s_dmg_other)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "SGB", s_dmg_other[idxSGB]);
	}

	const int idxCGB = GET_CBO(tab->cboCGB);
	assert(idxCGB >= 0);
	assert(idxCGB < ARRAY_SIZE_I(s_dmg_other));
	if (idxCGB >= 0 && idxCGB < ARRAY_SIZE_I(s_dmg_other)) {
		g_key_file_set_string(keyFile, "DMGTitleScreenMode", "CGB", s_dmg_other[idxCGB]);
	}

	// Configuration saved.
	tab->changed = false;
}

/** Signal handlers **/

#ifdef USE_GTK_DROP_DOWN
/**
 * Internal GObject "notify" signal handler for GtkDropDown's "selected" property.
 * @param cbo GtkDropDown sending the signal
 * @param pspec Property specification
 * @param widget RpLanguageComboBox
 */
static void
rp_systems_tab_notify_selected_handler(GtkDropDown *dropDown, GParamSpec *pspec, RpSystemsTab *tab)
{
	RP_UNUSED(dropDown);
	RP_UNUSED(pspec);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}
#else /* !USE_GTK_DROP_DOWN */
/**
 * Internal "modified" signal handler for GtkComboBox.
 * @param cbo GtkComboBox sending the signal
 * @param tab SystemsTab
 */
static void
rp_systems_tab_modified_handler(GtkComboBox *cbo, RpSystemsTab *tab)
{
	RP_UNUSED(cbo);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}
#endif /* USE_GTK_DROP_DOWN */

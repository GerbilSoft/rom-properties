/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * AchievementsTab.cpp: Achievements tab for rp-config.                    *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "AchievementsTab.hpp"
#include "RpConfigTab.h"
#include "../AchGDBus.hpp"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
#include "librpbase/Achievements.hpp"
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

// AchievementsTab class
struct _AchievementsTabClass {
	superclass __parent__;
};

// AchievementsTab instance
struct _AchievementsTab {
	super __parent__;

	GtkListStore	*listStore;
	GtkWidget	*treeView;
};

// Interface initialization
static void	achievements_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	achievements_tab_has_defaults			(AchievementsTab	*tab);
static void	achievements_tab_reset				(AchievementsTab	*tab);
static void	achievements_tab_load_defaults			(AchievementsTab	*tab);
static void	achievements_tab_save				(AchievementsTab	*tab,
							 GKeyFile       *keyFile);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(AchievementsTab, achievements_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			achievements_tab_rp_config_tab_interface_init));

static void
achievements_tab_class_init(AchievementsTabClass *klass)
{ }

static void
achievements_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))achievements_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))achievements_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))achievements_tab_load_defaults;
	iface->save = (__typeof__(iface->save))achievements_tab_save;
}

static void
achievements_tab_init(AchievementsTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	// Create the GtkListStore and GtkTreeView.
	tab->listStore = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_STRING);
	tab->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tab->listStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), tab->treeView);

	// Column 1: Icon
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("AchievementsTab", "Icon"));
	gtk_tree_view_column_set_resizable(column, FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);
	
	// Column 2: Achievement
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("AchievementsTab", "Achievement"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "markup", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

	// Column 3: Unlock Time
	// TODO: Store as a string, or as a GDateTime?
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("AchievementsTab", "Unlock Time"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), scrolledWindow);

	// FIXME: This isn't working; the GtkScrolledWindow is too small...
	gtk_widget_set_halign(scrolledWindow, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrolledWindow, GTK_ALIGN_FILL);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(scrolledWindow);
	gtk_widget_show(tab->treeView);

	gtk_box_pack_start(GTK_BOX(tab), scrolledWindow, true, true, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the achievements.
	achievements_tab_reset(tab);
}

GtkWidget*
achievements_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_ACHIEVEMENTS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
achievements_tab_has_defaults(AchievementsTab *tab)
{
	g_return_val_if_fail(IS_ACHIEVEMENTS_TAB(tab), false);
	return false;
}

static void
achievements_tab_reset(AchievementsTab *tab)
{
	g_return_if_fail(IS_ACHIEVEMENTS_TAB(tab));
	gtk_list_store_clear(tab->listStore);

	// Load the Achievements icon sprite sheet.
	// NOTE: Assuming 32x32 icons for now.
	// TODO: Check DPI and adjust on DPI changes?
	const gint iconSize = 32;
	PIMGTYPE imgAchSheet = AchGDBus::loadSpriteSheet(iconSize, false);
	PIMGTYPE imgAchGraySheet = AchGDBus::loadSpriteSheet(iconSize, true);

	const Achievements *const pAch = Achievements::instance();
	for (int i = 0; i < (int)Achievements::ID::Max; i++) {
		// Is the achievement unlocked?
		const Achievements::ID id = (Achievements::ID)i;
		const time_t timestamp = pAch->isUnlocked(id);
		const bool unlocked = (timestamp != -1);

		// Determine row and column.
		const int col = i % Achievements::ACH_SPRITE_SHEET_COLS;
		const int row = i / Achievements::ACH_SPRITE_SHEET_COLS;

		// Extract the sub-icon.
		PIMGTYPE subIcon = PIMGTYPE_get_subsurface(
			(unlocked ? imgAchSheet : imgAchGraySheet),
			col*iconSize, row*iconSize, iconSize, iconSize);
		assert(subIcon != nullptr);

		// Get the name and description.
		gchar *const s_ach_name = g_markup_escape_text(pAch->getName(id), -1);
		gchar *const s_ach_desc_unlocked = g_markup_escape_text(pAch->getDescUnlocked(id), -1);
		string s_ach = s_ach_name;
		s_ach += "\n<span size='75%'>";
		// TODO: Locked description?
		s_ach += s_ach_desc_unlocked;
		s_ach += "</span>";
		g_free(s_ach_name);
		g_free(s_ach_desc_unlocked);

		// Add the list item.
		GtkTreeIter treeIter;
		gtk_list_store_append(tab->listStore, &treeIter);
		gtk_list_store_set(tab->listStore, &treeIter,
			0, subIcon, 1, s_ach.c_str(), -1);
		PIMGTYPE_destroy(subIcon);

		if (unlocked) {
			// Format the unlock time.
			GDateTime *const dateTime = g_date_time_new_from_unix_utc(timestamp);
			assert(dateTime != nullptr);
			if (dateTime) {
				gchar *const str = g_date_time_format(dateTime, "%x %X");
				if (str) {
					gtk_list_store_set(tab->listStore, &treeIter, 2, str, -1);
					g_free(str);
				}
				g_date_time_unref(dateTime);
			}
		}
	}
}

static void
achievements_tab_load_defaults(AchievementsTab *tab)
{
	g_return_if_fail(IS_ACHIEVEMENTS_TAB(tab));

	// Not implemented.
	return;
}

static void
achievements_tab_save(AchievementsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_ACHIEVEMENTS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	// Not implemented.
	return;
}

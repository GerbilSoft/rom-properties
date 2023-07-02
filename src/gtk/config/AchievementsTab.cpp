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

#include "../AchSpritesheet.hpp"

#include "gtk-compat.h"
#include "RpGtk.hpp"

#ifdef USE_GTK_COLUMN_VIEW
#  include "AchievementItem.h"
#endif /* USE_GTK_COLUMN_VIEW */

// librpbase
#include "librpbase/Achievements.hpp"
using namespace LibRpBase;

// C++ STL classes
using std::string;

/* Column identifiers */
typedef enum {
	ACH_COL_ICON,
	ACH_COL_DESCRIPTION,
	ACH_COL_UNLOCK_TIME,

	ACH_COL_MAX
} AchievementColumns;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// RpAchievementsTab class
struct _RpAchievementsTabClass {
	superclass __parent__;
};

// RpAchievementsTab instance
struct _RpAchievementsTab {
	super __parent__;

#ifdef USE_GTK_COLUMN_VIEW
	GListStore	*listStore;
	GtkWidget	*columnView;
#else /* !USE_GTK_COLUMN_VIEW */
	GtkListStore	*listStore;
	GtkWidget	*treeView;
#endif /* USE_GTK_COLUMN_VIEW */
};

// Interface initialization
static void	rp_achievements_tab_rp_config_tab_interface_init	(RpConfigTabInterface	*iface);
static gboolean	rp_achievements_tab_has_defaults			(RpAchievementsTab	*tab);
static void	rp_achievements_tab_reset				(RpAchievementsTab	*tab);
static void	rp_achievements_tab_save				(RpAchievementsTab	*tab,
									 GKeyFile		*keyFile);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpAchievementsTab, rp_achievements_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_achievements_tab_rp_config_tab_interface_init));

static void
rp_achievements_tab_class_init(RpAchievementsTabClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_achievements_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_achievements_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_achievements_tab_reset;
	iface->load_defaults = nullptr;
	iface->save = (__typeof__(iface->save))rp_achievements_tab_save;
}

#ifdef USE_GTK_COLUMN_VIEW
// GtkSignalListItemFactory signal handlers
// Reference: https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// NOTE: user_data will indicate the column number: 0 == icon, 1 == description, 2 == unlock time
static void
setup_listitem_cb(GtkListItemFactory	*factory,
		  GtkListItem		*list_item,
		  gpointer		 user_data)
{
	RP_UNUSED(factory);

	switch (GPOINTER_TO_INT(user_data)) {
		case ACH_COL_ICON:
			gtk_list_item_set_child(list_item, gtk_image_new());
			//gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
			break;

		case ACH_COL_DESCRIPTION:
		case ACH_COL_UNLOCK_TIME: {
			GtkWidget *const label = gtk_label_new(nullptr);
			gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
			gtk_list_item_set_child(list_item, label);
			break;
		}

		default:
			assert(!"Invalid column number");
			return;
	}
}

static void
bind_listitem_cb(GtkListItemFactory	*factory,
                 GtkListItem		*list_item,
		 gpointer		 user_data)
{
	RP_UNUSED(factory);

	GtkWidget *const widget = gtk_list_item_get_child(list_item);
	assert(widget != nullptr);
	if (!widget) {
		return;
	}

	RpAchievementItem *const item = RP_ACHIEVEMENT_ITEM(gtk_list_item_get_item(list_item));
	if (!item) {
		return;
	}

	switch (GPOINTER_TO_INT(user_data)) {
		case ACH_COL_ICON:
			// Icon
			gtk_image_set_from_pixbuf(GTK_IMAGE(widget), rp_achievement_item_get_icon(item));
			break;

		case ACH_COL_DESCRIPTION:
			// Description
			gtk_label_set_markup(GTK_LABEL(widget), rp_achievement_item_get_description(item));
			break;

		case ACH_COL_UNLOCK_TIME: {
			// Unlock time
			bool is_set = false;

			const time_t unlock_time = rp_achievement_item_get_unlock_time(item);
			const bool unlocked = (unlock_time != -1);
			if (unlocked) {
				GDateTime *const dateTime = g_date_time_new_from_unix_local(unlock_time);
				assert(dateTime != nullptr);
				if (dateTime) {
					gchar *const str = g_date_time_format(dateTime, "%x %X");
					if (str) {
						gtk_label_set_text(GTK_LABEL(widget), str);
						g_free(str);
						is_set = true;
					}
					g_date_time_unref(dateTime);
				}
			}

			if (!is_set) {
				gtk_label_set_text(GTK_LABEL(widget), nullptr);
			}
			break;
		}
	}
}
#endif /* USE_GTK_COLUMN_VIEW */

static void
rp_achievements_tab_init(RpAchievementsTab *tab)
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
	gtk_widget_set_name(scrolledWindow, "scrolledWindow");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2,91,1)
	gtk_widget_set_halign(scrolledWindow, GTK_ALIGN_FILL);
	gtk_widget_set_valign(scrolledWindow, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(scrolledWindow, TRUE);
	gtk_widget_set_vexpand(scrolledWindow, TRUE);
#endif /* GTK_CHECK_VERSION(2,91,1) */

	// Column titles
	static const char *const column_titles[ACH_COL_MAX] = {
		NOP_C_("AchievementsTab", "Icon"),
		NOP_C_("AchievementsTab", "Achievement"),
		NOP_C_("AchievementsTab", "Unlock Time"),
	};
	// Column resizability
	static const bool column_resizable[ACH_COL_MAX] = {false, true, true};

#ifdef USE_GTK_COLUMN_VIEW
	// Create the GListStore and GtkColumnView.
	// NOTE: Each column will need its own GtkColumnViewColumn and GtkSignalListItemFactory.
	tab->listStore = g_list_store_new(RP_TYPE_ACHIEVEMENT_ITEM);
	tab->columnView = gtk_column_view_new(nullptr);
	gtk_widget_set_name(tab->columnView, "columnView");
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), tab->columnView);

	// GtkColumnView requires a GtkSelectionModel, so we'll create
	// a GtkSingleSelection to wrap around the GListStore.
	GtkSingleSelection *selModel = gtk_single_selection_new(G_LIST_MODEL(tab->listStore));
	gtk_column_view_set_model(GTK_COLUMN_VIEW(tab->columnView), GTK_SELECTION_MODEL(selModel));

	// NOTE: Regarding object ownership:
	// - GtkColumnViewColumn takes ownership of the GtkListItemFactory
	// - GtkColumnView takes ownership of the GtkColumnViewColumn
	// As such, neither the factory nor the column objects will be unref()'d here.

	// Create the columns.
	for (int i = 0; i < ACH_COL_MAX; i++) {
		GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb), GINT_TO_POINTER(i));
		g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), GINT_TO_POINTER(i));

		GtkColumnViewColumn *const column = gtk_column_view_column_new(
			dpgettext_expr(RP_I18N_DOMAIN, "AchievementsTab", column_titles[i]), factory);
		gtk_column_view_column_set_resizable(column, static_cast<gboolean>(column_resizable[i]));
		gtk_column_view_append_column(GTK_COLUMN_VIEW(tab->columnView), column);
	}
#else /* !USE_GTK_COLUMN_VIEW */
	// Create the GtkListStore and GtkTreeView.
	tab->listStore = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_STRING);
	tab->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tab->listStore));
	gtk_widget_set_name(tab->treeView, "treeView");
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), tab->treeView);

	// Property to use for each column
	static const char *const column_property_names[ACH_COL_MAX] = {
		GTK_CELL_RENDERER_PIXBUF_PROPERTY, "markup", "text"
	};

	// Create the columns.
	// NOTE: Unlock Time is stored as a string, not as a GDateTime or Unix timestamp.
	for (int i = 0; i < ACH_COL_MAX; i++) {
		GtkTreeViewColumn *const column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column,
			dpgettext_expr(RP_I18N_DOMAIN, "AchievementsTab", column_titles[i]));
		gtk_tree_view_column_set_resizable(column, static_cast<gboolean>(column_resizable[i]));

		GtkCellRenderer *const renderer = (i == 0 ? gtk_cell_renderer_pixbuf_new() : gtk_cell_renderer_text_new());
		gtk_tree_view_column_pack_start(column, renderer, FALSE);
		gtk_tree_view_column_add_attribute(column, renderer, column_property_names[i], i);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);
	}
#endif /* USE_GTK_COLUMN_VIEW */

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), scrolledWindow);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(tab), scrolledWindow, true, true, 0);

	gtk_widget_show_all(scrolledWindow);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the achievements.
	rp_achievements_tab_reset(tab);
}

GtkWidget*
rp_achievements_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_ACHIEVEMENTS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_achievements_tab_has_defaults(RpAchievementsTab *tab)
{
	g_return_val_if_fail(RP_IS_ACHIEVEMENTS_TAB(tab), FALSE);
	return FALSE;
}

static void
rp_achievements_tab_reset(RpAchievementsTab *tab)
{
	g_return_if_fail(RP_IS_ACHIEVEMENTS_TAB(tab));

#if USE_GTK_COLUMN_VIEW
	g_list_store_remove_all(tab->listStore);
#else /*USE_GTK_COLUMN_VIEW */
	gtk_list_store_clear(tab->listStore);
#endif /* USE_GTK_COLUMN_VIEW */

	// Load the Achievements icon sprite sheet.
	// NOTE: Assuming 32x32 icons for now.
	// TODO: Check DPI and adjust on DPI changes?
	const gint iconSize = 32;
	PIMGTYPE imgAchSheet = AchSpritesheet::load(iconSize, false);
	PIMGTYPE imgAchGraySheet = AchSpritesheet::load(iconSize, true);

	// Pango 1.49.0 [2021/08/22] added percentage sizes.
	// For older versions, we'll need to use 'smaller' instead.
	// Note that compared to the KDE version, 'smaller' is slightly
	// big, and 'smaller'+'smaller' is too small.
	const char *const span_start_line2 = (pango_version() >= PANGO_VERSION_ENCODE(1,49,0))
		? "\n<span size='75%'>"
		: "\n<span size='smaller'>";

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
		s_ach += span_start_line2;
		// TODO: Locked description?
		s_ach += s_ach_desc_unlocked;
		s_ach += "</span>";
		g_free(s_ach_name);
		g_free(s_ach_desc_unlocked);

		// Add the list item.
#ifdef USE_GTK_COLUMN_VIEW
		g_list_store_append(tab->listStore, rp_achievement_item_new(subIcon, s_ach.c_str(), timestamp));
		PIMGTYPE_unref(subIcon);
#else /* !USE_GTK_COLUMN_VIEW */
		GtkTreeIter treeIter;
		gtk_list_store_append(tab->listStore, &treeIter);
		gtk_list_store_set(tab->listStore, &treeIter,
			0, subIcon, 1, s_ach.c_str(), -1);
		PIMGTYPE_unref(subIcon);

		if (unlocked) {
			// Format the unlock time.
			GDateTime *const dateTime = g_date_time_new_from_unix_local(timestamp);
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
#endif /* USE_GTK_COLUMN_VIEW */
	}
}

static void
rp_achievements_tab_save(RpAchievementsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_ACHIEVEMENTS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	// Not implemented.
	return;
}

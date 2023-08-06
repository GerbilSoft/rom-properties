/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab_gtk3.hpp: Key Manager tab for rp-config. (GTK2/GTK3)      *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyManagerTab.hpp"
#include "KeyManagerTab_p.hpp"

#include "KeyStoreItem.h"

// C++ STL classes
using std::vector;

// Other rom-properties libraries
using LibRomData::KeyStoreUI;

/* Column identifiers */
typedef enum {
	KEY_COL_NAME,
	KEY_COL_VALUE,
	KEY_COL_VALID,
	//KEY_COL_FLAT_IDX,	// not visible

	KEY_COL_MAX
} KeyManagerColumns;

#if 0
/**
 * GWeakNotify function to destroy the GtkTreeListModel when the GtkColumnView is destroyed.
 * @param data GtkTreeListModel
 * @param where_the_object_was GtkColumnView
 */
static void
rp_key_manager_tab_GWeakNotify_GtkTreeView(gpointer data, GObject *where_the_object_was)
{
	RP_UNUSED(where_the_object_was);

	g_return_if_fail(GTK_IS_TREE_STORE(data));
	g_object_unref(data);
	delete tab->vSectionListStore
}
#endif

/**
 * Create GListModels for nodes when expanded.
 * @param item
 * @param user_data
 */
static GListModel*
rp_key_manager_tab_create_child_GListModel(gpointer item, gpointer user_data)
{
	// TODO
	g_return_val_if_fail(RP_IS_KEY_STORE_ITEM(item), nullptr);
	RpKeyStoreItem *const ksitem = RP_KEY_STORE_ITEM(item);
	if (!rp_key_store_item_get_is_section(ksitem)) {
		// Not a section header. No child items.
		return nullptr;
	}

	RpKeyManagerTab *const tab = RP_KEY_MANAGER_TAB(user_data);
	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	const int sectCount = keyStoreUI->sectCount();

	const int sectIdx = rp_key_store_item_get_flat_idx(ksitem);
	assert(sectIdx >= 0);
	assert(sectIdx < sectCount);
	assert(tab->vSectionListStore);
	assert(sectIdx <= static_cast<int>(tab->vSectionListStore->size()));

	if (tab->vSectionListStore && sectIdx <= static_cast<int>(tab->vSectionListStore->size())) {
		return G_LIST_MODEL(tab->vSectionListStore->at(sectIdx));
	}
	return nullptr;
}

// GtkSignalListItemFactory signal handlers
// Reference: https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// NOTE: user_data will indicate the column number: 0 == name, 1 == value, 2 == valid?
static void
setup_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	switch (GPOINTER_TO_INT(user_data)) {
		case KEY_COL_NAME:
		case KEY_COL_VALUE: {
			GtkWidget *const label = gtk_label_new(nullptr);
			gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
			gtk_list_item_set_child(list_item, label);
			break;
		}

		case KEY_COL_VALID:
			gtk_list_item_set_child(list_item, gtk_image_new());
			//gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
			break;

		default:
			assert(!"Invalid column number");
			return;
	}
}

static void
bind_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	GtkWidget *const widget = gtk_list_item_get_child(list_item);
	assert(widget != nullptr);
	if (!widget)
		return;

	RpKeyStoreItem *const item = RP_KEY_STORE_ITEM(gtk_list_item_get_item(list_item));
	if (!item)
		return;

	switch (GPOINTER_TO_INT(user_data)) {
		case KEY_COL_NAME:
			// Key name
			// TODO: Some indication that this is a section item, and if so,
			// span it across all columns.
			gtk_label_set_text(GTK_LABEL(widget), rp_key_store_item_get_name(item));
			break;

		case KEY_COL_VALUE:
			// Value
			gtk_label_set_text(GTK_LABEL(widget), rp_key_store_item_get_value(item));
			break;

		case KEY_COL_VALID:
			// Valid?
			// TODO: Validity icon.
			//gtk_image_set_from_pixbuf(GTK_IMAGE(widget), rp_achievement_item_get_icon(item));
			break;

		default:
			assert(!"Invalid column number");
			return;
	}
}

/**
 * Create the GtkTreeListModel and GtkColumnView. (GTK4)
 * @param tab RpKeyManagerTab
 */
void rp_key_manager_tab_create_GtkTreeView(RpKeyManagerTab *tab)
{
	// Create the GtkTreeListModel and GtkColumnView.
	// Columns: Key Name, Value, Valid?, Flat Key Index
	// NOTE: "Valid?" column contains an icon name.

	// GListSTore for the root list.
	// This contains the sections.
	// NOTE: Using RpKeyStoreItem for sections in order to reuse
	// flat-idx as the section index.
	tab->rootListStore = g_list_store_new(RP_TYPE_KEY_STORE_ITEM);

	tab->treeListModel = gtk_tree_list_model_new(
		G_LIST_MODEL(tab->rootListStore),		// root
		true,						// passthrough
		true, 						// autoexpand
		rp_key_manager_tab_create_child_GListModel,	// create_func
		tab,						// user_data
		nullptr);					// user_destroy

	// Create the GtkColumnView.
	tab->columnView = gtk_column_view_new(nullptr);
	gtk_widget_set_name(tab->columnView, "columnView");
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tab->scrolledWindow), tab->columnView);

	// GtkColumnView requires a GtkSelectionModel, so we'll create
	// a GtkSingleSelection to wrap around the GListStore.
	GtkSingleSelection *const selModel = gtk_single_selection_new(G_LIST_MODEL(tab->treeListModel));
	gtk_column_view_set_model(GTK_COLUMN_VIEW(tab->columnView), GTK_SELECTION_MODEL(selModel));

	// Column titles
	static const char *const column_titles[KEY_COL_MAX] = {
		NOP_C_("KeyManagerTab", "Key Name"),
		NOP_C_("KeyManagerTab", "Value"),
		NOP_C_("KeyManagerTab", "Valid?"),
	};

	// NOTE: Regarding object ownership:
	// - GtkColumnViewColumn takes ownership of the GtkListItemFactory
	// - GtkColumnView takes ownership of the GtkColumnViewColumn
	// As such, neither the factory nor the column objects will be unref()'d here.

	// Create the columns.
	for (int i = 0; i < KEY_COL_MAX; i++) {
		GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb), GINT_TO_POINTER(i));
		g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), GINT_TO_POINTER(i));

		GtkColumnViewColumn *const column = gtk_column_view_column_new(
			dpgettext_expr(RP_I18N_DOMAIN, "KeyManagerTab", column_titles[i]), factory);
		gtk_column_view_column_set_resizable(column, true);
		gtk_column_view_append_column(GTK_COLUMN_VIEW(tab->columnView), column);
	}

#if 0
	// Column 2: Value
	// TODO: Monospace font
	// TODO: Handle the cell editor's 'insert-text' signal and stop it
	// if the entered text is non-hex. (with allowKanji support)
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Value"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "family", "Monospace", nullptr);
	g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, nullptr);
	g_object_set(renderer, "editable", TRUE, nullptr);
	g_signal_connect(renderer, "edited", G_CALLBACK(renderer_edited_signal_handler), tab);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

	// Column 3: Valid?
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Valid?"));
	gtk_tree_view_column_set_resizable(column, FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "xalign", 0.5f, nullptr);	// FIXME: Not working on GTK2.
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "icon-name", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

	// Dummy column to shrink Column 3.
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);
#endif
}

/**
 * Initialize keys in the GtkTreeView.
 * This initializes sections and key names.
 * Key values and "Valid?" are initialized by reset().
 * @param tab KeyManagerTab
 */
void rp_key_manager_tab_init_keys(RpKeyManagerTab *tab)
{
	// Clear the GListStore containing root nodes. (section names)
	g_list_store_remove_all(tab->rootListStore);
	// Clear the vector of GListModels for child nodes.
	if (tab->vSectionListStore) {
		for (GListStore *listStore : *(tab->vSectionListStore)) {
			g_object_unref(listStore);
		}
		tab->vSectionListStore->clear();
	}

	if (!tab->vSectionListStore) {
		// TODO: Ensure this gets deleted when the GtkTreeListModel is destroyed.
		tab->vSectionListStore = new vector<GListStore*>();
	}

	// FIXME: GtkTreeView doesn't have anything equivalent to
	// Qt's QTreeView::setFirstColumnSpanned().

	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	const int sectCount = keyStoreUI->sectCount();
	tab->vSectionListStore->reserve(sectCount);
	int idx = 0;	// flat key index
	for (int sectIdx = 0; sectIdx < sectCount; sectIdx++) {
		GListStore *const listStore = g_list_store_new(RP_TYPE_KEY_STORE_ITEM);

		const int keyCount = keyStoreUI->keyCount(sectIdx);
		for (int keyIdx = 0; keyIdx < keyCount; keyIdx++, idx++) {
			const KeyStoreUI::Key *const key = keyStoreUI->getKey(sectIdx, keyIdx);
			// NOTE: Only key name and flat key index are added here.
			// Value and Valid? are set by the KeyStoreGTK signal handlers.
			g_list_store_append(listStore, rp_key_store_item_new_key(key->name.c_str(), nullptr, false, idx));
		}
		tab->vSectionListStore->emplace_back(listStore);

		// Add the root list node now that the child node has been created.
		g_list_store_append(tab->rootListStore, rp_key_store_item_new_section(keyStoreUI->sectName(sectIdx), nullptr, sectIdx));
	}

	// Expand all of the sections initially.
	// TODO
	//gtk_tree_view_expand_all(GTK_TREE_VIEW(tab->treeView));
}

/** KeyStoreGTK signal handlers **/

static const char *const is_valid_icon_name_tbl[] = {
	nullptr,		// Empty
	"dialog-question",	// Unknown
	"dialog-error",		// NotAKey
	"dialog-error",		// Incorrect
	"dialog-ok-apply",	// OK
};

/**
 * A key in the KeyStore has changed.
 * @param keyStore KeyStoreGTK
 * @param sectIdx Section index
 * @param keyIdx Key index
 * @param tab KeyManagerTab
 */
void keyStore_key_changed_signal_handler(RpKeyStoreGTK *keyStore, int sectIdx, int keyIdx, RpKeyManagerTab *tab)
{
	// TODO
#if 0
	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(keyStore);
	GtkTreeModel *const treeModel = GTK_TREE_MODEL(tab->treeStore);

	// Get the iterator from a path.
	GtkTreeIter treeIterKey;
	GtkTreePath *const path = gtk_tree_path_new_from_indices(sectIdx, keyIdx, -1);
	if (!gtk_tree_model_get_iter(treeModel, &treeIterKey, path)) {
		// Path not found...
		assert(!"GtkTreePath not found!");
		gtk_tree_path_free(path);
		return;
	}
	gtk_tree_path_free(path);

	const KeyStoreUI::Key *const key = keyStoreUI->getKey(sectIdx, keyIdx);
	assert(key != nullptr);
	if (!key) {
		return;
	}

	const char *icon_name = nullptr;
	if (/*(int)key->status >= 0 &&*/ (int)key->status < ARRAY_SIZE_I(is_valid_icon_name_tbl)) {
		icon_name = is_valid_icon_name_tbl[(int)key->status];
	}

	gtk_tree_store_set(tab->treeStore, &treeIterKey,
		1, key->value.c_str(),	// value
		2, icon_name, -1);	// Valid?
#endif
}

/**
 * All keys in the KeyStore have changed.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
void keyStore_all_keys_changed_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab)
{
	// TODO
#if 0
	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(keyStore);
	GtkTreeModel *const treeModel = GTK_TREE_MODEL(tab->treeStore);

	// Load the key values and "Valid?" icons.
	GtkTreeIter treeIterSect;
	for (bool validSect = gtk_tree_model_get_iter_first(treeModel, &treeIterSect);
	     validSect; validSect = gtk_tree_model_iter_next(treeModel, &treeIterSect))
	{
		// treeIterSect points to a section.
		// Iterate over all keys in the section.
		GtkTreeIter treeIterKey;
		for (bool validKey = gtk_tree_model_iter_children(treeModel, &treeIterKey, &treeIterSect);
		     validKey; validKey = gtk_tree_model_iter_next(treeModel, &treeIterKey))
		{
			// Get the flat key index.
			GValue gv_idx = G_VALUE_INIT;
			gtk_tree_model_get_value(treeModel, &treeIterKey, 3, &gv_idx);
			if (G_VALUE_HOLDS_INT(&gv_idx)) {
				const int idx = g_value_get_int(&gv_idx);
				const KeyStoreUI::Key *const key = keyStoreUI->getKey(idx);
				assert(key != nullptr);
				if (!key) {
					g_value_unset(&gv_idx);
					continue;
				}

				const char *icon_name = nullptr;
				if (/*(int)key->status >= 0 &&*/ (int)key->status < ARRAY_SIZE_I(is_valid_icon_name_tbl)) {
					icon_name = is_valid_icon_name_tbl[(int)key->status];
				}

				gtk_tree_store_set(tab->treeStore, &treeIterKey,
					1, key->value.c_str(),	// value
					2, icon_name, -1);	// Valid?
			}
			g_value_unset(&gv_idx);
		}
	}
#endif
}
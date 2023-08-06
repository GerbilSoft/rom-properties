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

// Other rom-properties libraries
using LibRomData::KeyStoreUI;

// GtkCellRendererText signal handlers
static void	renderer_edited_signal_handler(GtkCellRendererText	*self,
					       gchar			*path,
					       gchar			*new_text,
					       RpKeyManagerTab		*tab);

/**
 * GWeakNotify function to destroy the GtkTreeStore when the GtkTreeView is destroyed.
 * @param data RpKeyManagerTab
 * @param where_the_object_was GtkTreeView
 */
static void
rp_key_manager_tab_GWeakNotify_GtkTreeView(gpointer data, GObject *where_the_object_was)
{
	RP_UNUSED(where_the_object_was);
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(data));

	RpKeyManagerTab *const tab = RP_KEY_MANAGER_TAB(data);
	if (tab) {
		g_clear_object(&tab->treeStore);
	}
}

/**
 * Create the GtkTreeStore and GtkTreeView. (GTK2/GTK3)
 * @param tab RpKeyManagerTab
 */
void rp_key_manager_tab_create_GtkTreeView(RpKeyManagerTab *tab)
{
	// Create the GtkTreeStore and GtkTreeView.
	// Columns: Key Name, Value, Valid?, Flat Key Index
	// NOTE: "Valid?" column contains an icon name.
	tab->treeStore = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	tab->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tab->treeStore));
	gtk_widget_set_name(tab->treeView, "treeView");
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tab->scrolledWindow), tab->treeView);

	// Maintain a weak reference so we can destroy treeStore when treeView is destroyed.
	g_object_weak_ref(G_OBJECT(tab->treeView), rp_key_manager_tab_GWeakNotify_GtkTreeView, tab);

	// Column 1: Key Name
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Key Name"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);
	
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
}

/**
 * Initialize keys in the GtkTreeView.
 * This initializes sections and key names.
 * Key values and "Valid?" are initialized by reset().
 * @param tab KeyManagerTab
 */
void rp_key_manager_tab_init_keys(RpKeyManagerTab *tab)
{
	gtk_tree_store_clear(tab->treeStore);

	// FIXME: GtkTreeView doesn't have anything equivalent to
	// Qt's QTreeView::setFirstColumnSpanned().

	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	const int sectCount = keyStoreUI->sectCount();
	int idx = 0;	// flat key index
	for (int sectIdx = 0; sectIdx < sectCount; sectIdx++) {
		GtkTreeIter treeIterSect;
		gtk_tree_store_append(tab->treeStore, &treeIterSect, nullptr);
		gtk_tree_store_set(tab->treeStore, &treeIterSect,
			0, keyStoreUI->sectName(sectIdx), -1);

		const int keyCount = keyStoreUI->keyCount(sectIdx);
		for (int keyIdx = 0; keyIdx < keyCount; keyIdx++, idx++) {
			const KeyStoreUI::Key *const key = keyStoreUI->getKey(sectIdx, keyIdx);
			GtkTreeIter treeIterKey;
			gtk_tree_store_append(tab->treeStore, &treeIterKey, &treeIterSect);
			gtk_tree_store_set(tab->treeStore, &treeIterKey,
				0, key->name.c_str(),	// key name
				3, idx, -1);		// flat key index
		}
	}

	// Expand all of the sections initially.
	gtk_tree_view_expand_all(GTK_TREE_VIEW(tab->treeView));
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
	if (!key)
		return;

	const char *icon_name = nullptr;
	if (/*(int)key->status >= 0 &&*/ (int)key->status < ARRAY_SIZE_I(is_valid_icon_name_tbl)) {
		icon_name = is_valid_icon_name_tbl[(int)key->status];
	}

	gtk_tree_store_set(tab->treeStore, &treeIterKey,
		1, key->value.c_str(),	// value
		2, icon_name, -1);	// Valid?
}

/**
 * All keys in the KeyStore have changed.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
void keyStore_all_keys_changed_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab)
{
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
}

/** GtkCellRendererText signal handlers **/

static void
renderer_edited_signal_handler(GtkCellRendererText *self, gchar *path, gchar *new_text, RpKeyManagerTab *tab)
{
	RP_UNUSED(self);
	RP_UNUSED(tab);

	KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);

	// Convert the path to sectIdx/keyIdx.
	int sectIdx, keyIdx; char dummy;
	int ret = sscanf(path, "%d:%d%c", &sectIdx, &keyIdx, &dummy);
	assert(ret == 2);
	if (ret != 2) {
		// Path was not in the expected format.
		return;
	}

	// NOTE: GtkCellRendererText won't update the treeStore itself.
	// If the key is valid, KeyStoreUI will emit a 'key-changed' signal,
	// and our 'key-changed' signal handler will update the treeStore.
	keyStoreUI->setKey(sectIdx, keyIdx, new_text);
}

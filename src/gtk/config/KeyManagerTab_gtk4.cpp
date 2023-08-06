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
using std::string;
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

/**
 * GWeakNotify function to destroy the GtkTreeListModel and related when the GtkColumnView is destroyed.
 * @param data RpKeyManagerTab
 * @param where_the_object_was GtkColumnView
 */
static void
rp_key_manager_tab_GWeakNotify_GtkColumnView(gpointer data, GObject *where_the_object_was)
{
	RP_UNUSED(where_the_object_was);
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(data));

	RpKeyManagerTab *const tab = RP_KEY_MANAGER_TAB(data);
	if (tab) {
		// NOTE: treeListModel takes ownership of rootListStore
		// and all child GListModels, so we should *not* attempt
		// to g_object_unref() them. Just NULL them out.
		tab->rootListStore = nullptr;

		delete tab->vSectionListStore;
		tab->vSectionListStore = nullptr;

		// Delete treeListModel, which will also delete the
		// GListModels that were NULLed out above.
		g_clear_object(&tab->treeListModel);
	}
}

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

/**
 * GtkEditableLabel was changed.
 * @param self GtkEditable*
 * @param tab RpKeyManagerTab (user_data)
 */
static void
gtkEditableLabel_changed(GtkEditable *self, RpKeyManagerTab *tab)
{
	// NOTE: We can't use user_data for the flat key index because
	// GtkColumnView reuses widgets. The flat key index is stored
	// as a property when the data is bound in bind_listitem_cb().

	// NOTE: The property is incremented by 1 because a default
	// GtkEditableLabel will return NULL (0).

	if (gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(self))) {
		// Currently editing the label. Don't do anything.
		return;
	}
	int idx = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(self), KeyManagerTab_flatKeyIdx_quark));
	if (idx <= 0)
		return;

	// Subtract 1 to get the actual flat key index.
	idx--;

	// Update the key store.
	KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	keyStoreUI->setKey(idx, gtk_editable_get_text(self));
}

// GtkSignalListItemFactory signal handlers
// Reference: https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// NOTE: user_data will indicate the column number: 0 == name, 1 == value, 2 == valid?
static void
setup_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	switch (GPOINTER_TO_INT(user_data)) {
		case KEY_COL_NAME: {
			GtkWidget *const label = gtk_label_new(nullptr);
			gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
			gtk_list_item_set_child(list_item, label);
			break;
		}

		case KEY_COL_VALUE: {
			// NOTE: GtkEditableLabel doesn't like nullptr strings.
			GtkWidget *const label = gtk_editable_label_new("");
			//g_object_set(label, "xalign", 0.0f, nullptr);
			gtk_list_item_set_child(list_item, label);
			g_signal_connect(label, "changed", G_CALLBACK(gtkEditableLabel_changed),
				g_object_get_qdata(G_OBJECT(factory), KeyManagerTab_self_quark));

			// Set a monospace font.
			gtk_widget_add_css_class(label, "gsrp_monospace");
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

	static const char *const is_valid_icon_name_tbl[] = {
		nullptr,		// Empty
		"dialog-question",	// Unknown
		"dialog-error",		// NotAKey
		"dialog-error",		// Incorrect
		"dialog-ok-apply",	// OK
	};

	GtkWidget *const widget = gtk_list_item_get_child(list_item);
	assert(widget != nullptr);
	if (!widget)
		return;

	RpKeyStoreItem *const ksitem = RP_KEY_STORE_ITEM(gtk_list_item_get_item(list_item));
	if (!ksitem)
		return;

	switch (GPOINTER_TO_INT(user_data)) {
		case KEY_COL_NAME:
			// Key name
			// FIXME: GtkColumnView isn't indenting child items...
			if (unlikely(rp_key_store_item_get_is_section(ksitem))) {
				// Section header. Use it as-is.
				// TODO: Column spanning?
				gtk_label_set_text(GTK_LABEL(widget), rp_key_store_item_get_name(ksitem));
			} else {
				// Key name. Indent it a bit because GtkColumnView isn't.
				string str("\t");
				str += rp_key_store_item_get_name(ksitem);
				gtk_label_set_text(GTK_LABEL(widget), str.c_str());
			}
			break;

		case KEY_COL_VALUE: {
			// Value
			// NOTE: GtkEditableLabel doesn't like nullptr strings.
			const char *s_value = rp_key_store_item_get_value(ksitem);
			// NOTE: +1 because a default GtkEditableLabel will return NULL (0).
			const int idx = likely(!rp_key_store_item_get_is_section(ksitem))
				? rp_key_store_item_get_flat_idx(ksitem)+1
				: 0;

			printf("editable set text: %s\n", s_value);
			gtk_editable_set_text(GTK_EDITABLE(widget), s_value ? s_value : "");
			g_object_set_qdata(G_OBJECT(widget), KeyManagerTab_flatKeyIdx_quark, GINT_TO_POINTER(idx));
			break;
		}

		case KEY_COL_VALID: {
			// Valid?
			const uint8_t status = rp_key_store_item_get_status(ksitem);
			const char *const icon_name = likely(status < ARRAY_SIZE(is_valid_icon_name_tbl))
				? is_valid_icon_name_tbl[status]
				: nullptr;
			gtk_image_set_from_icon_name(GTK_IMAGE(widget), icon_name);
			break;
		}

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

	// GListStore for the root list.
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

	// Maintain a weak reference so we can destroy the GtkTreeListModel and related when columnView is destroyed.
	g_object_weak_ref(G_OBJECT(tab->columnView), rp_key_manager_tab_GWeakNotify_GtkColumnView, tab);

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
		g_object_set_qdata(G_OBJECT(factory), KeyManagerTab_self_quark, tab);
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

	assert(sectIdx >= 0);
	assert(sectIdx <= static_cast<int>(tab->vSectionListStore->size()));
	if (sectIdx < 0 || sectIdx >= static_cast<int>(tab->vSectionListStore->size()))
		return;

	GListStore *const listStore = tab->vSectionListStore->at(sectIdx);
	assert(keyIdx >= 0);
	assert(keyIdx < static_cast<int>(g_list_model_get_n_items(G_LIST_MODEL(listStore))));
	if (keyIdx < 0 || keyIdx >= static_cast<int>(g_list_model_get_n_items(G_LIST_MODEL(listStore))))
		return;

	RpKeyStoreItem *const ksitem = RP_KEY_STORE_ITEM(g_list_model_get_item(G_LIST_MODEL(listStore), keyIdx));
	assert(ksitem != nullptr);
	if (!ksitem)
		return;

	const KeyStoreUI::Key *const key = keyStoreUI->getKey(sectIdx, keyIdx);
	assert(key != nullptr);
	if (!key)
		return;

	rp_key_store_item_set_value(ksitem, key->value.c_str());
	rp_key_store_item_set_status(ksitem, static_cast<uint8_t>(key->status));
}

/**
 * All keys in the KeyStore have changed.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
void keyStore_all_keys_changed_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab)
{
	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(keyStore);

	// Load the key values and statuses.
	for (GListStore *listStore : *(tab->vSectionListStore)) {
		// Iterate over all keys in this section.
		const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(listStore));
		for (guint i = 0; i < n_items; i++) {
			RpKeyStoreItem *const ksitem = RP_KEY_STORE_ITEM(
				g_list_model_get_item(G_LIST_MODEL(listStore), i));
			assert(ksitem != nullptr);
			if (!ksitem)
				continue;

			const int idx = rp_key_store_item_get_flat_idx(ksitem);
			const KeyStoreUI::Key *const key = keyStoreUI->getKey(idx);
			assert(key != nullptr);
			if (!key)
				continue;

			rp_key_store_item_set_value(ksitem, key->value.c_str());
			rp_key_store_item_set_status(ksitem, static_cast<uint8_t>(key->status));
		}
	}
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab.cpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyManagerTab.hpp"
#include "RpConfigTab.hpp"

#include "RpGtk.hpp"
#include "gtk-compat.h"

#include "KeyStoreGTK.hpp"
using LibRomData::KeyStoreUI;

#include "MessageWidget.hpp"

// librpbase
using namespace LibRpBase;

// C++ STL classes
using std::string;

// KeyStoreUI::ImportFileID
static const char *const import_menu_actions[] = {
	NOP_C_("KeyManagerTab", "Wii keys.bin"),
	NOP_C_("KeyManagerTab", "Wii U otp.bin"),
	NOP_C_("KeyManagerTab", "3DS boot9.bin"),
	NOP_C_("KeyManagerTab", "3DS aeskeydb.bin"),
};

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

// GtkMenuButton was added in GTK 3.6.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
// NOTE: GtkMenu was removed from GTK4.
#if GTK_CHECK_VERSION(3,5,6)
#  define USE_GTK_MENU_BUTTON 1
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3,5,6) */

// KeyManagerTab class
struct _KeyManagerTabClass {
	superclass __parent__;
};

// KeyManagerTab instance
struct _KeyManagerTab {
	super __parent__;
	bool changed;	// If true, an option was changed.

	KeyStoreGTK *keyStore;

	GtkTreeStore *treeStore;
	GtkWidget *treeView;

	GtkWidget *btnImport;
	gchar *prevOpenDir;
#ifdef USE_G_MENU_MODEL
	GMenu *menuModel;
	GSimpleActionGroup *actionGroup;
#else /* !USE_G_MENU_MODEL */
	GtkWidget *menuImport;	// GtkMenu
#endif /* USE_G_MENU_MODEL */

	// MessageWidget for key import.
	GtkWidget *messageWidget;
};

static void	key_manager_tab_dispose				(GObject	*object);
static void	key_manager_tab_finalize			(GObject	*object);

static void	key_manager_tab_init_keys			(KeyManagerTab *tab);

// Interface initialization
static void	key_manager_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	key_manager_tab_has_defaults			(KeyManagerTab	*tab);
static void	key_manager_tab_reset				(KeyManagerTab	*tab);
static void	key_manager_tab_load_defaults			(KeyManagerTab	*tab);
static void	key_manager_tab_save				(KeyManagerTab	*tab,
								 GKeyFile       *keyFile);

// "Import" menu button
#ifndef USE_GTK_MENU_BUTTON
static gboolean	btnImport_event_signal_handler			(GtkButton	*button,
								 GdkEvent 	*event,
								 KeyManagerTab	*tab);
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
static void	action_triggered_signal_handler			(GSimpleAction	*action,
								 GVariant	*parameter,
								 KeyManagerTab	*tab);
#else
static void	menuImport_triggered_signal_handler		(GtkMenuItem	*menuItem,
								 KeyManagerTab	*tab);
#endif /* USE_G_MENU_MODEL */

// KeyStoreGTK signal handlers
static void	keyStore_key_changed_signal_handler		(KeyStoreGTK	*keyStore,
								 int		 sectIdx,
								 int		 keyIdx,
								 KeyManagerTab	*tab);
static void	keyStore_all_keys_changed_signal_handler	(KeyStoreGTK	*keyStore,
								 KeyManagerTab	*tab);
static void	keyStore_modified_signal_handler		(KeyStoreGTK	*keyStore,
								 KeyManagerTab	*tab);

// GtkCellRendererText signal handlers
static void	renderer_edited_signal_handler			(GtkCellRendererText *self,
								 gchar		*path,
								 gchar		*new_text,
								 KeyManagerTab	*tab);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(KeyManagerTab, key_manager_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			key_manager_tab_rp_config_tab_interface_init));

static void
key_manager_tab_class_init(KeyManagerTabClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = key_manager_tab_dispose;
	gobject_class->finalize = key_manager_tab_finalize;
}

static void
key_manager_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))key_manager_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))key_manager_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))key_manager_tab_load_defaults;
	iface->save = (__typeof__(iface->save))key_manager_tab_save;
}

static void
key_manager_tab_init(KeyManagerTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// MessageWidget goes at the top of the window.
	tab->messageWidget = message_widget_new();
	gtk_widget_set_name(tab->messageWidget, "messageWidget");

	// Initialize the KeyStoreGTK.
	tab->keyStore = key_store_gtk_new();
	g_signal_connect(tab->keyStore, "key-changed", G_CALLBACK(keyStore_key_changed_signal_handler), tab);
	g_signal_connect(tab->keyStore, "all-keys-changed", G_CALLBACK(keyStore_all_keys_changed_signal_handler), tab);
	g_signal_connect(tab->keyStore, "modified", G_CALLBACK(keyStore_modified_signal_handler), tab);

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
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

	// Create the GtkTreeStore and GtkTreeView.
	// Columns: Key Name, Value, Valid?, Flat Key Index
	// NOTE: "Valid?" column contains an icon name.
	tab->treeStore = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	tab->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tab->treeStore));
	gtk_widget_set_name(tab->treeView, "treeView");
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), tab->treeView);

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

	// "Import" button.
	string s_import = convert_accel_to_gtk(C_("KeyManagerTab", "&Import"));
#ifdef USE_GTK_MENU_BUTTON
	tab->btnImport = gtk_menu_button_new();
#else /* !USE_GTK_MENU_BUTTON */
	tab->btnImport = gtk_button_new();
#endif /* USE_GTK_MENU_BUTTON */
	gtk_widget_set_name(tab->btnImport, "btnImport");

#if GTK_CHECK_VERSION(4,0,0) && defined(USE_GTK_MENU_BUTTON)
	gtk_menu_button_set_label(GTK_MENU_BUTTON(tab->btnImport), s_import.c_str());
	gtk_menu_button_set_use_underline(GTK_MENU_BUTTON(tab->btnImport), TRUE);
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(tab->btnImport), GTK_ARROW_UP);
#else /* !GTK_CHECK_VERSION(4,0,0) || !defined(USE_GTK_MENU_BUTTON) */
	// GtkMenuButton in GTK3 only supports a label *or* an image by default.
	// Create a GtkBox to store both a label and image.
	// This will also be used for the non-GtkMenuButton version.
	GtkWidget *const lblImport = gtk_label_new(nullptr);
	gtk_widget_set_name(lblImport, "lblImport");
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(lblImport), s_import.c_str());
	GtkWidget *const imgImport = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_name(imgImport, "imgImport");

	GtkWidget *const hboxImport = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxImport, "hboxImport");
	gtk_box_pack_start(GTK_BOX(hboxImport), lblImport, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxImport), imgImport, false, false, 0);
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_button_set_child(GTK_BUTTON(tab->btnImport), hboxImport);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_container_add(GTK_CONTAINER(tab->btnImport), hboxImport);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#endif /* GTK_CHECK_VERSION(4,0,0) && defined(USE_GTK_MENU_BUTTON) */

#ifndef RP_USE_GTK_ALIGNMENT
	GTK_WIDGET_HALIGN_LEFT(tab->btnImport);
#else /* RP_USE_GTK_ALIGNMENT */
	// GTK2: Use a GtkAlignment.
	GtkWidget *const alignImport = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignImport, "alignImport");
	gtk_container_add(GTK_CONTAINER(alignImport), tab->btnImport);
	gtk_widget_show(alignImport);
#endif /* RP_USE_GTK_ALIGNMENT */

	// Create the "Import" popup menu.
#ifdef USE_G_MENU_MODEL
	char prefix[64];
	snprintf(prefix, sizeof(prefix), "rp-KeyManagerTab-Import-%p", tab->btnImport);

	tab->actionGroup = g_simple_action_group_new();
	tab->menuModel = g_menu_new();
	for (int i = 0; i < ARRAY_SIZE_I(import_menu_actions); i++) {
		// Create the action.
		char buf[128];
		snprintf(buf, sizeof(buf), "%d", i);
		GSimpleAction *const action = g_simple_action_new(buf, nullptr);
		g_simple_action_set_enabled(action, TRUE);
		g_object_set_data(G_OBJECT(action), "menuImport_id", GINT_TO_POINTER(i));
		g_signal_connect(action, "activate", G_CALLBACK(action_triggered_signal_handler), tab);
		g_action_map_add_action(G_ACTION_MAP(tab->actionGroup), G_ACTION(action));

		// Create the menu item.
		snprintf(buf, sizeof(buf), "%s.%d", prefix, i);
		g_menu_append(tab->menuModel, import_menu_actions[i], buf);
	}

	gtk_widget_insert_action_group(GTK_WIDGET(tab->btnImport), prefix, G_ACTION_GROUP(tab->actionGroup));
#else /* !USE_G_MENU_MODEL */
	tab->menuImport = gtk_menu_new();
	gtk_widget_set_name(tab->menuImport, "menuImport");
	for (int i = 0; i < ARRAY_SIZE_I(import_menu_actions); i++) {
		GtkWidget *const menuItem = gtk_menu_item_new_with_label(import_menu_actions[i]);
		char menu_name[32];
		snprintf(menu_name, sizeof(menu_name), "menuImport%d", i);
		g_object_set_data(G_OBJECT(menuItem), "menuImport_id", GINT_TO_POINTER(i));
		g_signal_connect(menuItem, "activate", G_CALLBACK(menuImport_triggered_signal_handler), tab);	// TODO
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(tab->menuImport), menuItem);
	}
#endif /* USE_G_MENU_MODEL */

	// If using GtkMenuButton, associate the menu.
	// Otherwise, we'll trigger the popup on click.
#ifdef USE_GTK_MENU_BUTTON
	// NOTE: GtkMenuButton does NOT take ownership of the menu.
#  ifdef USE_G_MENU_MODEL
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(tab->btnImport), G_MENU_MODEL(tab->menuModel));
#  else /* !USE_G_MENU_MODEL */
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(tab->btnImport), GTK_WIDGET(tab->menuImport));
#  endif /* USE_G_MENU_MODEL */
#endif /* USE_GTK_MENU_BUTTON */

#ifndef USE_GTK_MENU_BUTTON
	// Connect the button's "event" signal.
	// NOTE: We need to pass the event details. Otherwise, we'll
	// end up with the menu getting "stuck" to the mouse.
	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	g_signal_connect(tab->btnImport, "event", G_CALLBACK(btnImport_event_signal_handler), tab);
#endif /* !USE_GTK_MENU_BUTTON */

#if GTK_CHECK_VERSION(4,0,0)
	// Hide the MessageWidget initially.
	gtk_widget_hide(tab->messageWidget);

	gtk_box_append(GTK_BOX(tab), tab->messageWidget);
	gtk_box_append(GTK_BOX(tab), scrolledWindow);
	gtk_box_append(GTK_BOX(tab), tab->btnImport);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(tab), tab->messageWidget, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab), scrolledWindow, TRUE, TRUE, 0);

#  ifndef RP_USE_GTK_ALIGNMENT
	gtk_box_pack_start(GTK_BOX(tab), tab->btnImport, FALSE, FALSE, 0);
#  else /* RP_USE_GTK_ALIGNMENT */
	gtk_box_pack_start(GTK_BOX(tab), alignImport, FALSE, FALSE, 0);
#  endif /* RP_USE_GTK_ALIGNMENT */

	// NOTE: GTK4 defaults to visible; GTK2 and GTK3 defaults to invisible.
	// Hiding unconditionally just in case.
	gtk_widget_hide(tab->messageWidget);

	gtk_widget_show_all(scrolledWindow);
	gtk_widget_show_all(tab->btnImport);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Initialize the GtkTreeView with the available keys.
	key_manager_tab_init_keys(tab);

	// Load the current keys.
	key_manager_tab_reset(tab);
}

/**
 * Initialize keys in the GtkTreeView.
 * This initializes sections and key names.
 * Key values and "Valid?" are initialized by reset().
 * @param tab KeyManagerTab
 */
static void
key_manager_tab_init_keys(KeyManagerTab *tab)
{
	gtk_tree_store_clear(tab->treeStore);

	// FIXME: GtkTreeView doesn't have anything equivalent to
	// Qt's QTreeView::setFirstColumnSpanned().

	const KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(tab->keyStore);
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

static void
key_manager_tab_dispose(GObject *object)
{
	KeyManagerTab *const tab = KEY_MANAGER_TAB(object);

#ifdef USE_G_MENU_MODEL
	g_clear_object(&tab->menuModel);

	// The GSimpleActionGroup owns the actions, so
	// this will automatically delete the actions.
	g_clear_object(&tab->actionGroup);
#else /* !USE_G_MENU_MODEL */
	// Delete the "Import" button menu.
	if (tab->menuImport) {
		gtk_widget_destroy(tab->menuImport);
		tab->menuImport = nullptr;
	}
#endif /* USE_G_MENU_MODEL */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(key_manager_tab_parent_class)->dispose(object);
}

static void
key_manager_tab_finalize(GObject *object)
{
	KeyManagerTab *const tab = KEY_MANAGER_TAB(object);

	// Unreference the KeyStoreGTK.
	g_object_unref(tab->keyStore);

	// Free the strings.
	g_free(tab->prevOpenDir);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(key_manager_tab_parent_class)->finalize(object);
}

GtkWidget*
key_manager_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_KEY_MANAGER_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
key_manager_tab_has_defaults(KeyManagerTab *tab)
{
	g_return_val_if_fail(IS_KEY_MANAGER_TAB(tab), FALSE);
	return FALSE;
}

static void
key_manager_tab_reset(KeyManagerTab *tab)
{
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));

	// Reset/reload the key store.
	KeyStoreUI *const keyStore = key_store_gtk_get_key_store_ui(tab->keyStore);
	keyStore->reset();
}

static void
key_manager_tab_load_defaults(KeyManagerTab *tab)
{
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));
	// Not implemented.
}

static void
key_manager_tab_save(KeyManagerTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Keys were not changed.
		return;
	}

	// Save the keys.
	const KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(tab->keyStore);
	const int totalKeyCount = keyStoreUI->totalKeyCount();
	for (int i = 0; i < totalKeyCount; i++) {
		const KeyStoreUI::Key *const key = keyStoreUI->getKey(i);
		assert(key != nullptr);
		if (!key || !key->modified)
			continue;

		// Save this key.
		g_key_file_set_string(keyFile, "Keys", key->name.c_str(), key->value.c_str());
	}

	// Keys saved.
	tab->changed = false;
}

/** "Import" menu button **/

#ifndef USE_GTK_MENU_BUTTON
/**
 * Menu positioning function.
 * @param menu		[in] GtkMenu*
 * @param x		[out] X position
 * @param y		[out] Y position
 * @param push_in
 * @param user_data	[in] GtkButton* the menu is attached to.
 */
static void
btnImport_menu_pos_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GtkWidget *const button = (GtkWidget*)GTK_BUTTON(user_data);
	GdkWindow *const window = gtk_widget_get_window(button);

	// FIXME: GTK2: First run results in the menu going down, not up.
	GtkAllocation button_alloc, menu_alloc;
	gtk_widget_get_allocation(GTK_WIDGET(button), &button_alloc);
	gtk_widget_get_allocation(GTK_WIDGET(menu), &menu_alloc);
	gdk_window_get_origin(window, x, y);
	*x += button_alloc.x;
	*y += button_alloc.y - menu_alloc.height;

	*push_in = false;
}

/**
 * "Import" button event handler. (Non-GtkMenuButton version)
 * @param button	GtkButton
 * @param event		GdkEvent
 * @param tab		KeyManagerTab
 */
static gboolean
btnImport_event_signal_handler(GtkButton *button, GdkEvent *event, KeyManagerTab *tab)
{
	g_return_val_if_fail(IS_KEY_MANAGER_TAB(tab), FALSE);
	g_return_val_if_fail(GTK_IS_MENU(tab->menuImport), FALSE);

	if (gdk_event_get_event_type(event) != GDK_BUTTON_PRESS) {
		// Tell the caller that we did NOT handle the event.
		return FALSE;
	}

	// Pop up the menu.
	// FIXME: Improve button appearance so it's more pushed-in.
	guint button_id;
	if (!gdk_event_get_button(event, &button_id))
		return FALSE;

	gtk_menu_popup(GTK_MENU(tab->menuImport),
		nullptr, nullptr,
		btnImport_menu_pos_func, button, button_id,
		gdk_event_get_time(event));

	// Tell the caller that we handled the event.
	return TRUE;
}
#endif /* !USE_GTK_MENU_BUTTON */

static void
key_manager_tab_menu_action_response(GtkFileChooserDialog *fileDialog, gint response_id, KeyManagerTab *page);

/**
 * Handle a menu action.
 * Internal function used by both the GMenuModel and GtkMenu implementations.
 * @param tab KeyManagerTab
 * @param id Menu action ID
 */
static void
key_manager_tab_handle_menu_action(KeyManagerTab *tab, gint id)
{
	assert(id >= (int)KeyStoreUI::ImportFileID::WiiKeysBin);
	assert(id <= (int)KeyStoreUI::ImportFileID::N3DSaeskeydb);
	if (id < (int)KeyStoreUI::ImportFileID::WiiKeysBin ||
	    id > (int)KeyStoreUI::ImportFileID::N3DSaeskeydb)
		return;

	static const char dialog_titles_tbl[][32] = {
		// tr: Wii keys.bin dialog title
		NOP_C_("KeyManagerTab", "Select Wii keys.bin File"),
		// tr: Wii U otp.bin dialog title
		NOP_C_("KeyManagerTab", "Select Wii U otp.bin File"),
		// tr: Nintendo 3DS boot9.bin dialog title
		NOP_C_("KeyManagerTab", "Select 3DS boot9.bin File"),
		// tr: Nintendo 3DS aeskeydb.bin dialog title
		NOP_C_("KeyManagerTab", "Select 3DS aeskeydb.bin File"),
	};

	static const char file_filters_tbl[][88] = {
		// tr: Wii keys.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "keys.bin|keys.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		// tr: Wii U otp.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "otp.bin|otp.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		// tr: Nintendo 3DS boot9.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "boot9.bin|boot9.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		// tr: Nintendo 3DS aeskeydb.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "aeskeydb.bin|aeskeydb.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
	};

	const char *const s_title = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", dialog_titles_tbl[id]);
	const char *const s_filter = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", file_filters_tbl[id]);

	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(tab));
	GtkWidget *const fileDialog = gtk_file_chooser_dialog_new(
		s_title,			// title
		parent,				// parent
		GTK_FILE_CHOOSER_ACTION_OPEN,	// action
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Open"), GTK_RESPONSE_ACCEPT,
		nullptr);
	gtk_widget_set_name(fileDialog, "fileDialog");

#if GTK_CHECK_VERSION(4,0,0)
	// NOTE: GTK4 has *mandatory* overwrite confirmation.
	// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12

	// TODO: URI?
	if (tab->prevOpenDir) {
		GFile *const set_file = g_file_new_for_path(tab->prevOpenDir);
		if (set_file) {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), set_file, nullptr);
			g_object_unref(set_file);
		}
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fileDialog), TRUE);
	if (tab->prevOpenDir) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), tab->prevOpenDir);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Set the filters.
	rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(fileDialog), s_filter);

	// Set the file ID in the dialog.
	g_object_set_data(G_OBJECT(fileDialog), "KeyManagerTab.fileID", GINT_TO_POINTER(id));

	// Prompt for a filename.
	g_signal_connect(fileDialog, "response", G_CALLBACK(key_manager_tab_menu_action_response), tab);
	gtk_window_set_transient_for(GTK_WINDOW(fileDialog), parent);
	gtk_window_set_modal(GTK_WINDOW(fileDialog), true);
	gtk_widget_show(GTK_WIDGET(fileDialog));

	// GtkFileChooserDialog will send the "response" signal when the dialog is closed.
}

/**
 * Show key import return status.
 * @praam tab KeyManagerTab
 * @param filename Filename
 * @param keyType Key type
 * @param iret ImportReturn
 */
static void
key_manager_tab_show_key_import_return_status(KeyManagerTab	*tab,
					      const char	*filename,
					      const char	*keyType,
					      const KeyStoreUI::ImportReturn &iret)
{
	GtkMessageType type = GTK_MESSAGE_INFO;
	bool showKeyStats = false;
	string msg;
	msg.reserve(1024);

	// Filename, minus directory. (must be g_free()'d later)
	gchar *const fileNoPath = g_path_get_basename(filename);

	// TODO: Localize POSIX error messages?
	// TODO: Thread-safe strerror()?
	// NOTE: glib doesn't seem to have its own numeric formatting,
	// so we'll use printf()'s grouping modifier.

	switch (iret.status) {
		case KeyStoreUI::ImportStatus::InvalidParams:
		default:
			msg = C_("KeyManagerTab",
				"An invalid parameter was passed to the key importer.\n"
				"THIS IS A BUG; please report this to the developers!");
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::UnknownKeyID:
			msg = C_("KeyManagerTab",
				"An unknown key ID was passed to the key importer.\n"
				"THIS IS A BUG; please report this to the developers!");
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::OpenError:
			if (iret.error_code != 0) {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %1$s == filename, %2$s == error message
					"An error occurred while opening '%1$s': %2$s"),
					fileNoPath, strerror(iret.error_code));
			} else {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %s == filename
					"An error occurred while opening '%s'."),
					fileNoPath);
			}
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::ReadError:
			// TODO: Error code for short reads.
			if (iret.error_code != 0) {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %1$s == filename, %2$s == error message
					"An error occurred while reading '%1$s': %2$s"),
					fileNoPath, strerror(iret.error_code));
			} else {
				msg = rp_sprintf_p(C_("KeyManagerTab",
					// tr: %s == filename
					"An error occurred while reading '%s'."),
					fileNoPath);
			}
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::InvalidFile:
			msg = rp_sprintf_p(C_("KeyManagerTab",
				// tr: %1$s == filename, %2$s == type of file
				"The file '%1$s' is not a valid %2$s file."),
				fileNoPath, keyType);
			type = GTK_MESSAGE_WARNING;
			break;

		case KeyStoreUI::ImportStatus::NoKeysImported:
			msg = rp_sprintf(C_("KeyManagerTab",
				// tr: %s == filename
				"No keys were imported from '%s'."),
				fileNoPath);
			type = GTK_MESSAGE_INFO;
			showKeyStats = true;
			break;

		case KeyStoreUI::ImportStatus::KeysImported: {
			const unsigned int keyCount = iret.keysImportedVerify + iret.keysImportedNoVerify;
			char buf[16];
			snprintf(buf, sizeof(buf), "%'u", keyCount);

			msg = rp_sprintf_p(NC_("KeyManagerTab",
				// tr: %1$s == number of keys (formatted), %2$u == filename
				"%1$s key was imported from '%2$s'.",
				"%1$s keys were imported from '%2$s'.",
				keyCount), buf, fileNoPath);
			type = GTK_MESSAGE_INFO;	// NOTE: No equivalent to KMessageWidget::Positive.
			showKeyStats = true;
			break;
		}
	}

	// U+2022 (BULLET) == \xE2\x80\xA2
	static const char nl_bullet[] = "\n\xE2\x80\xA2 ";

	if (showKeyStats) {
		char buf[16];

		if (iret.keysExist > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysExist);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key already exists in the Key Manager.",
				"%s keys already exist in the Key Manager.",
				iret.keysExist), buf);
		}
		if (iret.keysInvalid > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysInvalid);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it is incorrect.",
				"%s keys were not imported because they are incorrect.",
				iret.keysInvalid), buf);
		}
		if (iret.keysNotUsed > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysNotUsed);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it isn't used by rom-properties.",
				"%s keys were not imported because they aren't used by rom-properties.",
				iret.keysNotUsed), buf);
		}
		if (iret.keysCantDecrypt > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysCantDecrypt);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key was not imported because it is encrypted and the master key isn't available.",
				"%s keys were not imported because they are encrypted and the master key isn't available.",
				iret.keysCantDecrypt), buf);
		}
		if (iret.keysImportedVerify > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysImportedVerify);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				// tr: %s == number of keys (formatted)
				"%s key has been imported and verified as correct.",
				"%s keys have been imported and verified as correct.",
				iret.keysImportedVerify), buf);
		}
		if (iret.keysImportedNoVerify > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysImportedNoVerify);
			msg += nl_bullet;
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key has been imported without verification.",
				"%s keys have been imported without verification.",
				iret.keysImportedNoVerify), buf);
		}
	}

	// Display the message.
	// TODO: Copy over timeout code from RomDataView?
	// (Or, remove the timeout code entirely?)
	// TODO: MessageSound?
	message_widget_set_message_type(MESSAGE_WIDGET(tab->messageWidget), type);
	message_widget_set_text(MESSAGE_WIDGET(tab->messageWidget), msg.c_str());
	gtk_widget_show(tab->messageWidget);
}

/**
 * The Save dialog for a Standard ROM Operation has been closed.
 * @param fileDialog GtkFileChooserDialog
 * @param response_id Response ID
 * @param tab KeyManagerTab
 */
static void
key_manager_tab_menu_action_response(GtkFileChooserDialog *fileDialog, gint response_id, KeyManagerTab *tab)
{
	if (response_id != GTK_RESPONSE_ACCEPT) {
		// User cancelled the dialog.
#if GTK_CHECK_VERSION(4,0,0)
		gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_widget_destroy(GTK_WIDGET(fileDialog));
#endif /* GTK_CHECK_VERSION(4,0,0) */
		return;
	}

	// Get the file ID from the dialog.
	const KeyStoreUI::ImportFileID id = static_cast<KeyStoreUI::ImportFileID>(
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(fileDialog), "KeyManagerTab.fileID")));

#if GTK_CHECK_VERSION(4,0,0)
	// TODO: URIs?
	gchar *in_filename = nullptr;
	GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(fileDialog));
	if (get_file) {
		in_filename = g_file_get_path(get_file);
		g_object_unref(get_file);
	}
	gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gchar *const in_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileDialog));
	gtk_widget_destroy(GTK_WIDGET(fileDialog));
#endif /* GTK_CHECK_VERSION(4,0,0) */

	if (!in_filename) {
		// No filename...
		return;
	}

	KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(tab->keyStore);
	KeyStoreUI::ImportReturn iret = keyStoreUI->importKeysFromBin(id,
		reinterpret_cast<const char8_t*>(in_filename));

	// TODO: Show the key import status in a MessageWidget.
	key_manager_tab_show_key_import_return_status(tab, in_filename,
		dpgettext_expr(RP_I18N_DOMAIN, "KeyManagerTab", import_menu_actions[(int)id]), iret);
	g_free(in_filename);
}

#ifdef USE_G_MENU_MODEL
/**
 * An "Import" menu action was triggered.
 * @param action     	GSimpleAction (Get the "menuImport_id" data.)
 * @param parameter	Parameter data
 * @param tab		KeyManagerTab
 */
static void
action_triggered_signal_handler(GSimpleAction *action, GVariant *parameter, KeyManagerTab *tab)
{
	RP_UNUSED(parameter);
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(action), "menuImport_id"));
	key_manager_tab_handle_menu_action(tab, id);
}
#else /* !USE_G_MENU_MODEL */
/**
 * An "Options" menu action was triggered.
 * @param menuItem     	Menu item (Get the "menuImport_id" data.)
 * @param tab		KeyManagerTab
 */
static void
menuImport_triggered_signal_handler(GtkMenuItem *menuItem, KeyManagerTab *tab)
{
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(menuItem), "menuImport_id"));
	key_manager_tab_handle_menu_action(tab, id);
}
#endif /* USE_G_MENU_MODEL */

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
static void
keyStore_key_changed_signal_handler(KeyStoreGTK *keyStore, int sectIdx, int keyIdx, KeyManagerTab *tab)
{
	const KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(keyStore);
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
}

/**
 * All keys in the KeyStore have changed.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
static void
keyStore_all_keys_changed_signal_handler(KeyStoreGTK *keyStore, KeyManagerTab *tab)
{
	const KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(keyStore);
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

/**
 * KeyStore has been modified.
 * This signal is forwarded to the parent ConfigDialog.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
static void
keyStore_modified_signal_handler(KeyStoreGTK *keyStore, KeyManagerTab *tab)
{
        RP_UNUSED(keyStore);

        // Forward the "modified" signal.
        tab->changed = true;
        g_signal_emit_by_name(tab, "modified", NULL);
}

/** GtkCellRendererText signal handlers **/

static void
renderer_edited_signal_handler(GtkCellRendererText *self, gchar *path, gchar *new_text, KeyManagerTab *tab)
{
	RP_UNUSED(self);
	RP_UNUSED(tab);

	KeyStoreUI *const keyStoreUI = key_store_gtk_get_key_store_ui(tab->keyStore);

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

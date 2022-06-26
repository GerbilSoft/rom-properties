/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab.cpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyManagerTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
using namespace LibRpBase;

// C++ STL classes
using std::string;

enum ImportMenuID {
	IMPORT_WII_KEYS_BIN	= 0,
	IMPORT_WII_U_OTP_BIN	= 1,
	IMPORT_3DS_BOOT9_BIN	= 2,
	IMPORT_3DS_AESKEYDB_BIN	= 3,
};
static const char *const import_menu_actions[] = {
	"Wii keys.bin",
	"Wii U otp.bin",
	"3DS boot9.bin",
	"3DS aeskeydb.bin",
};

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// GtkMenuButton was added in GTK 3.6.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
// NOTE: GtkMenu was removed from GTK4.
#if GTK_CHECK_VERSION(3,6,0)
#  define USE_GTK_MENU_BUTTON 1
#  define USE_G_MENU_MODEL 1
#endif

// KeyManagerTab class
struct _KeyManagerTabClass {
	superclass __parent__;
};

// KeyManagerTab instance
struct _KeyManagerTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

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
};

static void	key_manager_tab_dispose				(GObject	*object);
static void	key_manager_tab_finalize			(GObject	*object);

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

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(KeyManagerTab, key_manager_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_CONFIG_TYPE_TAB,
			key_manager_tab_rp_config_tab_interface_init));

static void
key_manager_tab_class_init(KeyManagerTabClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
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

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	// Create the GtkTreeStore and GtkTreeView.
	// TODO: Additional internal columns for key IDs?
	tab->treeStore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, PIMGTYPE_GOBJECT_TYPE);
	tab->treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tab->treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tab->treeView), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), tab->treeView);

	// Column 1: Key Name
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Key Name"));
	gtk_tree_view_column_set_resizable(column, FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);
	
	// Column 2: Value
	// TODO: Monospace font
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Value"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

	// Column 3: Unlock Time
	// TODO: Store as a string, or as a GDateTime?
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, C_("KeyManagerTab", "Valid?"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tab->treeView), column);

	// "Import" button.
	string s_import = convert_accel_to_gtk(C_("KeyManagerTab", "&Import"));
#ifdef USE_GTK_MENU_BUTTON
	tab->btnImport = gtk_menu_button_new();
#else /* !USE_GTK_MENU_BUTTON */
	tab->btnImport = gtk_button_new();
#endif /* USE_GTK_MENU_BUTTON */

#if GTK_CHECK_VERSION(4,0,0) && defined(USE_GTK_MENU_BUTTON)
	gtk_menu_button_set_label(GTK_MENU_BUTTON(tab->btnImport), s_import.c_str());
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(tab->btnImport), GTK_ARROW_UP);
#else /* !GTK_CHECK_VERSION(4,0,0) || !defined(USE_GTK_MENU_BUTTON) */
	// GtkMenuButton in GTK3 only supports a label *or* an image by default.
	// Create a GtkBox to store both a label and image.
	// This will also be used for the non-GtkMenuButton version.
	GtkWidget *const lblImport = gtk_label_new(nullptr);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(lblImport), s_import.c_str());
	gtk_widget_show(lblImport);

	GtkWidget *const imgImport = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(imgImport);

	GtkWidget *const hboxImport = RP_gtk_hbox_new(4);
	gtk_box_pack_start(GTK_BOX(hboxImport), lblImport, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxImport), imgImport, false, false, 0);
	gtk_widget_show(hboxImport);
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_button_set_child(GTK_BUTTON(tab->btnImport), hboxImport);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_container_add(GTK_CONTAINER(tab->btnImport), hboxImport);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#endif /* GTK_CHECK_VERSION(4,0,0) && defined(USE_GTK_MENU_BUTTON) */
	GTK_WIDGET_HALIGN_LEFT(tab->btnImport);

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
	for (int i = 0; i < ARRAY_SIZE_I(import_menu_actions); i++) {
		GtkWidget *const menuItem = gtk_menu_item_new_with_label(import_menu_actions[i]);
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
	gtk_box_append(GTK_BOX(tab), scrolledWindow);
	gtk_box_append(GTK_BOX(tab), tab->btnImport);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(scrolledWindow);
	gtk_widget_show(tab->treeView);
	gtk_widget_show(tab->btnImport);

	gtk_box_pack_start(GTK_BOX(tab), scrolledWindow, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->btnImport, FALSE, FALSE, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	key_manager_tab_reset(tab);
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
	g_return_val_if_fail(IS_KEY_MANAGER_TAB(tab), false);
	return true;
}

static void
key_manager_tab_reset(KeyManagerTab *tab)
{
	g_return_if_fail(IS_KEY_MANAGER_TAB(tab));

	// TODO
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

	// TODO
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

/**
 * Handle a menu action.
 * Internal function used by both the GMenuModel and GtkMenu implementations.
 * @param tab KeyManagerTab
 * @param id Menu action ID
 */
static void
key_manager_tab_handle_menu_action(KeyManagerTab *tab, gint id)
{
	printf("key_manager_tab_handle_menu_action: %d\n", id);
	assert(id >= IMPORT_WII_KEYS_BIN);
	assert(id <= IMPORT_3DS_AESKEYDB_BIN);
	if (id < IMPORT_WII_KEYS_BIN || id > IMPORT_3DS_AESKEYDB_BIN)
		return;

	static const char *const dialog_titles_tbl[] = {
		NOP_C_("KeyManagerTab", "Select Wii keys.bin File"),
		NOP_C_("KeyManagerTab", "Select Wii U otp.bin File"),
		NOP_C_("KeyManagerTab", "Select 3DS boot9.bin File"),
		NOP_C_("KeyManagerTab", "Select 3DS aeskeydb.bin File"),
	};

	static const char *const file_filters_tbl[] = {
		NOP_C_("KeyManagerTab", "keys.bin|keys.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		NOP_C_("KeyManagerTab", "otp.bin|otp.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		NOP_C_("KeyManagerTab", "boot9.bin|boot9.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
		NOP_C_("KeyManagerTab", "aeskeydb.bin|aeskeydb.bin|-|Binary Files|*.bin|application/octet-stream|All Files|*.*|-"),
	};

	const char *const s_title = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", dialog_titles_tbl[id]);
	const char *const s_filter = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", file_filters_tbl[id]);

	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(tab));

#if GTK_CHECK_VERSION(4,0,0)
	// NOTE: GTK4 has *mandatory* overwrite confirmation.
	// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12
	GtkFileChooserNative *const dialog = gtk_file_chooser_native_new(
		s_title,			// title
		parent,				// parent
		GTK_FILE_CHOOSER_ACTION_OPEN,	// action
		_("_Open"), _("_Cancel"));
	// TODO: URI?
	if (tab->prevOpenDir) {
		GFile *const set_file = g_file_new_for_path(tab->prevOpenDir);
		if (set_file) {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), set_file, nullptr);
			g_object_unref(set_file);
		}
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const dialog = gtk_file_chooser_dialog_new(
		s_title,			// title
		parent,				// parent
		GTK_FILE_CHOOSER_ACTION_OPEN,	// action
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Open"), GTK_RESPONSE_ACCEPT,
		nullptr);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	if (tab->prevOpenDir) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), tab->prevOpenDir);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Set the filters.
	rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(dialog), s_filter);

	// Prompt for a filename.
#if GTK_CHECK_VERSION(4,0,0)
	// GTK4 no longer supports blocking dialogs.
	// FIXME for GTK4: Rewrite to use gtk_window_set_modal() and handle the "response" signal.
	// This will also work for older GTK+.
	assert(!"gtk_dialog_run() is not available in GTK4; needs a rewrite!");
	GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

	// TODO: URIs?
	gchar *in_filename = (get_file ? g_file_get_path(get_file) : nullptr);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gint res = gtk_dialog_run(GTK_DIALOG(dialog));
	gchar *in_filename = (res == GTK_RESPONSE_ACCEPT
		? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))
		: nullptr);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	g_object_unref(dialog);
	if (!in_filename) {
		// No filename...
		return;
	}

	// Save the previous export directory.
	g_free(tab->prevOpenDir);
	tab->prevOpenDir = g_path_get_dirname(in_filename);

	// TODO
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

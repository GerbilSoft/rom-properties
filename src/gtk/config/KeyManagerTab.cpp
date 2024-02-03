/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * KeyManagerTab.cpp: Key Manager tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyManagerTab.hpp"
#include "KeyManagerTab_p.hpp"
#include "RpGtkCpp.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRomData::KeyStoreUI;

// C++ STL classes
using std::string;

// KeyStoreUI::ImportFileID
static const std::array<const char*, 4> import_menu_actions = {{
	"Wii keys.bin",
	"Wii U otp.bin",
	"3DS boot9.bin",
	"3DS aeskeydb.bin",
}};

static GQuark menuImport_id_quark;
static GQuark KeyManagerTab_fileID_quark;

static void	rp_key_manager_tab_dispose			(GObject	*object);
static void	rp_key_manager_tab_finalize			(GObject	*object);

// Interface initialization
static void	rp_key_manager_tab_rp_config_tab_interface_init	(RpConfigTabInterface	*iface);
static gboolean	rp_key_manager_tab_has_defaults			(RpKeyManagerTab	*tab);
static void	rp_key_manager_tab_reset			(RpKeyManagerTab	*tab);
static void	rp_key_manager_tab_save				(RpKeyManagerTab	*tab,
								 GKeyFile		*keyFile);

// "Import" menu button
#ifndef USE_GTK_MENU_BUTTON
static gboolean	btnImport_event_signal_handler			(GtkButton		*button,
								 GdkEvent 		*event,
								 RpKeyManagerTab	*tab);
#endif /* !USE_GTK_MENU_BUTTON */

static void rp_key_manager_tab_show_key_import_return_status	(RpKeyManagerTab	*tab,
								 const char		*filename,
								 const char		*keyType,
								 const KeyStoreUI::ImportReturn &iret);

#ifdef USE_G_MENU_MODEL
static void	action_triggered_signal_handler			(GSimpleAction		*action,
								 GVariant		*parameter,
								 RpKeyManagerTab	*tab);
#else
static void	menuImport_triggered_signal_handler		(GtkMenuItem		*menuItem,
								 RpKeyManagerTab	*tab);
#endif /* USE_G_MENU_MODEL */

/** Signal handlers **/

static void	keyStore_modified_signal_handler		(RpKeyStoreGTK *keyStore,
								 RpKeyManagerTab *tab);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpKeyManagerTab, rp_key_manager_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_key_manager_tab_rp_config_tab_interface_init));

static void
rp_key_manager_tab_class_init(RpKeyManagerTabClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_key_manager_tab_dispose;
	gobject_class->finalize = rp_key_manager_tab_finalize;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	menuImport_id_quark = g_quark_from_string("menuImport_id");
	KeyManagerTab_fileID_quark = g_quark_from_string("KeyManagerTab.fileID");

	// Version-specific class initialization
	rp_key_manager_tab_class_init_gtkver(klass);
}

static void
rp_key_manager_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_key_manager_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_key_manager_tab_reset;
	iface->load_defaults = nullptr;
	iface->save = (__typeof__(iface->save))rp_key_manager_tab_save;
}

static void
rp_key_manager_tab_init(RpKeyManagerTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// MessageWidget goes at the top of the window.
	tab->messageWidget = rp_message_widget_new();
	gtk_widget_set_name(tab->messageWidget, "messageWidget");

	// Initialize the KeyStoreGTK.
	tab->keyStore = rp_key_store_gtk_new();
	g_signal_connect(tab->keyStore, "key-changed", G_CALLBACK(keyStore_key_changed_signal_handler), tab);
	g_signal_connect(tab->keyStore, "all-keys-changed", G_CALLBACK(keyStore_all_keys_changed_signal_handler), tab);
	g_signal_connect(tab->keyStore, "modified", G_CALLBACK(keyStore_modified_signal_handler), tab);

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	tab->scrolledWindow = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(tab->scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	tab->scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(tab->scrolledWindow), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_widget_set_name(tab->scrolledWindow, "scrolledWindow");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scrolledWindow),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(2,91,1)
	gtk_widget_set_halign(tab->scrolledWindow, GTK_ALIGN_FILL);
	gtk_widget_set_valign(tab->scrolledWindow, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand(tab->scrolledWindow, TRUE);
	gtk_widget_set_vexpand(tab->scrolledWindow, TRUE);
#endif /* GTK_CHECK_VERSION(2,91,1) */

	// Create the Gtk(Tree|Column)View and backing store.
	rp_key_manager_tab_create_GtkTreeView(tab);

	// "Import" button.
	const string s_import = convert_accel_to_gtk(C_("KeyManagerTab", "I&mport"));
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
	gtk_label_set_text_with_mnemonic(GTK_LABEL(lblImport), s_import.c_str());
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
		g_object_set_qdata(G_OBJECT(action), menuImport_id_quark, GINT_TO_POINTER(i));
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
		g_object_set_qdata(G_OBJECT(menuItem), menuImport_id_quark, GINT_TO_POINTER(i));
		g_signal_connect(menuItem, "activate", G_CALLBACK(menuImport_triggered_signal_handler), tab);
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
	gtk_widget_set_visible(tab->messageWidget, false);

	gtk_box_append(GTK_BOX(tab), tab->messageWidget);
	gtk_box_append(GTK_BOX(tab), tab->scrolledWindow);
	gtk_box_append(GTK_BOX(tab), tab->btnImport);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(tab), tab->messageWidget, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->scrolledWindow, TRUE, TRUE, 0);

#  ifndef RP_USE_GTK_ALIGNMENT
	gtk_box_pack_start(GTK_BOX(tab), tab->btnImport, FALSE, FALSE, 0);
#  else /* RP_USE_GTK_ALIGNMENT */
	gtk_box_pack_start(GTK_BOX(tab), alignImport, FALSE, FALSE, 0);
#  endif /* RP_USE_GTK_ALIGNMENT */

	// NOTE: GTK4 defaults to visible; GTK2 and GTK3 defaults to invisible.
	// Hiding unconditionally just in case.
	gtk_widget_set_visible(tab->messageWidget, false);

	gtk_widget_show_all(tab->scrolledWindow);
	gtk_widget_show_all(tab->btnImport);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Initialize the GtkTreeView with the available keys.
	rp_key_manager_tab_init_keys(tab);

	// Load the current keys.
	rp_key_manager_tab_reset(tab);
}

static void
rp_key_manager_tab_dispose(GObject *object)
{
	RpKeyManagerTab *const tab = RP_KEY_MANAGER_TAB(object);

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

	// KeyStoreGTK
	g_clear_object(&tab->keyStore);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_key_manager_tab_parent_class)->dispose(object);
}

static void
rp_key_manager_tab_finalize(GObject *object)
{
	RpKeyManagerTab *const tab = RP_KEY_MANAGER_TAB(object);

	g_free(tab->prevOpenDir);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_key_manager_tab_parent_class)->finalize(object);
}

GtkWidget*
rp_key_manager_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_KEY_MANAGER_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_key_manager_tab_has_defaults(RpKeyManagerTab *tab)
{
	g_return_val_if_fail(RP_IS_KEY_MANAGER_TAB(tab), FALSE);
	return FALSE;
}

static void
rp_key_manager_tab_reset(RpKeyManagerTab *tab)
{
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(tab));

	// Reset/reload the key store.
	KeyStoreUI *const keyStore = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	keyStore->reset();
}

static void
rp_key_manager_tab_save(RpKeyManagerTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Keys were not changed.
		return;
	}

	// Save the keys.
	const KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
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
 * @param button tkButton
 * @param event GdkEvent
 * @param tab KeyManagerTab
 */
static gboolean
btnImport_event_signal_handler(GtkButton *button, GdkEvent *event, RpKeyManagerTab *tab)
{
	g_return_val_if_fail(RP_IS_KEY_MANAGER_TAB(tab), FALSE);
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

// Simple struct for passing both KeyManagerTab and the file ID.
struct open_data_t {
	RpKeyManagerTab *tab;
	KeyStoreUI::ImportFileID id;
};

/**
 * File dialog callback function.
 * @param file (in) (transfer full): Selected file, or nullptr if no file was selected
 * @param open_data (in) (transfer full): open_data_t specified as user_data when rpGtk_getOpenFileName() was called.
 */
static void
rp_key_manager_getOpenFileDialog_callback(GFile *file, open_data_t *open_data)
{
	if (!file) {
		// No file selected.
		g_free(open_data);
		return;
	}

	// TODO: URIs?
	gchar *const filename = g_file_get_path(file);
	g_object_unref(file);
	if (!filename) {
		// No filename...
		g_free(open_data);
		return;
	}

	// for convenience purposes
	RpKeyManagerTab *const tab = open_data->tab;

	KeyStoreUI *const keyStoreUI = rp_key_store_gtk_get_key_store_ui(tab->keyStore);
	const KeyStoreUI::ImportReturn iret = keyStoreUI->importKeysFromBin(open_data->id, filename);

	rp_key_manager_tab_show_key_import_return_status(tab, filename, import_menu_actions[(int)open_data->id], iret);
	g_free(filename);
	g_free(open_data);
}

/**
 * Handle a menu action.
 * Internal function used by both the GMenuModel and GtkMenu implementations.
 * @param tab KeyManagerTab
 * @param id Menu action ID
 */
static void
rp_key_manager_tab_handle_menu_action(RpKeyManagerTab *tab, gint id)
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

	static const char file_filters_tbl[][64] = {
		// tr: Wii keys.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "keys.bin|keys.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Wii U otp.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "otp.bin|otp.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Nintendo 3DS boot9.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "boot9.bin|boot9.bin|-|Binary Files|*.bin|-|All Files|*|-"),
		// tr: Nintendo 3DS aeskeydb.bin file filter (RP format)
		NOP_C_("KeyManagerTab", "aeskeydb.bin|aeskeydb.bin|-|Binary Files|*.bin|-|All Files|*|-"),
	};

	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(tab));
	const char *const title = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", dialog_titles_tbl[id]);
	const char *const filter = dpgettext_expr(
		RP_I18N_DOMAIN, "KeyManagerTab", file_filters_tbl[id]);

	open_data_t *const open_data = static_cast<open_data_t*>(g_malloc(sizeof(*open_data)));
	open_data->tab = tab;
	open_data->id = static_cast<LibRomData::KeyStoreUI::ImportFileID>(id);

	const rpGtk_getFileName_t gfndata = {
		parent,			// parent
		title,			// title
		filter,			// filter
		tab->prevOpenDir,	// init_dir
		nullptr,		// init_name
		(rpGtk_fileDialogCallback)rp_key_manager_getOpenFileDialog_callback,	// callback
		open_data,		// user_data
	};
	const int ret = rpGtk_getOpenFileName(&gfndata);
	if (ret != 0) {
		// rpGtk_getOpenFileName() failed.
		// g_free() the open_data_t because the callback won't be run.
		g_free(open_data);
	}

	// rpGtk_getOpenFileName() will call rp_key_manager_getOpenFileDialog_callback()
	// when the dialog is closed.
}

/**
 * Show key import return status.
 * @param tab KeyManagerTab
 * @param filename Filename
 * @param keyType Key type
 * @param iret ImportReturn
 */
static void
rp_key_manager_tab_show_key_import_return_status(RpKeyManagerTab	*tab,
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
				// tr: %1$s == filename, %2$s == error message
				msg = rp_sprintf_p(C_("KeyManagerTab",
					"An error occurred while opening '%1$s': %2$s"),
					fileNoPath, strerror(iret.error_code));
			} else {
				// tr: %s == filename
				msg = rp_sprintf_p(C_("KeyManagerTab",
					"An error occurred while opening '%s'."),
					fileNoPath);
			}
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::ReadError:
			// TODO: Error code for short reads.
			if (iret.error_code != 0) {
				// tr: %1$s == filename, %2$s == error message
				msg = rp_sprintf_p(C_("KeyManagerTab",
					"An error occurred while reading '%1$s': %2$s"),
					fileNoPath, strerror(iret.error_code));
			} else {
				// tr: %s == filename
				msg = rp_sprintf_p(C_("KeyManagerTab",
					"An error occurred while reading '%s'."),
					fileNoPath);
			}
			type = GTK_MESSAGE_ERROR;
			break;

		case KeyStoreUI::ImportStatus::InvalidFile:
			// tr: %1$s == filename, %2$s == type of file
			msg = rp_sprintf_p(C_("KeyManagerTab",
				"The file '%1$s' is not a valid %2$s file."),
				fileNoPath, keyType);
			type = GTK_MESSAGE_WARNING;
			break;

		case KeyStoreUI::ImportStatus::NoKeysImported:
			// tr: %s == filename
			msg = rp_sprintf(C_("KeyManagerTab",
				"No keys were imported from '%s'."),
				fileNoPath);
			type = GTK_MESSAGE_INFO;
			showKeyStats = true;
			break;

		case KeyStoreUI::ImportStatus::KeysImported: {
			const int keyCount = static_cast<int>(iret.keysImportedVerify + iret.keysImportedNoVerify);
			char buf[16];
			snprintf(buf, sizeof(buf), "%'d", keyCount);

			// tr: %1$s == number of keys (formatted), %2$u == filename
			msg = rp_sprintf_p(NC_("KeyManagerTab",
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
			// tr: %s == number of keys (formatted)
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key already exists in the Key Manager.",
				"%s keys already exist in the Key Manager.",
				iret.keysExist), buf);
		}
		if (iret.keysInvalid > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysInvalid);
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key was not imported because it is incorrect.",
				"%s keys were not imported because they are incorrect.",
				iret.keysInvalid), buf);
		}
		if (iret.keysNotUsed > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysNotUsed);
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key was not imported because it isn't used by rom-properties.",
				"%s keys were not imported because they aren't used by rom-properties.",
				iret.keysNotUsed), buf);
		}
		if (iret.keysCantDecrypt > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysCantDecrypt);
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_sprintf(NC_("KeyManagerTab",
				"%s key was not imported because it is encrypted and the master key isn't available.",
				"%s keys were not imported because they are encrypted and the master key isn't available.",
				iret.keysCantDecrypt), buf);
		}
		if (iret.keysImportedVerify > 0) {
			snprintf(buf, sizeof(buf), "%'d", iret.keysImportedVerify);
			msg += nl_bullet;
			// tr: %s == number of keys (formatted)
			msg += rp_sprintf(NC_("KeyManagerTab",
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

	g_free(fileNoPath);

	// Display the message.
	// TODO: Copy over timeout code from RomDataView?
	// (Or, remove the timeout code entirely?)
	// TODO: MessageSound?
	rp_message_widget_set_message_type(RP_MESSAGE_WIDGET(tab->messageWidget), type);
	rp_message_widget_set_text(RP_MESSAGE_WIDGET(tab->messageWidget), msg.c_str());
	gtk_widget_set_visible(tab->messageWidget, true);
}

#ifdef USE_G_MENU_MODEL
/**
 * An "Import" menu action was triggered.
 * @param action GSimpleAction (Get the "menuImport_id" data.)
 * @param parameter Parameter data
 * @param tab KeyManagerTab
 */
static void
action_triggered_signal_handler(GSimpleAction *action, GVariant *parameter, RpKeyManagerTab *tab)
{
	RP_UNUSED(parameter);
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(tab));

	const gint id = (gboolean)GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(action), menuImport_id_quark));
	rp_key_manager_tab_handle_menu_action(tab, id);
}
#else /* !USE_G_MENU_MODEL */
/**
 * An "Import" menu action was triggered.
 * @param menuItem Menu item (Get the "menuImport_id" data.)
 * @param tab KeyManagerTab
 */
static void
menuImport_triggered_signal_handler(GtkMenuItem *menuItem, RpKeyManagerTab *tab)
{
	g_return_if_fail(RP_IS_KEY_MANAGER_TAB(tab));

	const gint id = (gboolean)GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(menuItem), menuImport_id_quark));
	rp_key_manager_tab_handle_menu_action(tab, id);
}
#endif /* USE_G_MENU_MODEL */

/** Signal handlers **/

/**
 * KeyStore has been modified.
 * This signal is forwarded to the parent ConfigDialog.
 * @param keyStore KeyStoreGTK
 * @param tab KeyManagerTab
 */
static void
keyStore_modified_signal_handler(RpKeyStoreGTK *keyStore, RpKeyManagerTab *tab)
{
        RP_UNUSED(keyStore);

        // Forward the "modified" signal.
        tab->changed = true;
        g_signal_emit_by_name(tab, "modified", nullptr);
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsMenuButton.hpp: Options menu GtkMenuButton subclass.             *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsMenuButton.hpp"
#include "PIMGTYPE.hpp"

// librpbase
using LibRpBase::RomData;

// C++ STL classes
using std::string;
using std::vector;

#if GTK_CHECK_VERSION(4,0,0)
// FIXME: GtkMenuButton is final in GTK4.
// For now, copying the class definitions from GTK 4.2, but this
// will need to be remade into a container.
struct _GtkMenuButton {
	GtkWidget parent_instance;

	GtkWidget *button;
	GtkWidget *popover; /* Only one at a time can be set */
	GMenuModel *model;

	GtkMenuButtonCreatePopupFunc create_popup_func;
	gpointer create_popup_user_data;
	GDestroyNotify create_popup_destroy_notify;

	GtkWidget *label_widget;
	GtkWidget *image_widget;
	GtkWidget *arrow_widget;
	GtkArrowType arrow_type;
	gboolean always_show_arrow;

	gboolean primary;
};

struct _GtkMenuButtonClass {
	GtkWidgetClass parent_class;

	void (* activate) (GtkMenuButton *self);
};
typedef struct _GtkMenuButtonClass GtkMenuButtonClass;
#endif /* GTK_CHECK_VERSION(4,0,0) */

// Use GtkButton or GtkMenuButton?
#if GTK_CHECK_VERSION(3,6,0)
typedef GtkMenuButtonClass superclass;
typedef GtkMenuButton super;
#  define GTK_TYPE_SUPER GTK_TYPE_MENU_BUTTON
#  define USE_GTK_MENU_BUTTON 1
#else /* !GTK_CHECK_VERSION(3,6,0) */
typedef GtkButtonClass superclass;
typedef GtkButton super;
#  define GTK_TYPE_SUPER GTK_TYPE_BUTTON
#endif

// Use GMenuModel?
// - glib-2.32: Adds GMenuModel.
// - GTK+ 3.4: Supports GMenuModel.
// - GTK+ 3.6: gtk_widget_insert_action_group()
#if GTK_CHECK_VERSION(3,6,0)
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3,6,0) */

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_DIRECTION,

	PROP_LAST
} OptionsMenuButtonPropID;

/* Signal identifiers */
typedef enum {
	SIGNAL_TRIGGERED,	// Menu item was triggered.

	SIGNAL_LAST
} OptionsMenuButtonSignalID;

static void	options_menu_button_dispose	(GObject	*object);
static void	options_menu_button_get_property(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	options_menu_button_set_property(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);

#ifndef USE_GTK_MENU_BUTTON
static gboolean	btnOptions_event_signal_handler (GtkButton	*button,
						 GdkEvent 	*event,
						 gpointer	user_data);
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
static void	action_triggered_signal_handler     (GSimpleAction	*action,
						     GVariant		*parameter,
						     gpointer		 user_data);
#else
static void	menuOptions_triggered_signal_handler(GtkMenuItem	*menuItem,
						     gpointer		 user_data);
#endif /* USE_G_MENU_MODEL */

// OptionsMenuButton class.
struct _OptionsMenuButtonClass {
	superclass __parent__;

	GParamSpec *properties[PROP_LAST];
	guint signal_ids[SIGNAL_LAST];
};

// OptionsMenuButton instance.
struct _OptionsMenuButton {
	super __parent__;

#ifdef USE_G_MENU_MODEL
	GMenu *menuModel;
	GMenu *menuRomOps;	// owned by menuModel
	// Map of GActions.
	// - Key: RomOp ID
	// - Value: GSimpleAction*
	std::unordered_map<int, GSimpleAction*> *actionMap;
	GSimpleActionGroup *actionGroup;
#else /* !USE_G_MENU_MODEL */
	GtkWidget *menuOptions;	// GtkMenu
#endif /* USE_G_MENU_MODEL */

#if !GTK_CHECK_VERSION(4,0,0)
	GtkWidget *imgOptions;	// up/down icon
#endif /* !GTK_CHECK_VERSION(4,0,0) */

#ifndef USE_GTK_MENU_BUTTON
	GtkArrowType arrowType;
#endif /* !USE_GTK_MENU_BUTTON */
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(OptionsMenuButton, options_menu_button,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

/** Standard actions. **/
struct option_menu_action_t {
	const char *desc;
	int id;
};
static const option_menu_action_t stdacts[] = {
	{NOP_C_("OptionsMenuButton|StdActs", "Export to Text..."),	OPTION_EXPORT_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Export to JSON..."),	OPTION_EXPORT_JSON},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as Text"),		OPTION_COPY_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as JSON"),		OPTION_COPY_JSON},
	{nullptr, 0}
};

static void
options_menu_button_class_init(OptionsMenuButtonClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = options_menu_button_dispose;
	gobject_class->get_property = options_menu_button_get_property;
	gobject_class->set_property = options_menu_button_set_property;

	/** Properties **/

	klass->properties[PROP_DIRECTION] = g_param_spec_enum(
		"direction", "Direction (up or down)", "Direction for the dropdown arrow.",
		GTK_TYPE_ARROW_TYPE, GTK_ARROW_UP,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

#ifdef USE_GTK_MENU_BUTTON
	// Override the "direction" property in GtkMenuButton.
	// FIXME: Verify that this works!
	g_object_class_override_property(gobject_class, PROP_DIRECTION, "direction");
#else /* !USE_GTK_MENU_BUTTON */
	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, klass->properties);
#endif /* USE_GTK_MENU_BUTTON */

	/** Signals **/

	klass->signal_ids[SIGNAL_TRIGGERED] = g_signal_new("triggered",
		TYPE_OPTIONS_MENU_BUTTON, G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL,
		G_TYPE_NONE, 1, G_TYPE_INT);
}

// TODO: Consolidate into common .cpp file.
static string
convert_accel_to_gtk(const char *str)
{
	// GTK+ uses '_' for accelerators, not '&'.
	string s_ret = str;
	size_t accel_pos = s_ret.find('&');
	if (accel_pos != string::npos) {
		s_ret[accel_pos] = '_';
	}
	return s_ret;
}

static void
options_menu_button_init(OptionsMenuButton *widget)
{
	string s_title = convert_accel_to_gtk(C_("OptionsMenuButton", "&Options"));

	// Initialize the direction image.
#if !GTK_CHECK_VERSION(4,0,0)
	widget->imgOptions = gtk_image_new();
#endif /* !GTK_CHECK_VERSION(4,0,0) */
#ifndef USE_GTK_MENU_BUTTON
	widget->arrowType = (GtkArrowType)-1;	// force update
#endif /* !USE_GTK_MENU_BUTTON */
	options_menu_button_set_direction(widget, GTK_ARROW_UP);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_menu_button_set_label(GTK_MENU_BUTTON(widget), s_title.c_str());
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const lblOptions = gtk_label_new(nullptr);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(lblOptions), s_title.c_str());
	gtk_widget_show(lblOptions);
#  if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *const hboxOptions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#  else /* !GTK_CHECK_VERSION(3,0,0) */
	GtkWidget *const hboxOptions = gtk_hbox_new(false, 4);
#  endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_widget_show(hboxOptions);

	// Add the label and image to the GtkBox.
	gtk_box_pack_start(GTK_BOX(hboxOptions), lblOptions, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxOptions), widget->imgOptions, false, false, 0);
	gtk_container_add(GTK_CONTAINER(widget), hboxOptions);
#endif /* GTK_CHECK_VERSION(4,0,0) */

#ifndef USE_GTK_MENU_BUTTON
	// Connect the superclass's "event" signal.
	// NOTE: We need to pass the event details. Otherwise, we'll
	// end up with the menu getting "stuck" to the mouse.
	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	g_signal_connect(widget, "event", G_CALLBACK(btnOptions_event_signal_handler), 0);
#endif /* !USE_GTK_MENU_BUTTON */

#if USE_G_MENU_MODEL
	widget->actionMap = new std::unordered_map<int, GSimpleAction*>();
	widget->actionGroup = nullptr;	// will be created later
#endif /* USE_G_MENU_MODEL */
}

static void
options_menu_button_dispose(GObject *object)
{
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(object);

#ifndef USE_GTK_MENU_BUTTON
	// Delete the "Options" button menu.
	if (widget->menuOptions) {
		gtk_widget_destroy(widget->menuOptions);
		widget->menuOptions = nullptr;
	}
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
	if (widget->menuModel) {
		g_object_unref(widget->menuModel);
		widget->menuModel = nullptr;
	}
	// owned by widget->menuModel
	widget->menuRomOps = nullptr;

	if (widget->actionMap) {
		delete widget->actionMap;
		widget->actionMap = nullptr;
	}
	if (widget->actionGroup) {
		// The GSimpleActionGroup owns the actions, so
		// this will automatically delete the actions.
		g_object_unref(widget->actionGroup);
		widget->actionGroup = nullptr;
	}
#endif /* USE_G_MENU_MODEL */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(options_menu_button_parent_class)->dispose(object);
}

GtkWidget*
options_menu_button_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_OPTIONS_MENU_BUTTON, nullptr));
}

/** Properties **/

static void
options_menu_button_get_property(GObject	*object,
				 guint		 prop_id,
				 GValue		*value,
				 GParamSpec	*pspec)
{
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(object);

	switch (prop_id) {
		case PROP_DIRECTION:
			g_value_set_enum(value, options_menu_button_get_direction(widget));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
options_menu_button_set_property(GObject	*object,
				 guint	 	 prop_id,
				 const GValue	*value,
				 GParamSpec	*pspec)
{
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(object);

	switch (prop_id) {
		case PROP_DIRECTION:
			options_menu_button_set_direction(widget, (GtkArrowType)g_value_get_enum(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

GtkArrowType
options_menu_button_get_direction(OptionsMenuButton *widget)
{
	g_return_val_if_fail(IS_OPTIONS_MENU_BUTTON(widget), GTK_ARROW_UP);
#ifdef USE_GTK_MENU_BUTTON
	return gtk_menu_button_get_direction(GTK_MENU_BUTTON(widget));
#else /* !USE_GTK_MENU_BUTTON */
	return widget->arrowType;
#endif /* USE_GTK_MENU_BUTTON */
}

void
options_menu_button_set_direction(OptionsMenuButton *widget, GtkArrowType arrowType)
{
	g_return_if_fail(IS_OPTIONS_MENU_BUTTON(widget));
#ifdef USE_GTK_MENU_BUTTON
	if (gtk_menu_button_get_direction(GTK_MENU_BUTTON(widget)) == arrowType)
		return;
#else /* !USE_GTK_MENU_BUTTON */
	if (widget->arrowType == arrowType)
		return;
#endif /* USE_GTK_MENU_BUTTON */

#if !GTK_CHECK_VERSION(4,0,0)
	const char *iconName;
	switch (arrowType) {
		case GTK_ARROW_UP:
			iconName = "pan-up-symbolic";
			break;
		case GTK_ARROW_DOWN:
			iconName = "pan-down-symbolic";
			break;
		case GTK_ARROW_LEFT:
			iconName = "pan-start-symbolic";
			break;
		case GTK_ARROW_RIGHT:
			iconName = "pan-end-symbolic";
			break;
		default:
			iconName = nullptr;
			break;
	}

	if (iconName) {
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->imgOptions), iconName, GTK_ICON_SIZE_BUTTON);
		gtk_widget_show(widget->imgOptions);
	} else {
		gtk_widget_hide(widget->imgOptions);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

#ifdef USE_GTK_MENU_BUTTON
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(widget), arrowType);
#else /* !USE_GTK_MENU_BUTTON */
	widget->arrowType = arrowType;
#endif /* USE_GTK_MENU_BUTTON */
}

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
btnOptions_menu_pos_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
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
 * "Options" button event handler. (Non-GtkMenuButton version)
 * @param button	GtkButton
 * @param event		GdkEvent
 * @param user_data	User data
 */
static gboolean
btnOptions_event_signal_handler(GtkButton *button,
				GdkEvent  *event,
				gpointer   user_data)
{
	RP_UNUSED(user_data);

	if (gdk_event_get_event_type(event) != GDK_BUTTON_PRESS) {
		// Tell the caller that we did NOT handle the event.
		return FALSE;
	}

	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(button);
	assert(widget->menuOptions != nullptr);
	if (!widget->menuOptions) {
		// No menu...
		return FALSE;
	}

	GtkMenuPositionFunc menuPositionFunc;
#if GTK_CHECK_VERSION(3,12,0)
	// If we're using a GtkHeaderBar, don't use a custom menu positioning function.
	if (gtk_dialog_get_header_bar(GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(widget)))) != nullptr) {
		menuPositionFunc = nullptr;
	} else
#endif /* GTK_CHECK_VERSION(3,12,0) */
	{
		menuPositionFunc = btnOptions_menu_pos_func;
	}

	// Pop up the menu.
	// FIXME: Improve button appearance so it's more pushed-in.
	guint button_id;
	if (!gdk_event_get_button(event, &button_id))
		return FALSE;

	gtk_menu_popup(GTK_MENU(widget->menuOptions),
		nullptr, nullptr,
		menuPositionFunc, button, button_id,
		gdk_event_get_time(event));

	// Tell the caller that we handled the event.
	return TRUE;
}
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
/**
 * An "Options" menu action was triggered.
 * @param action     	GSimpleAction (Get the "menuOptions_id" data.)
 * @param parameter	Parameter data
 * @param user_data	OptionsMenuButton
 */
static void
action_triggered_signal_handler(GSimpleAction	*action,
				GVariant	*parameter,
				gpointer	 user_data)
{
	RP_UNUSED(parameter);
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(user_data);
	OptionsMenuButtonClass *const klass = OPTIONS_MENU_BUTTON_GET_CLASS(widget);

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(action), "menuOptions_id"));

	g_signal_emit(widget, klass->signal_ids[SIGNAL_TRIGGERED], 0, id);
}
#else /* !USE_G_MENU_MODEL */
/**
 * An "Options" menu action was triggered.
 * @param menuItem     	Menu item (Get the "menuOptions_id" data.)
 * @param user_data	OptionsMenuButton
 */
static void
menuOptions_triggered_signal_handler(GtkMenuItem *menuItem,
				     gpointer user_data)
{
	OptionsMenuButton *const widget = OPTIONS_MENU_BUTTON(user_data);
	OptionsMenuButtonClass *const klass = OPTIONS_MENU_BUTTON_GET_CLASS(widget);

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(menuItem), "menuOptions_id"));

	g_signal_emit(widget, klass->signal_ids[SIGNAL_TRIGGERED], 0, id);
}
#endif /* USE_G_MENU_MODEL */

/** Menu manipulation functions **/

/**
 * Reset the menu items using the specified RomData object.
 * @param widget OptionsMenuButton
 * @param romData RomData object
 */
void
options_menu_button_reinit_menu(OptionsMenuButton *widget,
				const RomData *romData)
{
	g_return_if_fail(IS_OPTIONS_MENU_BUTTON(widget));

#if USE_G_MENU_MODEL
	GApplication *const pApp = g_application_get_default();
	assert(pApp != nullptr);
	g_return_if_fail(pApp != nullptr);

	char prefix[64];
	snprintf(prefix, sizeof(prefix), "rp-OptionsMenuButton-%p", widget);

	// Remove the existing GActionGroup from the widget.
	gtk_widget_insert_action_group(GTK_WIDGET(widget), prefix, nullptr);
	GSimpleActionGroup *const actionGroup = g_simple_action_group_new();
	widget->actionMap->clear();

	// GMenuModel does not have separator items per se.
	// Instead, we have to use separate sections:
	// - one for standard actions
	// - one for ROM operations
	GMenu *const menuModel = g_menu_new();
	GMenu *menuRomOps = nullptr;

	GMenu *const menuStdActs = g_menu_new();
	g_menu_append_section(menuModel, nullptr, G_MENU_MODEL(menuStdActs));
	for (const auto *p = stdacts; p->desc != nullptr; p++) {
		// Create the action.
		char buf[128];
		snprintf(buf, sizeof(buf), "%d", p->id);
		GSimpleAction *const action = g_simple_action_new(buf, nullptr);
		g_simple_action_set_enabled(action, TRUE);
		g_object_set_data(G_OBJECT(action), "menuOptions_id", GINT_TO_POINTER(p->id));
		g_signal_connect(action, "activate", G_CALLBACK(action_triggered_signal_handler), widget);
		g_action_map_add_action(G_ACTION_MAP(actionGroup), G_ACTION(action));
		widget->actionMap->emplace(p->id, action);

		// Create the menu item.
		snprintf(buf, sizeof(buf), "%s.%d", prefix, p->id);
		g_menu_append(menuStdActs, dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p->desc), buf);
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		// NOTE: The separator *does* show up properly with the KDE Breeze theme
		// after converting everything over to GMenuModel.
		menuRomOps = g_menu_new();
		g_menu_append_section(menuModel, nullptr, G_MENU_MODEL(menuRomOps));

		int i = 0;
		const auto ops_end = ops.cend();
		for (auto iter = ops.cbegin(); iter != ops_end; ++iter, i++) {
			// Create the action.
			char buf[128];
			snprintf(buf, sizeof(buf), "%d", i);
			GSimpleAction *const action = g_simple_action_new(buf, nullptr);
			g_simple_action_set_enabled(action, !!(iter->flags & RomData::RomOp::ROF_ENABLED));
			g_object_set_data(G_OBJECT(action), "menuOptions_id", GINT_TO_POINTER(i));
			g_signal_connect(action, "activate", G_CALLBACK(action_triggered_signal_handler), widget);
			g_action_map_add_action(G_ACTION_MAP(actionGroup), G_ACTION(action));
			widget->actionMap->emplace(i, action);

			// Create the menu item.
			const string desc = convert_accel_to_gtk(iter->desc);
			snprintf(buf, sizeof(buf), "%s.%d", prefix, i);
			g_menu_append(menuRomOps, desc.c_str(), buf);
		}
	}
#else /* !USE_G_MENU_MODEL */
	// Create a new GtkMenu.
	GtkWidget *const menuOptions = gtk_menu_new();

	for (const auto *p = stdacts; p->desc != nullptr; p++) {
		GtkWidget *const menuItem = gtk_menu_item_new_with_label(
			dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p->desc));
		g_object_set_data(G_OBJECT(menuItem), "menuOptions_id", GINT_TO_POINTER(p->id));
		g_signal_connect(menuItem, "activate", G_CALLBACK(menuOptions_triggered_signal_handler), widget);
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menuOptions), menuItem);
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		// NOTE: The separator *does* show up properly with the KDE Breeze theme
		// after converting everything over to GMenuModel.
		// Obviously it won't work in this code path, though...
		GtkWidget *menuItem = gtk_separator_menu_item_new();
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menuOptions), menuItem);

		int i = 0;
		const auto ops_end = ops.cend();
		for (auto iter = ops.cbegin(); iter != ops_end; ++iter, i++) {
			const string desc = convert_accel_to_gtk(iter->desc);
			menuItem = gtk_menu_item_new_with_mnemonic(desc.c_str());
			gtk_widget_set_sensitive(menuItem, !!(iter->flags & RomData::RomOp::ROF_ENABLED));
			g_object_set_data(G_OBJECT(menuItem), "menuOptions_id", GINT_TO_POINTER(i));
			g_signal_connect(menuItem, "activate", G_CALLBACK(menuOptions_triggered_signal_handler), widget);
			gtk_widget_show(menuItem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menuOptions), menuItem);
		}
	}
#endif /* USE_G_MENU_MODEL */

	// Replace the existing menu.
#ifdef USE_GTK_MENU_BUTTON
	// NOTE: GtkMenuButton takes ownership of the menu.
#  ifdef USE_G_MENU_MODEL
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(widget), G_MENU_MODEL(menuModel));
#  else /* !USE_G_MENU_MODEL */
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(widget), GTK_WIDGET(menuOptions));
#  endif /* USE_G_MENU_MODEL */
#else /* !USE_GTK_MENU_BUTTON */
#endif /* USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
	if (widget->menuModel) {
		g_object_unref(widget->menuModel);
	}
	widget->menuModel = menuModel;
	widget->menuRomOps = menuRomOps;
	if (widget->actionGroup) {
		g_object_unref(widget->actionGroup);
	}
	widget->actionGroup = actionGroup;
	gtk_widget_insert_action_group(GTK_WIDGET(widget), prefix, G_ACTION_GROUP(actionGroup));
#else /* !USE_G_MENU_MODEL */
	if (widget->menuOptions) {
		gtk_widget_destroy(widget->menuOptions);
	}
	widget->menuOptions = menuOptions;
#endif /* USE_G_MENU_MODEL */
}

/**
 * Update a ROM operation menu item.
 * @param widget OptionsMenuButton
 * @param id ROM operation ID
 * @param op ROM operation
 */
void
options_menu_button_update_op(OptionsMenuButton *widget,
			      int id,
			      const RomData::RomOp *op)
{
	g_return_if_fail(IS_OPTIONS_MENU_BUTTON(widget));
	g_return_if_fail(op != nullptr);
#ifndef USE_G_MENU_MODEL
	g_return_if_fail(GTK_IS_MENU(widget->menuOptions));
#endif /* USE_G_MENU_MODEL */

#ifdef USE_G_MENU_MODEL
	// Look up the GAction in the map.
	g_return_if_fail(widget->actionMap != nullptr);
	g_return_if_fail(G_IS_MENU_MODEL(widget->menuRomOps));

	auto iter = widget->actionMap->find(id);
	assert(iter != widget->actionMap->end());
	if (iter == widget->actionMap->end())
		return;

	// It seems we can't simply edit a menu item in place,
	// so we'll need to delete the old item and add the new one.
	assert(id >= 0);
	assert(id < g_menu_model_get_n_items(G_MENU_MODEL(widget->menuRomOps)));
	if (id < 0 || id >= g_menu_model_get_n_items(G_MENU_MODEL(widget->menuRomOps)))
		return;

	char buf[128];
	snprintf(buf, sizeof(buf), "rp-OptionsMenuButton-%p.%d", widget, id);
	g_menu_remove(widget->menuRomOps, id);

	const string desc = convert_accel_to_gtk(op->desc);
	g_menu_insert(widget->menuRomOps, id, desc.c_str(), buf);
	g_simple_action_set_enabled(iter->second, !!(op->flags & RomData::RomOp::ROF_ENABLED));
#else /* !USE_G_MENU_MODEL */
	GtkMenuItem *menuItem = nullptr;
	GList *l = gtk_container_get_children(GTK_CONTAINER(widget->menuOptions));

	// Skip the standard actions and separator.
	for (int i = 0; i < ARRAY_SIZE(stdacts) && l != nullptr; i++) {
		l = l->next;
	}

	// Find the matching menu item.
	while (l != nullptr) {
		GList *next = l->next;
		if (GTK_IS_MENU_ITEM(l->data)) {
			const gint item_id = (gboolean)GPOINTER_TO_INT(
				g_object_get_data(G_OBJECT(l->data), "menuOptions_id"));
			if (item_id == id) {
				// Found the menu item.
				menuItem = GTK_MENU_ITEM(l->data);
				break;
			}
		}
		l = next;
	}
	if (!menuItem)
		return;

	// Update the menu item.
	const string desc = convert_accel_to_gtk(op->desc);
	gtk_menu_item_set_label(menuItem, desc.c_str());
	gtk_widget_set_sensitive(GTK_WIDGET(menuItem), !!(op->flags & RomData::RomOp::ROF_ENABLED));
#endif /* USE_G_MENU_MODEL */
}

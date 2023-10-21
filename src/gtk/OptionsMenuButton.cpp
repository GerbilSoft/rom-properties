/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsMenuButton.cpp: Options menu GtkMenuButton container.            *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsMenuButton.hpp"
#include "PIMGTYPE.hpp"
#include "RpGtk.hpp"

// librpbase
using LibRpBase::RomData;

// C++ STL classes
using std::string;
using std::vector;

// NOTE: GtkMenuButton is final in GTK4, but not GTK3.
// As a result, RpOptionsMenuButton is now rewritten to be a
// GtkWidget (GTK4) or GtkContainer (GTK3, GTK2) that contains
// a GtkMenuButton (or GtkButton).

// Superclass should be GtkBox. (GtkHBox in GTK2)
// NOTE: We only have one child widget, so we don't
// have to explicitly set H vs. V for GtkBox.
// (default is horizontal?)
#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkHBoxClass superclass;
typedef GtkHBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_HBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// GtkMenuButton was added in GTK 3.6.
// GMenuModel is also implied by this, since GMenuModel
// support was added to GTK+ 3.4.
// NOTE: GtkMenu was removed from GTK4.
#if GTK_CHECK_VERSION(3,5,6)
#  define USE_GTK_MENU_BUTTON 1
#  define USE_G_MENU_MODEL 1
#endif /* GTK_CHECK_VERSION(3,5,6) */

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_DIRECTION,

	PROP_LAST
} OptionsMenuButtonPropID;

/* Signal identifiers */
typedef enum {
	// GtkButton signals
	SIGNAL_CLICKED,
	SIGNAL_ACTIVATE,

	// OptionsMenuButton signals
	SIGNAL_TRIGGERED,	// Menu item was triggered.

	SIGNAL_LAST
} OptionsMenuButtonSignalID;

static GQuark menuOptions_id_quark;

static void	rp_options_menu_button_dispose		(GObject	*object);
static void	rp_options_menu_button_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void	rp_options_menu_button_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);

static gboolean	menuButton_clicked_signal_handler	(GtkButton	*button,
							 RpOptionsMenuButton *widget);
static gboolean	menuButton_activate_signal_handler	(GtkButton	*button,
							 RpOptionsMenuButton *widget);
#ifndef USE_GTK_MENU_BUTTON
static gboolean	btnOptions_event_signal_handler (GtkButton	*button,
						 GdkEvent 	*event,
						 RpOptionsMenuButton *widget);
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
static void	action_triggered_signal_handler     (GSimpleAction	*action,
						     GVariant		*parameter,
						     RpOptionsMenuButton *widget);
#else
static void	menuOptions_triggered_signal_handler(GtkMenuItem	*menuItem,
						     RpOptionsMenuButton *widget);
#endif /* USE_G_MENU_MODEL */

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

// OptionsMenuButton class
struct _RpOptionsMenuButtonClass {
	superclass __parent__;
};

// OptionsMenuButton instance
struct _RpOptionsMenuButton {
	super __parent__;
	GtkWidget *menuButton;	// GtkMenuButton (or GtkButton)

#ifdef USE_G_MENU_MODEL
	GMenu *menuModel;
	GMenu *menuRomOps;	// owned by menuModel
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
G_DEFINE_TYPE_EXTENDED(RpOptionsMenuButton, rp_options_menu_button,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

/** Standard actions. **/
struct option_menu_action_t {
	const char *desc;
	int id;
};
static const std::array<option_menu_action_t, 4> stdacts = {{
	{NOP_C_("OptionsMenuButton|StdActs", "Export to Text..."),	OPTION_EXPORT_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Export to JSON..."),	OPTION_EXPORT_JSON},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as Text"),		OPTION_COPY_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as JSON"),		OPTION_COPY_JSON},
}};

static void
rp_options_menu_button_class_init(RpOptionsMenuButtonClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rp_options_menu_button_dispose;
	gobject_class->set_property = rp_options_menu_button_set_property;
	gobject_class->get_property = rp_options_menu_button_get_property;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	menuOptions_id_quark = g_quark_from_string("menuOptions_id");

	/** Properties **/

	props[PROP_DIRECTION] = g_param_spec_enum(
		"direction", "Direction (up or down)", "Direction for the dropdown arrow.",
		GTK_TYPE_ARROW_TYPE, GTK_ARROW_UP,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	// NOTE: Not overriding properties anymore because this widget
	// *contains* a GtkMenuButton instead of subclassing it.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

	/** Signals **/

	// GtkButton signals
	signals[SIGNAL_CLICKED] = g_signal_new("clicked",
		G_OBJECT_CLASS_TYPE(gobject_class),
		static_cast<GSignalFlags>(G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);
	signals[SIGNAL_ACTIVATE] = g_signal_new("activate",
		G_OBJECT_CLASS_TYPE(gobject_class),
		static_cast<GSignalFlags>(G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 0);

	// GtkMenuButton signals
	// TODO: G_SIGNAL_ACTION?
	signals[SIGNAL_TRIGGERED] = g_signal_new("triggered",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_INT);

	// Register the Activate signal.
	gtk_widget_class_set_activate_signal(GTK_WIDGET_CLASS(klass), signals[SIGNAL_ACTIVATE]);
}

static void
rp_options_menu_button_init(RpOptionsMenuButton *widget)
{
	const string s_title = convert_accel_to_gtk(C_("OptionsMenuButton", "&Options"));

	// Create the GtkMenuButton.
#ifdef USE_GTK_MENU_BUTTON
	widget->menuButton = gtk_menu_button_new();
#else /* !USE_GTK_MENU_BUTTON */
	widget->menuButton = gtk_button_new();
	widget->arrowType = (GtkArrowType)-1;	// force update
#endif /* USE_GTK_MENU_BUTTON */
	gtk_widget_set_name(widget->menuButton, "menuButton");

	// Initialize the direction image.
#if !GTK_CHECK_VERSION(4,0,0)
	widget->imgOptions = gtk_image_new();
	gtk_widget_set_name(widget->imgOptions, "imgOptions");
#endif /* !GTK_CHECK_VERSION(4,0,0) */
	rp_options_menu_button_set_direction(widget, GTK_ARROW_UP);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_menu_button_set_label(GTK_MENU_BUTTON(widget), s_title.c_str());
	gtk_menu_button_set_use_underline(GTK_MENU_BUTTON(widget), TRUE);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_widget_show(widget->menuButton);	// needed for GTK2/GTK3 but not GTK4

	GtkWidget *const lblOptions = gtk_label_new(nullptr);
	gtk_widget_set_name(lblOptions, "lblOptions");
	gtk_label_set_text_with_mnemonic(GTK_LABEL(lblOptions), s_title.c_str());
	gtk_widget_show(lblOptions);
	GtkWidget *const hboxOptions = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxOptions, "hboxOptions");
	gtk_widget_show(hboxOptions);

	// Add the label and image to the GtkBox.
	gtk_box_pack_start(GTK_BOX(hboxOptions), lblOptions, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxOptions), widget->imgOptions, false, false, 0);
	gtk_container_add(GTK_CONTAINER(widget->menuButton), hboxOptions);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Add the menu button to the container widget.
#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(widget), widget->menuButton);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_container_add(GTK_CONTAINER(widget), widget->menuButton);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Connect the wrapper signals.
	g_signal_connect(widget->menuButton, "clicked", G_CALLBACK(menuButton_clicked_signal_handler), widget);
	g_signal_connect(widget->menuButton, "activate", G_CALLBACK(menuButton_activate_signal_handler), widget);

#ifndef USE_GTK_MENU_BUTTON
	// Connect the button's "event" signal.
	// NOTE: We need to pass the event details. Otherwise, we'll
	// end up with the menu getting "stuck" to the mouse.
	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	g_signal_connect(widget->menuButton, "event", G_CALLBACK(btnOptions_event_signal_handler), widget);
#endif /* !USE_GTK_MENU_BUTTON */

#if USE_G_MENU_MODEL
	widget->actionGroup = nullptr;	// will be created later
#endif /* USE_G_MENU_MODEL */
}

static void
rp_options_menu_button_dispose(GObject *object)
{
	RpOptionsMenuButton *const widget = RP_OPTIONS_MENU_BUTTON(object);

	// NOTE: widget->menuButton is owned by the GtkBox,
	// so we don't have to delete it manually.

#ifndef USE_GTK_MENU_BUTTON
	// Delete the "Options" button menu.
	if (widget->menuOptions) {
		gtk_widget_destroy(widget->menuOptions);
		widget->menuOptions = nullptr;
	}
#endif /* !USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
	g_clear_object(&widget->menuModel);
	// owned by widget->menuModel
	widget->menuRomOps = nullptr;

	// The GSimpleActionGroup owns the actions, so
	// this will automatically delete the actions.
	g_clear_object(&widget->actionGroup);
#endif /* USE_G_MENU_MODEL */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_options_menu_button_parent_class)->dispose(object);
}

GtkWidget*
rp_options_menu_button_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_OPTIONS_MENU_BUTTON, nullptr));
}

/** Properties **/

static void
rp_options_menu_button_set_property(GObject		*object,
				    guint	 	 prop_id,
				    const GValue	*value,
				    GParamSpec		*pspec)
{
	RpOptionsMenuButton *const widget = RP_OPTIONS_MENU_BUTTON(object);

	switch (prop_id) {
		case PROP_DIRECTION:
			rp_options_menu_button_set_direction(widget, (GtkArrowType)g_value_get_enum(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_options_menu_button_get_property(GObject	*object,
				    guint	 prop_id,
				    GValue	*value,
				    GParamSpec	*pspec)
{
	RpOptionsMenuButton *const widget = RP_OPTIONS_MENU_BUTTON(object);

	switch (prop_id) {
		case PROP_DIRECTION:
			g_value_set_enum(value, rp_options_menu_button_get_direction(widget));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

GtkArrowType
rp_options_menu_button_get_direction(RpOptionsMenuButton *widget)
{
	g_return_val_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget), GTK_ARROW_UP);
#ifdef USE_GTK_MENU_BUTTON
	return gtk_menu_button_get_direction(GTK_MENU_BUTTON(widget->menuButton));
#else /* !USE_GTK_MENU_BUTTON */
	return widget->arrowType;
#endif /* USE_GTK_MENU_BUTTON */
}

void
rp_options_menu_button_set_direction(RpOptionsMenuButton *widget, GtkArrowType arrowType)
{
	g_return_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget));
#ifdef USE_GTK_MENU_BUTTON
	if (gtk_menu_button_get_direction(GTK_MENU_BUTTON(widget->menuButton)) == arrowType)
		return;
#else /* !USE_GTK_MENU_BUTTON */
	if (widget->arrowType == arrowType)
		return;
#endif /* USE_GTK_MENU_BUTTON */

#if !GTK_CHECK_VERSION(4,0,0)
	static const char iconName_tbl[][20] = {
		"pan-up-symbolic",
		"pan-down-symbolic",
		"pan-start-symbolic",
		"pan-end-symbolic",
	};
	
	const char *iconName = nullptr;
	if (arrowType >= GTK_ARROW_UP && arrowType <= GTK_ARROW_RIGHT) {
		iconName = iconName_tbl[(int)arrowType];
	}

	if (iconName) {
		gtk_image_set_from_icon_name(GTK_IMAGE(widget->imgOptions), iconName, GTK_ICON_SIZE_BUTTON);
		gtk_widget_show(widget->imgOptions);
	} else {
		gtk_widget_hide(widget->imgOptions);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

#ifdef USE_GTK_MENU_BUTTON
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(widget->menuButton), arrowType);
#else /* !USE_GTK_MENU_BUTTON */
	widget->arrowType = arrowType;
#endif /* USE_GTK_MENU_BUTTON */
}

static gboolean
menuButton_clicked_signal_handler(GtkButton *button, RpOptionsMenuButton *widget)
{
	RP_UNUSED(button);
	g_return_val_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget), FALSE);
	g_signal_emit(GTK_WIDGET(widget), signals[SIGNAL_CLICKED], 0);
	return TRUE;
}

static gboolean
menuButton_activate_signal_handler(GtkButton *button, RpOptionsMenuButton *widget)
{
	RP_UNUSED(button);
	g_return_val_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget), FALSE);
	g_signal_emit(GTK_WIDGET(widget), signals[SIGNAL_ACTIVATE], 0);
	return TRUE;
}

#ifndef USE_GTK_MENU_BUTTON
/**
 * Menu positioning function.
 * @param menu		[in] GtkMenu
 * @param x		[out] X position
 * @param y		[out] Y position
 * @param push_in
 * @param button	[in] GtkWidget the menu is attached to.
 */
static void
btnOptions_menu_pos_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *button)
{
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
 * @param widget	OptionsMenuButton
 */
static gboolean
btnOptions_event_signal_handler(GtkButton *button, GdkEvent *event, RpOptionsMenuButton *widget)
{
	RP_UNUSED(button);
	g_return_val_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget), FALSE);
	g_return_val_if_fail(GTK_IS_MENU(widget->menuOptions), FALSE);

	if (gdk_event_get_event_type(event) != GDK_BUTTON_PRESS) {
		// Tell the caller that we did NOT handle the event.
		return FALSE;
	}

	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	GtkMenuPositionFunc menuPositionFunc;
#if GTK_CHECK_VERSION(3,12,0)
	// If we're using a GtkHeaderBar, don't use a custom menu positioning function.
	if (gtk_dialog_get_header_bar(gtk_widget_get_toplevel_dialog(GTK_WIDGET(widget))) != nullptr) {
		menuPositionFunc = nullptr;
	} else
#endif /* GTK_CHECK_VERSION(3,12,0) */
	{
		menuPositionFunc = (GtkMenuPositionFunc)btnOptions_menu_pos_func;
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
 * @param widget	OptionsMenuButton
 */
static void
action_triggered_signal_handler(GSimpleAction *action, GVariant *parameter, RpOptionsMenuButton *widget)
{
	g_return_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget));
	RP_UNUSED(parameter);

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(action), menuOptions_id_quark));

	g_signal_emit(widget, signals[SIGNAL_TRIGGERED], 0, id);
}
#else /* !USE_G_MENU_MODEL */
/**
 * An "Options" menu action was triggered.
 * @param menuItem     	Menu item (Get the "menuOptions_id" data.)
 * @param widget	OptionsMenuButton
 */
static void
menuOptions_triggered_signal_handler(GtkMenuItem *menuItem,
				     RpOptionsMenuButton *widget)
{
	g_return_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget));

	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(menuItem), menuOptions_id_quark));

	g_signal_emit(widget, signals[SIGNAL_TRIGGERED], 0, id);
}
#endif /* USE_G_MENU_MODEL */

/** Menu manipulation functions **/

/**
 * Reset the menu items using the specified RomData object.
 * @param widget RpOptionsMenuButton
 * @param romData RomData object
 */
void
rp_options_menu_button_reinit_menu(RpOptionsMenuButton *widget,
				   const RomData *romData)
{
	g_return_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget));

#if USE_G_MENU_MODEL
	char prefix[64];
	snprintf(prefix, sizeof(prefix), "rp-OptionsMenuButton-%p", widget);

	// Remove the existing GActionGroup from the widget.
	gtk_widget_insert_action_group(GTK_WIDGET(widget), prefix, nullptr);
	GSimpleActionGroup *const actionGroup = g_simple_action_group_new();

	// GMenuModel does not have separator items per se.
	// Instead, we have to use separate sections:
	// - one for standard actions
	// - one for ROM operations
	GMenu *const menuModel = g_menu_new();
	GMenu *menuRomOps = nullptr;

	GMenu *const menuStdActs = g_menu_new();
	g_menu_append_section(menuModel, nullptr, G_MENU_MODEL(menuStdActs));
	for (const option_menu_action_t &p : stdacts) {
		// Create the action.
		char buf[128];
		snprintf(buf, sizeof(buf), "%d", p.id);
		GSimpleAction *const action = g_simple_action_new(buf, nullptr);
		g_simple_action_set_enabled(action, TRUE);
		g_object_set_qdata(G_OBJECT(action), menuOptions_id_quark, GINT_TO_POINTER(p.id));
		g_signal_connect(action, "activate", G_CALLBACK(action_triggered_signal_handler), widget);
		g_action_map_add_action(G_ACTION_MAP(actionGroup), G_ACTION(action));

		// Create the menu item.
		snprintf(buf, sizeof(buf), "%s.%d", prefix, p.id);
		g_menu_append(menuStdActs, dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p.desc), buf);
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		// NOTE: The separator *does* show up properly with the KDE Breeze theme
		// after converting everything over to GMenuModel.
		menuRomOps = g_menu_new();
		g_menu_append_section(menuModel, nullptr, G_MENU_MODEL(menuRomOps));

		int i = 0;
		for (const RomData::RomOp &op : ops) {
			// Create the action.
			char buf[128];
			snprintf(buf, sizeof(buf), "%d", i);
			GSimpleAction *const action = g_simple_action_new(buf, nullptr);
			g_simple_action_set_enabled(action, !!(op.flags & RomData::RomOp::ROF_ENABLED));
			g_object_set_qdata(G_OBJECT(action), menuOptions_id_quark, GINT_TO_POINTER(i));
			g_signal_connect(action, "activate", G_CALLBACK(action_triggered_signal_handler), widget);
			g_action_map_add_action(G_ACTION_MAP(actionGroup), G_ACTION(action));

			// Create the menu item.
			const string desc = convert_accel_to_gtk(op.desc);
			snprintf(buf, sizeof(buf), "%s.%d", prefix, i);
			g_menu_append(menuRomOps, desc.c_str(), buf);

			// Next operation.
			i++;
		}
	}
#else /* !USE_G_MENU_MODEL */
	// Create a new GtkMenu.
	GtkWidget *const menuOptions = gtk_menu_new();
	gtk_widget_set_name(menuOptions, "menuOptions");

	for (const option_menu_action_t &p : stdacts) {
		GtkWidget *const menuItem = gtk_menu_item_new_with_label(
			dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p.desc));
		// NOTE: No name for this GtkWidget.
		g_object_set_qdata(G_OBJECT(menuItem), menuOptions_id_quark, GINT_TO_POINTER(p.id));
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
		// NOTE: No name for this GtkWidget.
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menuOptions), menuItem);

		int i = 0;
		for (const RomData::RomOp &op : ops) {
			const string desc = convert_accel_to_gtk(op.desc);
			menuItem = gtk_menu_item_new_with_mnemonic(desc.c_str());
			// NOTE: No name for this GtkWidget.
			gtk_widget_set_sensitive(menuItem, !!(op.flags & RomData::RomOp::ROF_ENABLED));
			g_object_set_qdata(G_OBJECT(menuItem), menuOptions_id_quark, GINT_TO_POINTER(i));
			g_signal_connect(menuItem, "activate", G_CALLBACK(menuOptions_triggered_signal_handler), widget);
			gtk_widget_show(menuItem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menuOptions), menuItem);

			// Next operation.
			i++;
		}
	}
#endif /* USE_G_MENU_MODEL */

	// Replace the existing menu.
#ifdef USE_GTK_MENU_BUTTON
	// NOTE: GtkMenuButton does NOT take ownership of the menu.
#  ifdef USE_G_MENU_MODEL
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(widget->menuButton), G_MENU_MODEL(menuModel));
#  else /* !USE_G_MENU_MODEL */
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(widget->menuButton), GTK_WIDGET(menuOptions));
#  endif /* USE_G_MENU_MODEL */
#endif /* USE_GTK_MENU_BUTTON */

#ifdef USE_G_MENU_MODEL
	g_clear_object(&widget->menuModel);
	widget->menuModel = menuModel;
	widget->menuRomOps = menuRomOps;
	g_clear_object(&widget->actionGroup);
	widget->actionGroup = actionGroup;
	gtk_widget_insert_action_group(GTK_WIDGET(widget), prefix, G_ACTION_GROUP(actionGroup));
#else /* !USE_G_MENU_MODEL */
	g_clear_object(&widget->menuOptions);
	widget->menuOptions = menuOptions;
#endif /* USE_G_MENU_MODEL */
}

/**
 * Update a ROM operation menu item.
 * @param widget RpOptionsMenuButton
 * @param id ROM operation ID
 * @param op ROM operation
 */
void
rp_options_menu_button_update_op(RpOptionsMenuButton *widget,
				 int id,
				 const RomData::RomOp *op)
{
	g_return_if_fail(RP_IS_OPTIONS_MENU_BUTTON(widget));
	g_return_if_fail(op != nullptr);
#ifndef USE_G_MENU_MODEL
	g_return_if_fail(GTK_IS_MENU(widget->menuOptions));
#endif /* USE_G_MENU_MODEL */

#ifdef USE_G_MENU_MODEL
	// Look up the GAction in the map.
	char action_name[16];
	snprintf(action_name, sizeof(action_name), "%d", id);
	GSimpleAction *const action = G_SIMPLE_ACTION(
		g_action_map_lookup_action(G_ACTION_MAP(widget->actionGroup), action_name));
	if (!action)
		return;

	// It seems we can't simply edit a menu item in place,
	// so we'll need to delete the old item and add the new one.
	g_return_if_fail(G_IS_MENU_MODEL(widget->menuRomOps));
	assert(id >= 0);
	assert(id < g_menu_model_get_n_items(G_MENU_MODEL(widget->menuRomOps)));
	if (id < 0 || id >= g_menu_model_get_n_items(G_MENU_MODEL(widget->menuRomOps)))
		return;

	char buf[128];
	snprintf(buf, sizeof(buf), "rp-OptionsMenuButton-%p.%d", widget, id);

	g_menu_remove(widget->menuRomOps, id);
	const string desc = convert_accel_to_gtk(op->desc);
	g_simple_action_set_enabled(action, !!(op->flags & RomData::RomOp::ROF_ENABLED));
	g_menu_insert(widget->menuRomOps, id, desc.c_str(), buf);
#else /* !USE_G_MENU_MODEL */
	GtkMenuItem *menuItem = nullptr;
	GList *l = gtk_container_get_children(GTK_CONTAINER(widget->menuOptions));

	// Skip the standard actions and separator.
	for (size_t i = 0; i < (stdacts.size()+1) && l != nullptr; i++) {
		l = l->next;
	}

	// Find the matching menu item.
	while (l != nullptr) {
		GList *next = l->next;
		if (GTK_IS_MENU_ITEM(l->data)) {
			const gint item_id = (gboolean)GPOINTER_TO_INT(
				g_object_get_qdata(G_OBJECT(l->data), menuOptions_id_quark));
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

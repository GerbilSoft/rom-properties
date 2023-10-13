/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.cpp: Language GtkComboBox subclass.                    *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LanguageComboBox.hpp"
#include "PIMGTYPE.hpp"

#ifdef USE_GTK_DROP_DOWN
#  include "LanguageComboBoxItem.h"
#endif /* USE_GTK_DROP_DOWN */

// librpbase
using namespace LibRpBase;

// C++ STL classes
using std::string;

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_SELECTED_LC,
	PROP_FORCE_PAL,

	PROP_LAST
} RpLanguageComboBoxPropID;

/* Signal identifiers */
typedef enum {
	SIGNAL_LC_CHANGED,	// Language code was changed.

	SIGNAL_LAST
} RpLanguageComboBoxSignalID;

#ifndef USE_GTK_DROP_DOWN
/* Column identifiers */
typedef enum {
	SM_COL_ICON,
	SM_COL_TEXT,
	SM_COL_LC,
} StringMultiColumns;
#endif /* !USE_GTK_DROP_DOWN */

static void	rp_language_combo_box_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec);
static void	rp_language_combo_box_get_property(GObject	*object,
						   guint	 prop_id,
						   GValue	*value,
						   GParamSpec	*pspec);

static void	rp_language_combo_box_dispose     (GObject	*object);

/** Signal handlers **/
#ifdef USE_GTK_DROP_DOWN
static void	rp_language_combo_box_notify_selected_handler(GtkDropDown		*dropDown,
							      GParamSpec		*pspec,
							      RpLanguageComboBox	*widget);
#else /* !USE_GTK_DROP_DOWN */
static void	rp_language_combo_box_changed_handler(GtkComboBox		*comboBox,
						      RpLanguageComboBox	*widget);
#endif /* USE_GTK_DROP_DOWN */

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkHBoxClass superclass;
typedef GtkHBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_HBOX
#endif

// LanguageComboBox class
struct _RpLanguageComboBoxClass {
	superclass __parent__;
};

// LanguageComboBox instance
struct _RpLanguageComboBox {
	super __parent__;

#ifdef USE_GTK_DROP_DOWN
	GtkWidget *dropDown;
	GListStore *listStore;
#else /* !USE_GTK_DROP_DOWN */
	GtkWidget *comboBox;
	GtkListStore *listStore;
#endif /* USE_GTK_DROP_DOWN */

	gboolean forcePAL;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpLanguageComboBox, rp_language_combo_box,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rp_language_combo_box_class_init(RpLanguageComboBoxClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_language_combo_box_set_property;
	gobject_class->get_property = rp_language_combo_box_get_property;
	gobject_class->dispose = rp_language_combo_box_dispose;

	/** Properties **/

	props[PROP_SELECTED_LC] = g_param_spec_uint(
		"selected-lc", "Selected LC", "Selected language code.",
		0U, ~0U, 0U,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_FORCE_PAL] = g_param_spec_boolean(
		"force-pal", "Force PAL", "Force PAL regions.",
		false,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

	/** Signals **/

	signals[SIGNAL_LC_CHANGED] = g_signal_new("lc-changed",
		G_OBJECT_CLASS_TYPE(gobject_class),
		G_SIGNAL_RUN_LAST,
		0, nullptr, nullptr, nullptr,
		G_TYPE_NONE, 1, G_TYPE_UINT);
}

#ifdef USE_GTK_DROP_DOWN
// GtkSignalListItemFactory signal handlers
// Reference: https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
static void
setup_listitem_cb(GtkListItemFactory	*factory,
		  GtkListItem		*list_item,
		  gpointer		 user_data)
{
	RP_UNUSED(factory);
	RP_UNUSED(user_data);

	GtkWidget *const icon = gtk_image_new();
	GtkWidget *const label = gtk_label_new(nullptr);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);

	GtkWidget *const hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_append(GTK_BOX(hbox), icon);
	gtk_box_append(GTK_BOX(hbox), label);
	gtk_list_item_set_child(list_item, hbox);
}

static void
bind_listitem_cb(GtkListItemFactory	*factory,
                 GtkListItem		*list_item,
		 gpointer		 user_data)
{
	RP_UNUSED(factory);
	RP_UNUSED(user_data);

	GtkWidget *const hbox = gtk_list_item_get_child(list_item);
	GtkWidget *const icon = gtk_widget_get_first_child(hbox);
	GtkWidget *const label = gtk_widget_get_next_sibling(icon);
	assert(icon != nullptr);
	assert(label != nullptr);
	if (!icon || !label) {
		return;
	}

	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(gtk_list_item_get_item(list_item));
	if (!item) {
		return;
	}

	gtk_image_set_from_paintable(GTK_IMAGE(icon), GDK_PAINTABLE(rp_language_combo_box_item_get_icon(item)));
	gtk_label_set_text(GTK_LABEL(label), rp_language_combo_box_item_get_name(item));
}
#endif /* USE_GTK_DROP_DOWN */

static void
rp_language_combo_box_init(RpLanguageComboBox *widget)
{
#ifdef USE_GTK_DROP_DOWN
	// Create the GListStore
	widget->listStore = g_list_store_new(RP_TYPE_LANGUAGE_COMBO_BOX_ITEM);

	// Create the GtkDropDown widget
	// NOTE: GtkDropDown takes ownership of widget->listStore.
	widget->dropDown = gtk_drop_down_new(G_LIST_MODEL(widget->listStore), nullptr);
	gtk_box_append(GTK_BOX(widget), widget->dropDown);

	// Create the GtkSignalListItemFactory
	GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
	g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb), nullptr);
	g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), nullptr);
	gtk_drop_down_set_factory(GTK_DROP_DOWN(widget->dropDown), factory);
	g_object_unref(factory);	// we don't need to keep a reference
#else /* !USE_GTK_DROP_DOWN */
	// Create the GtkComboBox widget.
	widget->comboBox = gtk_combo_box_new();
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(widget), widget->comboBox);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(widget), widget->comboBox, TRUE, TRUE, 0);
	gtk_widget_show(widget->comboBox);
#  endif /* GTK_CHECK_VERSION(4,0,0) */

	// Create the GtkListStore
	widget->listStore = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_UINT);
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget->comboBox), GTK_TREE_MODEL(widget->listStore));

	// Remove our reference on widget->listStore.
	// The GtkComboBox will automatically destroy it.
	g_object_unref(widget->listStore);
#endif /* USE_GTK_DROP_DOWN */

#ifndef USE_GTK_DROP_DOWN
	/** Cell renderers **/

	// Icon renderer
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget->comboBox), renderer, false);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget->comboBox),
		renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, SM_COL_ICON, nullptr);

	// Text renderer
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget->comboBox), renderer, true);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget->comboBox),
		renderer, "text", SM_COL_TEXT, nullptr);
#endif /* !USE_GTK_DROP_DOWN */

	/** Signals **/

#ifdef USE_GTK_DROP_DOWN
	// GtkDropDown doesn't have a "changed" signal, and its
	// GtkSelectionModel object isn't accessible.
	// Listen for GObject::notify for the "selected" property.
	g_signal_connect(widget->dropDown, "notify::selected", G_CALLBACK(rp_language_combo_box_notify_selected_handler), widget);
#else /* !USE_GTK_DROP_DOWN */
	// Connect the "changed" signal.
	g_signal_connect(widget->comboBox, "changed", G_CALLBACK(rp_language_combo_box_changed_handler), widget);
#endif /* USE_GTK_DROP_DOWN */
}

GtkWidget*
rp_language_combo_box_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_LANGUAGE_COMBO_BOX, nullptr));
}

/** Properties **/

static void
rp_language_combo_box_set_property(GObject	*object,
				   guint	 prop_id,
				   const GValue	*value,
				   GParamSpec	*pspec)
{
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	switch (prop_id) {
		case PROP_SELECTED_LC:
			rp_language_combo_box_set_selected_lc(widget, g_value_get_uint(value));
			break;

		case PROP_FORCE_PAL:
			rp_language_combo_box_set_force_pal(widget, g_value_get_boolean(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_language_combo_box_get_property(GObject	*object,
				   guint	 prop_id,
				   GValue	*value,
				   GParamSpec	*pspec)
{
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	switch (prop_id) {
		case PROP_SELECTED_LC:
			g_value_set_uint(value, rp_language_combo_box_get_selected_lc(widget));
			break;

		case PROP_FORCE_PAL:
			g_value_set_boolean(value, widget->forcePAL);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Dispose / Finalize **/

static void
rp_language_combo_box_dispose(GObject *object)
{
#ifdef USE_GTK_DROP_DOWN
	RpLanguageComboBox *const widget = RP_LANGUAGE_COMBO_BOX(object);

	if (widget->dropDown) {
		gtk_drop_down_set_factory(GTK_DROP_DOWN(widget->dropDown), nullptr);
	}
#endif /* USE_GTK_DROP_DOWN */

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_language_combo_box_parent_class)->dispose(object);
}

/** Property accessors / mutators **/

/**
 * Rebuild the language icons.
 * @param widget RpLanguageComboBox
 */
static void
rp_language_combo_box_rebuild_icons(RpLanguageComboBox *widget)
{
	// TODO:
	// - High-DPI scaling on GTK+ earlier than 3.10
	// - Fractional scaling
	// - Runtime adjustment via "configure" event
	// Reference: https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#gdk-window-get-scale-factor
	const unsigned int iconSize = 16;
#if GTK_CHECK_VERSION(3,10,0)
#  if 0
	// FIXME: gtk_widget_get_window() doesn't work unless the window is realized.
	// We might need to initialize the dropdown in the "realize" signal handler.
	GdkWindow *const gdk_window = gtk_widget_get_window(GTK_WIDGET(widget));
	assert(gdk_window != nullptr);
	if (gdk_window) {
		const gint scale_factor = gdk_window_get_scale_factor(gdk_window);
		if (scale_factor >= 2) {
			// 2x scaling or higher.
			// TODO: Larger icon sizes?
			iconSize = 32;
		}
	}
#  endif /* 0 */
#endif /* GTK_CHECK_VERSION(3,10,0) */

	// Load the flags sprite sheet.
	char flags_filename[64];
	snprintf(flags_filename, sizeof(flags_filename),
		"/com/gerbilsoft/rom-properties/flags/flags-%ux%u.png",
		iconSize, iconSize);
	PIMGTYPE flags_spriteSheet = PIMGTYPE_load_png_from_gresource(flags_filename);
	assert(flags_spriteSheet != nullptr);

#ifdef USE_GTK_DROP_DOWN
	const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(widget->listStore));
	for (guint i = 0; i < n_items; i++) {
		RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(
			g_list_model_get_item(G_LIST_MODEL(widget->listStore), i));
		assert(item != nullptr);
		if (!item)
			continue;

		if (flags_spriteSheet) {
			const uint32_t lc = rp_language_combo_box_item_get_lc(item);

			int col, row;
			if (!SystemRegion::getFlagPosition(lc, &col, &row, widget->forcePAL)) {
				// Found a matching icon.
				PIMGTYPE icon = PIMGTYPE_get_subsurface(flags_spriteSheet,
					col*iconSize, row*iconSize, iconSize, iconSize);
				rp_language_combo_box_item_set_icon(item, icon);
				PIMGTYPE_unref(icon);
			} else {
				// No icon. Clear it.
				rp_language_combo_box_item_set_icon(item, nullptr);
			}
		} else {
			// Clear the icon.
			rp_language_combo_box_item_set_icon(item, nullptr);
		}
	}
#else /* !USE_GTK_DROP_DOWN */
	GtkTreeIter iter;
	GtkTreeModel *const treeModel = GTK_TREE_MODEL(widget->listStore);
	gboolean ok = gtk_tree_model_get_iter_first(treeModel, &iter);
	if (!ok) {
		// No iterators...
		return;
	}

	do {
		if (flags_spriteSheet) {
			uint32_t lc = 0;
			gtk_tree_model_get(treeModel, &iter, SM_COL_LC, &lc, -1);

			int col, row;
			if (!SystemRegion::getFlagPosition(lc, &col, &row, widget->forcePAL)) {
				// Found a matching icon.
				PIMGTYPE icon = PIMGTYPE_get_subsurface(flags_spriteSheet,
					col*iconSize, row*iconSize, iconSize, iconSize);
				gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, icon, -1);
				PIMGTYPE_unref(icon);
			} else {
				// No icon. Clear it.
				gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, nullptr, -1);
			}
		} else {
			// Clear the icon.
			gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, nullptr, -1);
		}

		// Next row.
		ok = gtk_tree_model_iter_next(treeModel, &iter);
	} while (ok);
#endif /* USE_GTK_DROP_DOWN */

	if (flags_spriteSheet) {
		PIMGTYPE_unref(flags_spriteSheet);
	}
}

/**
 * Set the language codes.
 * @param widget RpLanguageComboBox
 * @param lcs_array 0-terminated array of language codes.
 */
void
rp_language_combo_box_set_lcs(RpLanguageComboBox *widget, const uint32_t *lcs_array)
{
	g_return_if_fail(lcs_array != nullptr);

	// Check the LC of the selected index.
	const uint32_t sel_lc = rp_language_combo_box_get_selected_lc(widget);

	// Populate the GtkListStore / GListStore.
#ifdef USE_GTK_DROP_DOWN
	g_list_store_remove_all(widget->listStore);

	guint sel_idx = GTK_INVALID_LIST_POSITION;
	for (guint cur_idx = 0; *lcs_array != 0; lcs_array++, cur_idx++) {
		const uint32_t lc = *lcs_array;
		const char *const name = SystemRegion::getLocalizedLanguageName(lc);

		RpLanguageComboBoxItem *const item = rp_language_combo_box_item_new(nullptr, name, lc);
		if (!name) {
			// Invalid language code.
			rp_language_combo_box_item_set_name(item, SystemRegion::lcToString(lc).c_str());
		}
		g_list_store_append(widget->listStore, item);

		if (sel_lc != 0 && lc == sel_lc) {
			// This was the previously-selected LC.
			sel_idx = cur_idx;
		}
	}

	// Rebuild icons.
	rp_language_combo_box_rebuild_icons(widget);

	// Re-select the previously-selected LC.
	gtk_drop_down_set_selected(GTK_DROP_DOWN(widget->dropDown), sel_idx);
#else /* !USE_GTK_DROP_DOWN */
	gtk_list_store_clear(widget->listStore);

	int sel_idx = -1;
	for (int cur_idx = 0; *lcs_array != 0; lcs_array++, cur_idx++) {
		const uint32_t lc = *lcs_array;
		const char *const name = SystemRegion::getLocalizedLanguageName(lc);

		GtkTreeIter iter;
		gtk_list_store_append(widget->listStore, &iter);
		gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, nullptr, SM_COL_LC, lc, -1);
		if (name) {
			gtk_list_store_set(widget->listStore, &iter, SM_COL_TEXT, name, -1);
		} else {
			// Invalid language code.
			gtk_list_store_set(widget->listStore, &iter, SM_COL_TEXT,
				SystemRegion::lcToString(lc).c_str(), -1);
		}

		if (sel_lc != 0 && lc == sel_lc) {
			// This was the previously-selected LC.
			sel_idx = cur_idx;
		}
	}

	// Rebuild icons.
	rp_language_combo_box_rebuild_icons(widget);

	// Re-select the previously-selected LC.
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget->comboBox), sel_idx);
#endif /* USE_GTK_DROP_DOWN */
}

/**
 * Get the set of language codes.
 * Caller must g_free() the returned array.
 * @param widget RpLanguageComboBox
 * @return 0-terminated array of language codes, or nullptr if none.
 */
uint32_t*
rp_language_combo_box_get_lcs(RpLanguageComboBox *widget)
{
#ifdef USE_GTK_DROP_DOWN
	const int count = (int)g_list_model_get_n_items(G_LIST_MODEL(widget->listStore));
#else /* USE_GTK_DROP_DOWN */
	const int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(widget->listStore), nullptr);
#endif /* USE_GTK_DROP_DOWN */
	assert(count <= 1024);
	if (count <= 0 || count > 1024) {
		// No language codes, or too many language codes.
		return nullptr;
	}

	uint32_t *const lcs_array = static_cast<uint32_t*>(g_malloc_n(count+1, sizeof(uint32_t)));
	uint32_t *p = lcs_array;
	uint32_t *const p_end = lcs_array + count;

#ifdef USE_GTK_DROP_DOWN
	for (guint i = 0; i < (guint)count && p < p_end; i++) {
		RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(
			g_list_model_get_item(G_LIST_MODEL(widget->listStore), i));
		assert(item != nullptr);
		if (!item)
			continue;

		const uint32_t lc = rp_language_combo_box_item_get_lc(item);
		if (lc != 0) {
			*p++ = lc;
		}
	}
#else /* USE_GTK_DROP_DOWN */
	GtkTreeIter iter;
	gboolean ok = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(widget->listStore), &iter);
	while (ok && p < p_end) {
		GValue value = G_VALUE_INIT;
		gtk_tree_model_get_value(GTK_TREE_MODEL(widget->listStore), &iter, SM_COL_LC, &value);
		const uint32_t lc = g_value_get_uint(&value);
		g_value_unset(&value);

		if (lc != 0) {
			*p++ = lc;
		}

		// Next row.
		ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(widget->listStore), &iter);
	}
#endif /* USE_GTK_DROP_DOWN */

	// Last entry is 0.
	*p = 0;
	return lcs_array;
}

/**
 * Clear the language codes.
 * @param widget RpLanguageComboBox
 */
void
rp_language_combo_box_clear_lcs(RpLanguageComboBox *widget)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget));

#ifdef USE_GTK_DROP_DOWN
	const guint cur_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(widget->dropDown));
	g_list_store_remove_all(widget->listStore);

	if (cur_idx != GTK_INVALID_LIST_POSITION) {
		// Nothing is selected now.
		g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, 0);
	}
#else /* !USE_GTK_DROP_DOWN */
	const int cur_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	gtk_list_store_clear(widget->listStore);

	if (cur_idx >= 0) {
		// Nothing is selected now.
		g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, 0);
	}
#endif /* USE_GTK_DROP_DOWN */
}

/**
 * Set the selected language code.
 *
 * NOTE: This function will return true if the LC was found,
 * even if it was already selected.
 *
 * @param widget RpLanguageComboBox
 * @param lc Language code. (0 to unselect)
 * @return True if set; false if LC was not found.
 */
gboolean
rp_language_combo_box_set_selected_lc(RpLanguageComboBox *widget, uint32_t lc)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget), false);

	// Check if this LC is already selected.
	if (lc == rp_language_combo_box_get_selected_lc(widget)) {
		// Already selected.
		return true;
	}

	bool bRet;
#ifdef USE_GTK_DROP_DOWN
	if (lc == 0) {
		// Unselect the selected LC.
		gtk_drop_down_set_selected(GTK_DROP_DOWN(widget->dropDown), GTK_INVALID_LIST_POSITION);
		bRet = true;
	} else {
		// Find an item with a matching LC.
		bRet = false;

		const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(widget->listStore));
		for (guint i = 0; i < n_items; i++) {
			RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(
				g_list_model_get_item(G_LIST_MODEL(widget->listStore), i));
			assert(item != nullptr);
			if (!item)
				continue;

			const uint32_t check_lc = rp_language_combo_box_item_get_lc(item);
			if (lc == check_lc) {
				// Found it.
				gtk_drop_down_set_selected(GTK_DROP_DOWN(widget->dropDown), i);
				bRet = true;
				break;
			}
		}
	}
#else /* USE_GTK_DROP_DOWN */
	if (lc == 0) {
		// Unselect the selected LC.
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget->comboBox), -1);
		bRet = true;
	} else {
		// Find an item with a matching LC.
		bRet = false;
		GtkTreeIter iter;
		gboolean ok = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(widget->listStore), &iter);
		while (ok) {
			GValue value = G_VALUE_INIT;
			gtk_tree_model_get_value(GTK_TREE_MODEL(widget->listStore), &iter, SM_COL_LC, &value);
			const uint32_t check_lc = g_value_get_uint(&value);
			g_value_unset(&value);

			if (lc == check_lc) {
				// Found it.
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget->comboBox), &iter);
				bRet = true;
				break;
			}

			// Next row.
			ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(widget->listStore), &iter);
		}
	}
#endif /* USE_GTK_DROP_DOWN */

	// FIXME: If called from rp_language_combo_box_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_SELECTED_LC]);

	// NOTE: rp_language_combo_box_changed_handler will emit SIGNAL_LC_CHANGED,
	// so we don't need to emit it here.
	return bRet;
}

/**
 * Get the selected language code.
 * @param widget RpLanguageComboBox
 * @return Selected language code. (0 if none)
 */
uint32_t
rp_language_combo_box_get_selected_lc(RpLanguageComboBox *widget)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget), 0);
	uint32_t sel_lc = 0;

#ifdef USE_GTK_DROP_DOWN
	const gpointer obj = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(widget->dropDown));
	if (obj) {
		RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(obj);
		assert(item != nullptr);
		if (item) {
			sel_lc = rp_language_combo_box_item_get_lc(item);
		}
	}
#else /* !USE_GTK_DROP_DOWN */
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget->comboBox), &iter)) {
		GValue value = G_VALUE_INIT;
		gtk_tree_model_get_value(GTK_TREE_MODEL(widget->listStore), &iter, SM_COL_LC, &value);
		sel_lc = g_value_get_uint(&value);
		g_value_unset(&value);
	}
#endif /* USE_GTK_DROP_DOWN */

	return sel_lc;
}

/**
 * Set the Force PAL setting.
 * @param forcePAL Force PAL setting.
 */
void
rp_language_combo_box_set_force_pal(RpLanguageComboBox *widget, gboolean forcePAL)
{
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget));

	if (widget->forcePAL == forcePAL)
		return;
	widget->forcePAL = forcePAL;
	rp_language_combo_box_rebuild_icons(widget);
}

/**
 * Get the Force PAL setting.
 * @return Force PAL setting.
 */
gboolean
rp_language_combo_box_get_force_pal(RpLanguageComboBox *widget)
{
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget), false);
	return widget->forcePAL;
}

#ifdef USE_GTK_DROP_DOWN
/**
 * Internal GObject "notify" signal handler for GtkDropDown's "selected" property.
 * @param dropDown GtkDropDown sending the signal
 * @param pspec Property specification
 * @param widget RpLanguageComboBox
 */
static void
rp_language_combo_box_notify_selected_handler(GtkDropDown		*dropDown,
					      GParamSpec		*pspec,
					      RpLanguageComboBox	*widget)
{
	RP_UNUSED(dropDown);
	RP_UNUSED(pspec);
	const uint32_t lc = rp_language_combo_box_get_selected_lc(widget);
	g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, lc);
}
#else /* !USE_GTK_DROP_DOWN */
/**
 * Internal signal handler for GtkComboBox's "changed" signal.
 * @param comboBox GtkComboBox sending the signal
 * @param widget RpLanguageComboBox
 */
static void
rp_language_combo_box_changed_handler(GtkComboBox		*comboBox,
				      RpLanguageComboBox	*widget)
{
	RP_UNUSED(comboBox);
	const uint32_t lc = rp_language_combo_box_get_selected_lc(widget);
	g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, lc);
}
#endif /* USE_GTK_DROP_DOWN */

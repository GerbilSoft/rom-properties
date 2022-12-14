/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.hpp: Language GtkComboBox subclass.                    *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LanguageComboBox.hpp"
#include "PIMGTYPE.hpp"

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

/* Column identifiers */
typedef enum {
	SM_COL_ICON,
	SM_COL_TEXT,
	SM_COL_LC,
} StringMultiColumns;

static void	rp_language_combo_box_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec);
static void	rp_language_combo_box_get_property(GObject	*object,
						   guint	 prop_id,
						   GValue	*value,
						   GParamSpec	*pspec);

/** Signal handlers. **/
static void	internal_changed_handler	(RpLanguageComboBox *widget,
						 gpointer          user_data);

static GParamSpec *props[PROP_LAST];
static guint signals[SIGNAL_LAST];

// RpLanguageComboBox class
struct _RpLanguageComboBoxClass {
	GtkComboBoxClass __parent__;
};

// RpLanguageComboBox instance
struct _RpLanguageComboBox {
	GtkComboBox __parent__;

	GtkListStore *listStore;
	gboolean forcePAL;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpLanguageComboBox, rp_language_combo_box,
	GTK_TYPE_COMBO_BOX, static_cast<GTypeFlags>(0), {});

static void
rp_language_combo_box_class_init(RpLanguageComboBoxClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_language_combo_box_set_property;
	gobject_class->get_property = rp_language_combo_box_get_property;

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
		RP_TYPE_LANGUAGE_COMBO_BOX, G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL,
		G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
rp_language_combo_box_init(RpLanguageComboBox *widget)
{
	// Create the GtkListStore.
	widget->listStore = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_UINT);
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget), GTK_TREE_MODEL(widget->listStore));

	// Remove our reference on widget->listStore.
	// The superclass will automatically destroy it.
	g_object_unref(widget->listStore);

	/** Cell renderers **/

	// Icon renderer
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, false);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget),
		renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, SM_COL_ICON, NULL);

	// Text renderer
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, true);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget),
		renderer, "text", SM_COL_TEXT, NULL);

	/** Signals **/

	// Connect the "changed" signal.
	g_signal_connect(widget, "changed", G_CALLBACK(internal_changed_handler), nullptr);
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

/**
 * Rebuild the language icons.
 * @param widget RpLanguageComboBox
 */
void
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

	GtkTreeIter iter;
	GtkTreeModel *const treeModel = GTK_TREE_MODEL(widget->listStore);
	gboolean ok = gtk_tree_model_get_iter_first(treeModel, &iter);
	if (!ok) {
		// No iterators...
		return;
	}

	// Load the flags sprite sheet.
	char flags_filename[64];
	snprintf(flags_filename, sizeof(flags_filename),
		"/com/gerbilsoft/rom-properties/flags/flags-%ux%u.png",
		iconSize, iconSize);
	PIMGTYPE flags_spriteSheet = PIMGTYPE_load_png_from_gresource(flags_filename);
	assert(flags_spriteSheet != nullptr);

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
				PIMGTYPE_destroy(icon);
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

	if (flags_spriteSheet) {
		PIMGTYPE_destroy(flags_spriteSheet);
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

	// Populate the GtkListStore.
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
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), sel_idx);
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
	const int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(widget->listStore), nullptr);
	assert(count <= 1024);
	if (count <= 0 || count > 1024) {
		// No language codes, or too many language codes.
		return nullptr;
	}

	uint32_t *const lcs_array = static_cast<uint32_t*>(g_malloc_n(count+1, sizeof(uint32_t)));
	uint32_t *p = lcs_array;
	uint32_t *const p_end = lcs_array + count;

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
	const int cur_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	gtk_list_store_clear(widget->listStore);
	if (cur_idx >= 0) {
		// Nothing is selected now.
		g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, 0);
	}
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
	// Check if this LC is already selected.
	if (lc == rp_language_combo_box_get_selected_lc(widget)) {
		// Already selected.
		return true;
	}

	bool bRet;
	if (lc == 0) {
		// Unselect the selected LC.
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), -1);
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
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), &iter);
				bRet = true;
				break;
			}

			// Next row.
			ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(widget->listStore), &iter);
		}
	}

	// FIXME: If called from rp_language_combo_box_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_SELECTED_LC]);

	// NOTE: internal_changed_handler will emit SIGNAL_LC_CHANGED,
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
	uint32_t sel_lc = 0;
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		GValue value = G_VALUE_INIT;
		gtk_tree_model_get_value(GTK_TREE_MODEL(widget->listStore), &iter, SM_COL_LC, &value);
		sel_lc = g_value_get_uint(&value);
		g_value_unset(&value);
	}
	return sel_lc;
}

/**
 * Set the Force PAL setting.
 * @param forcePAL Force PAL setting.
 */
void
rp_language_combo_box_set_force_pal(RpLanguageComboBox *widget, gboolean forcePAL)
{
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
	return widget->forcePAL;
}

/**
 * Internal signal handler for GtkComboBox "changed".
 * @param widget RpLanguageComboBox
 * @param user_data
 */
static void
internal_changed_handler(RpLanguageComboBox *widget,
			 gpointer          user_data)
{
	RP_UNUSED(user_data);
	const uint32_t lc = rp_language_combo_box_get_selected_lc(widget);
	g_signal_emit(widget, signals[SIGNAL_LC_CHANGED], 0, lc);
}

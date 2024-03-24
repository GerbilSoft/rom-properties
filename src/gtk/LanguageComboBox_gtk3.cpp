/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LanguageComboBox.cpp: Language GtkComboBox subclass (GTK2/GTK3-specific)*
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PIMGTYPE.hpp"

#include "LanguageComboBox.hpp"
#include "LanguageComboBox_p.hpp"

#include "FlagSpriteSheet.hpp"

// librpbase (for SystemRegion)
using namespace LibRpBase;

/* Column identifiers */
typedef enum {
	SM_COL_ICON,
	SM_COL_TEXT,
	SM_COL_LC,
} StringMultiColumns;

/** Signal handlers **/
static void	rp_language_combo_box_changed_handler(GtkComboBox		*comboBox,
						      RpLanguageComboBox	*widget);

/**
 * Initialize the GTK3/GTK4-specific portion of the LanguageComboBox.
 * @param widget RpLanguageComboBox
 */
void rp_language_combo_box_init_gtkX(struct _RpLanguageComboBox *widget)
{
	// Create the GtkComboBox widget.
	widget->comboBox = gtk_combo_box_new();
	gtk_box_pack_start(GTK_BOX(widget), widget->comboBox, TRUE, TRUE, 0);
	gtk_widget_show(widget->comboBox);

	// Create the GtkListStore
	widget->listStore = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_UINT);
	gtk_combo_box_set_model(GTK_COMBO_BOX(widget->comboBox), GTK_TREE_MODEL(widget->listStore));

	// Remove our reference on widget->listStore.
	// The GtkComboBox will automatically destroy it.
	g_object_unref(widget->listStore);

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

	/** Signals **/

	// Connect the "changed" signal.
	g_signal_connect(widget->comboBox, "changed", G_CALLBACK(rp_language_combo_box_changed_handler), widget);
}

/**
 * Rebuild the language icons.
 * @param widget RpLanguageComboBox
 */
void
rp_language_combo_box_rebuild_icons(struct _RpLanguageComboBox *widget)
{
	// TODO:
	// - High-DPI scaling on GTK+ earlier than 3.10
	// - Fractional scaling
	// - Runtime adjustment via "configure" event
	// Reference: https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#gdk-window-get-scale-factor
	static const int iconSize = 16;

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

	// Flag sprite sheet
	FlagSpriteSheet flagSpriteSheet(iconSize);

	GtkTreeIter iter;
	GtkTreeModel *const treeModel = GTK_TREE_MODEL(widget->listStore);
	gboolean ok = gtk_tree_model_get_iter_first(treeModel, &iter);
	if (!ok) {
		// No iterators...
		return;
	}

	do {
		uint32_t lc = 0;
		gtk_tree_model_get(treeModel, &iter, SM_COL_LC, &lc, -1);
		PIMGTYPE icon = flagSpriteSheet.getIcon(lc, widget->forcePAL);
		if (icon) {
			// Found a matching icon.
			gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, icon, -1);
			PIMGTYPE_unref(icon);
		} else {
			// No icon. Clear it.
			gtk_list_store_set(widget->listStore, &iter, SM_COL_ICON, nullptr, -1);
		}

		// Next row.
		ok = gtk_tree_model_iter_next(treeModel, &iter);
	} while (ok);
}

/** Property accessors / mutators **/

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
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget->comboBox), sel_idx);
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
	g_return_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget));

	const int cur_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	gtk_list_store_clear(widget->listStore);

	if (cur_idx >= 0) {
		// Nothing is selected now.
		g_signal_emit(widget, rp_language_combo_box_signals[SIGNAL_LC_CHANGED], 0, 0);
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
	g_return_val_if_fail(RP_IS_LANGUAGE_COMBO_BOX(widget), false);

	// Check if this LC is already selected.
	if (lc == rp_language_combo_box_get_selected_lc(widget)) {
		// Already selected.
		return true;
	}

	bool bRet;
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

	// NOTE: rp_language_combo_box_changed_handler will emit SIGNAL_LC_CHANGED,
	// so we don't need to emit it here.
	g_object_notify_by_pspec(G_OBJECT(widget), rp_language_combo_box_props[PROP_SELECTED_LC]);
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

	GtkTreeIter iter;
	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget->comboBox), &iter)) {
		return 0;
	}

	GValue value = G_VALUE_INIT;
	gtk_tree_model_get_value(GTK_TREE_MODEL(widget->listStore), &iter, SM_COL_LC, &value);
	const uint32_t sel_lc = g_value_get_uint(&value);
	g_value_unset(&value);

	return sel_lc;
}

/** Signal handlers **/

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
	g_signal_emit(widget, rp_language_combo_box_signals[SIGNAL_LC_CHANGED], 0, lc);
}

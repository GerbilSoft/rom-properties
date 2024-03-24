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
#include "LanguageComboBoxItem.h"

#include "FlagSpriteSheet.hpp"

// librpbase (for SystemRegion)
using namespace LibRpBase;

/** Signal handlers **/
static void	rp_language_combo_box_notify_selected_handler(GtkDropDown		*dropDown,
							      GParamSpec		*pspec,
							      RpLanguageComboBox	*widget);

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

/**
 * Initialize the GTK3/GTK4-specific portion of the LanguageComboBox.
 * @param widget RpLanguageComboBox
 */
void rp_language_combo_box_init_gtkX(struct _RpLanguageComboBox *widget)
{
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

	/** Signals **/

	// GtkDropDown doesn't have a "changed" signal, and its
	// GtkSelectionModel object isn't accessible.
	// Listen for GObject::notify for the "selected" property.
	g_signal_connect(widget->dropDown, "notify::selected", G_CALLBACK(rp_language_combo_box_notify_selected_handler), widget);
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

	const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(widget->listStore));
	for (guint i = 0; i < n_items; i++) {
		RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(
			g_list_model_get_item(G_LIST_MODEL(widget->listStore), i));
		assert(item != nullptr);
		if (!item)
			continue;

		const uint32_t lc = rp_language_combo_box_item_get_lc(item);
		PIMGTYPE icon = flagSpriteSheet.getIcon(lc, widget->forcePAL);
		if (icon) {
			// Found a matching icon.
			rp_language_combo_box_item_set_icon(item, icon);
			PIMGTYPE_unref(icon);
		} else {
			// No icon. Clear it.
			rp_language_combo_box_item_set_icon(item, nullptr);
		}
	}
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

	// Populate the GtkListStore / GListStore.
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
		g_object_unref(item);

		if (sel_lc != 0 && lc == sel_lc) {
			// This was the previously-selected LC.
			sel_idx = cur_idx;
		}
	}

	// Rebuild icons.
	rp_language_combo_box_rebuild_icons(widget);

	// Re-select the previously-selected LC.
	gtk_drop_down_set_selected(GTK_DROP_DOWN(widget->dropDown), sel_idx);
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
	const int count = (int)g_list_model_get_n_items(G_LIST_MODEL(widget->listStore));
	assert(count <= 1024);
	if (count <= 0 || count > 1024) {
		// No language codes, or too many language codes.
		return nullptr;
	}

	uint32_t *const lcs_array = static_cast<uint32_t*>(g_malloc_n(count+1, sizeof(uint32_t)));
	uint32_t *p = lcs_array;
	uint32_t *const p_end = lcs_array + count;

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

	const guint cur_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(widget->dropDown));
	g_list_store_remove_all(widget->listStore);

	if (cur_idx != GTK_INVALID_LIST_POSITION) {
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

	const gpointer obj = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(widget->dropDown));
	if (!obj) {
		return 0;
	}

	RpLanguageComboBoxItem *const item = RP_LANGUAGE_COMBO_BOX_ITEM(obj);
	assert(item != nullptr);
	if (!item) {
		return 0;
	}

	return rp_language_combo_box_item_get_lc(item);
}

/** Signal handlers **/

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
	g_signal_emit(widget, rp_language_combo_box_signals[SIGNAL_LC_CHANGED], 0, lc);
}

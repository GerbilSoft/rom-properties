/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView_gtk3.cpp: RomData viewer widget. (GTK4-specific)            *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RomDataFormat.hpp"

#include "ListDataItem.h"
#include "sort_funcs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpTexture;

// C++ STL classes
using std::set;
using std::string;
using std::vector;

// TODO: Ideal icon size? Using 32x32 for now.
static const int icon_sz = 32;

// Format tables.
// Pango enum values are known to fit in uint8_t.
static const gfloat align_tbl_xalign[4] = {
	// Order: TXA_D, TXA_L, TXA_C, TXA_R
	0.0f, 0.0f, 0.5f, 1.0f
};
static const uint8_t align_tbl_halign[4] = {
	// Order: TXA_D, TXA_L, TXA_C, TXA_R
	GTK_ALIGN_START, GTK_ALIGN_START,
	GTK_ALIGN_CENTER, GTK_ALIGN_END
};

// GtkSignalListItemFactory signal handlers
// Reference: https://blog.gtk.org/2020/09/05/a-primer-on-gtklistview/
// NOTE: user_data is RpListDataItemCol0Type.

// Column 0 setup: user_data == RpListDataItemCol0Type
static void
setup_listitem_cb_col0(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	const RpListDataItemCol0Type col0_type = (RpListDataItemCol0Type)GPOINTER_TO_INT(user_data);
	switch (col0_type) {
		default:
			assert(!"Unsupported col0_type!");
			break;

		case RP_LIST_DATA_ITEM_COL0_TYPE_TEXT:
			assert(!"col0 setup should only be used for checkbox or icon!");
			break;

		case RP_LIST_DATA_ITEM_COL0_TYPE_CHECKBOX:
			gtk_list_item_set_child(list_item, gtk_check_button_new());
			break;

		case RP_LIST_DATA_ITEM_COL0_TYPE_ICON: {
			GtkWidget *const picture = gtk_picture_new();
			gtk_widget_set_size_request(picture, icon_sz, icon_sz);
			gtk_list_item_set_child(list_item, picture);
			break;
		}
	}
}

// Text column setup: user_data == (col_attrs.align_data & RomFields::TXA_MASK)
static void
setup_listitem_cb_text(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	const uint32_t align_data = (GPOINTER_TO_UINT(user_data) & RomFields::TXA_MASK);

	GtkWidget *const label = gtk_label_new(nullptr);
	gtk_label_set_xalign(GTK_LABEL(label), align_tbl_xalign[align_data]);
	gtk_widget_set_halign(label, static_cast<GtkAlign>(align_tbl_halign[align_data]));
	gtk_list_item_set_child(list_item, label);
}

static void
bind_listitem_cb(GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
	RP_UNUSED(factory);

	GtkWidget *const widget = gtk_list_item_get_child(list_item);
	assert(widget != nullptr);
	if (!widget)
		return;

	// user_data is the column number.
	// If has_icon is set, column 0 is the icon; text starts at column 1.
	// Otherwise, text starts at column 0.
	RpListDataItem *const item = RP_LIST_DATA_ITEM(gtk_list_item_get_item(list_item));
	if (!item)
		return;

	const int column = GPOINTER_TO_INT(user_data);
	switch (rp_list_data_item_get_col0_type(item)) {
		default:
			assert(!"Unsupported col0_type!");
			return;
		case RP_LIST_DATA_ITEM_COL0_TYPE_TEXT:
			// No icon or checkbox.
			gtk_label_set_markup(GTK_LABEL(widget), rp_list_data_item_get_column_text(item, column));
			break;
		case RP_LIST_DATA_ITEM_COL0_TYPE_CHECKBOX:
			// Column 0 is a checkbox.
			if (column == 0) {
				gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), rp_list_data_item_get_checked(item));
			} else {
				gtk_label_set_markup(GTK_LABEL(widget), rp_list_data_item_get_column_text(item, column-1));
			}
			break;
		case RP_LIST_DATA_ITEM_COL0_TYPE_ICON:
			// Column 0 is an icon.
			if (column == 0) {
				gtk_picture_set_paintable(GTK_PICTURE(widget), GDK_PAINTABLE(rp_list_data_item_get_icon(item)));
			} else {
				gtk_label_set_markup(GTK_LABEL(widget), rp_list_data_item_get_column_text(item, column-1));
			}
			break;
	}
}

/**
 * Initialize a list data field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
GtkWidget*
rp_rom_data_view_init_listdata(RpRomDataView *page, const RomFields::Field &field)
{
	// ListData type. Create a GtkListStore for the data.
	const auto &listDataDesc = field.desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	// Single language ListData_t.
	// For RFT_LISTDATA_MULTI, this is only used for row and column count.
	const RomFields::ListData_t *list_data;
	const bool isMulti = !!(field.flags & RomFields::RFT_LISTDATA_MULTI);
	if (isMulti) {
		// Multiple languages.
		const auto *const multi = field.data.list_data.data.multi;
		assert(multi != nullptr);
		assert(!multi->empty());
		if (!multi || multi->empty()) {
			// No data...
			return nullptr;
		}

		list_data = &multi->cbegin()->second;
	} else {
		// Single language.
		list_data = field.data.list_data.data.single;
	}

	assert(list_data != nullptr);
	if (!list_data) {
		// No data...
		return nullptr;
	}

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(field.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(field.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		return nullptr;
	}

	if (hasIcons) {
		assert(field.data.list_data.mxd.icons != nullptr);
		if (!field.data.list_data.mxd.icons) {
			// No icons vector...
			return nullptr;
		}
	}

	int colCount = 1;
	if (listDataDesc.names) {
		colCount = static_cast<int>(listDataDesc.names->size());
	} else {
		// No column headers.
		// Use the first row.
		colCount = static_cast<int>(list_data->at(0).size());
	}
	assert(colCount > 0);
	if (colCount <= 0) {
		// No columns...
		return nullptr;
	}

	// Create the GListStore and GtkColumnView.
	// NOTE: Each column will need its own GtkColumnViewColumn and GtkSignalListItemFactory.
	GListStore *const listStore = g_list_store_new(RP_TYPE_LIST_DATA_ITEM);

	// Create the GtkColumnView.
	GtkWidget *const columnView = gtk_column_view_new(nullptr);
	// FIXME: GtkColumnView doesn't expose a function to hide column headers.
	// We'll have to manually hide them.
	if (!listDataDesc.names) {
		gtk_widget_set_visible(gtk_widget_get_first_child(columnView), false);
	}

	// GtkColumnView requires a GtkSelectionModel, so we'll create
	// a GtkSingleSelection to wrap around the GListStore.
	GtkSingleSelection *const selModel = gtk_single_selection_new(G_LIST_MODEL(listStore));
	gtk_column_view_set_model(GTK_COLUMN_VIEW(columnView), GTK_SELECTION_MODEL(selModel));
	g_object_unref(selModel);

	// NOTE: Regarding object ownership:
	// - GtkColumnViewColumn takes ownership of the GtkListItemFactory
	// - GtkColumnView takes ownership of the GtkColumnViewColumn
	// As such, neither the factory nor the column objects will be unref()'d here.

	// Create the columns.
	RpListDataItemCol0Type col0_type;
	int listStore_col_start;
	if (hasCheckboxes || hasIcons) {
		// Prepend an extra column for checkboxes or icons.
		col0_type = (hasCheckboxes) ? RP_LIST_DATA_ITEM_COL0_TYPE_CHECKBOX : RP_LIST_DATA_ITEM_COL0_TYPE_ICON;
		GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb_col0), GINT_TO_POINTER(col0_type));
		g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), GINT_TO_POINTER(0));

		GtkColumnViewColumn *const column = gtk_column_view_column_new(nullptr, factory);
		gtk_column_view_column_set_fixed_width(column, icon_sz);
		gtk_column_view_append_column(GTK_COLUMN_VIEW(columnView), column);

		listStore_col_start = 1;	// Skip the checkbox column for strings.
	} else {
		// All strings.
		col0_type = RP_LIST_DATA_ITEM_COL0_TYPE_TEXT;
		listStore_col_start = 0;
	}

	// Create the remaining columns.
	RomFields::ListDataColAttrs_t col_attrs = listDataDesc.col_attrs;
	for (int i = 0; i < colCount; i++, col_attrs.shiftRight()) {
		// Prepend an extra column for checkboxes or icons.
		GtkListItemFactory *const factory = gtk_signal_list_item_factory_new();
		g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem_cb_text), GUINT_TO_POINTER(col_attrs.align_data & RomFields::TXA_MASK));
		g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem_cb), GINT_TO_POINTER(i + listStore_col_start));

		// NOTE: Not skipping empty column names.
		// TODO: Hide them.
		GtkColumnViewColumn *const column = gtk_column_view_column_new(
			(listDataDesc.names ? listDataDesc.names->at(i).c_str() : ""), factory);
		gtk_column_view_append_column(GTK_COLUMN_VIEW(columnView), column);

#if 0
		// Header/data alignment
		g_object_set(column,
			"alignment", align_tbl_xalign[col_attrs.align_headers & RomFields::TXA_MASK],
			nullptr);
#endif

		// Column sizing
		// NOTE: We don't have direct equivalents to QHeaderView::ResizeMode.
		switch (col_attrs.sizing & RomFields::COLSZ_MASK) {
			case RomFields::ColSizing::COLSZ_INTERACTIVE:
				gtk_column_view_column_set_resizable(column, true);
				//gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
				break;
			/*case RomFields::ColSizing::COLSZ_FIXED:
				gtk_column_view_column_set_resizable(column, true);
				gtk_tree_view_column_set_sizing(column, FIXED);
				break;*/
			case RomFields::ColSizing::COLSZ_STRETCH:
				// TODO: Wordwrapping and/or text elision?
				// NOTE: Allowing the user to resize the column because
				// unlike Qt, we can't shrink it by shrinking the window.
				gtk_column_view_column_set_resizable(column, true);
				gtk_column_view_column_set_expand(column, true);
				//gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
				break;
			case RomFields::ColSizing::COLSZ_RESIZETOCONTENTS:
				gtk_column_view_column_set_resizable(column, true);
				//gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
				break;
		}

#if 0
		// Enable sorting.
		gtk_tree_view_column_set_sort_column_id(column, listStore_col_idx);
		gtk_tree_view_column_set_clickable(column, TRUE);

		// Check what we should use for sorting.
		// NOTE: We're setting the sorting functions on the proxy model.
		// That way, it won't affect the underlying data, which ensures
		// that RFT_LISTDATA_MULTI is still handled correctly.
		// NOTE 2: On GTK3, "standard sorting" seems to be case-insensitive.
		// Not sure if this will be changed, so we'll explicitly sort with
		// case-sensitivity for that case.
		switch (col_attrs.sorting & RomFields::COLSORT_MASK) {
			default:
				// Unsupported. We'll use standard sorting.
				assert(!"Unsupported sorting method.");
				// fall-through
			case RomFields::COLSORT_STANDARD:
				// Standard sorting.
				gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sortProxy),
					listStore_col_idx, sort_RFT_LISTDATA_standard,
					GINT_TO_POINTER(listStore_col_idx), nullptr);
				break;
			case RomFields::COLSORT_NOCASE:
				// Case-insensitive sorting.
				gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sortProxy),
					listStore_col_idx, sort_RFT_LISTDATA_nocase,
					GINT_TO_POINTER(listStore_col_idx), nullptr);
				break;
			case RomFields::COLSORT_NUMERIC:
				// Numeric sorting. (case-insensitive)
				gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sortProxy),
					listStore_col_idx, sort_RFT_LISTDATA_numeric,
					GINT_TO_POINTER(listStore_col_idx), nullptr);
				break;
		}
#endif
	}

	// Add the row data.
	uint32_t checkboxes = 0;
	if (hasCheckboxes) {
		checkboxes = field.data.list_data.mxd.checkboxes;
	}
	unsigned int row = 0;	// for icons [TODO: Use iterator?]
	for (const vector<string> &data_row : *list_data) {
		// FIXME: Skip even if we don't have checkboxes?
		// (also check other UI frontends)
		if (hasCheckboxes && data_row.empty()) {
			// Skip this row.
			checkboxes >>= 1;
			row++;
			continue;
		}

		RpListDataItem *const item = rp_list_data_item_new(colCount, col0_type);
		if (hasCheckboxes) {
			// Checkbox column
			rp_list_data_item_set_checked(item, (checkboxes & 1));
			checkboxes >>= 1;
		} else if (hasIcons) {
			// Icon column
			const rp_image_const_ptr &icon = field.data.list_data.mxd.icons->at(row);
			if (icon) {
				PIMGTYPE pixbuf = rp_image_to_PIMGTYPE(icon);
				if (pixbuf) {
					// NOTE: GtkPicture *can* scale the pixbuf itself.
					// Using GtkPicture to scale it instead of scaling here.
					rp_list_data_item_set_icon(item, pixbuf);
					PIMGTYPE_unref(pixbuf);
				}
			}
		}

		if (!isMulti) {
			int col = 0;	// RpListDataItem doesn't include the icon/checkbox column
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			for (const string &str : data_row) {
				// TODO: RpListDataItem timestamp mutator function?
				if (unlikely((is_timestamp & 1) && str.size() == sizeof(int64_t))) {
					// Timestamp column. Format the timestamp.
					RomFields::TimeString_t time_string;
					memcpy(time_string.str, str.data(), 8);

					gchar *const str = rom_data_format_datetime(
						time_string.time, listDataDesc.col_attrs.dtflags);
					rp_list_data_item_set_column_text(item, col,
						(likely(str != nullptr) ? str : C_("RomData", "Unknown")));
					g_free(str);
				} else {
					rp_list_data_item_set_column_text(item, col, str.c_str());
				}

				is_timestamp >>= 1;
				col++;
			}
		}

		g_list_store_append(listStore, item);
		row++;
	}

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new();
	// NOTE: No name for this GtkWidget.
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *const scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
	// NOTE: No name for this GtkWidget.
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
	gtk_widget_show(scrolledWindow);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

#if 0
	// Sort proxy model for the GtkListStore.
	GtkTreeModel *sortProxy = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(listStore));
#endif

	// Add the GtkColumnView to the scrolled window.
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), columnView);

	// TODO: Set fixed height mode?
	// May require fixed columns...
	// Reference: https://developer.gnome.org/gtk3/stable/GtkTreeView.html#gtk-tree-view-set-fixed-height-mode

#if 0
	// Set the default sorting column.
	// NOTE: sort_dir maps directly to GtkSortType.
	if (col_attrs.sort_col >= 0) {
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortProxy),
			col_attrs.sort_col + listStore_col_start,
			static_cast<GtkSortType>(col_attrs.sort_dir));
	}
#endif

	// Set a minimum height for the scroll area.
	// TODO: Adjust for DPI, and/or use a font size?
	// TODO: Force maximum horizontal width somehow?
	gtk_widget_set_size_request(scrolledWindow, -1, 128);

#if 0
	if (!isMulti) {
		// Resize the columns to fit the contents.
		gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeView));
	}

	// Row height is recalculated when the window is first visible
	// and/or the system theme is changed.
	// TODO: Set an actual default number of rows, or let GTK+ handle it?
	// (Windows uses 5.)
	g_object_set_qdata(G_OBJECT(treeView), RFT_LISTDATA_rows_visible_quark, GINT_TO_POINTER(listDataDesc.rows_visible));
	if (listDataDesc.rows_visible > 0) {
		g_signal_connect(treeView, "realize", G_CALLBACK(tree_view_realize_signal_handler), page);
	}
#endif

	if (isMulti) {
		page->cxx->vecListDataMulti.emplace_back(listStore, GTK_COLUMN_VIEW(columnView), &field);
	}

	return scrolledWindow;
}

/**
 * Update RFT_LISTDATA_MULTI fields.
 * Called from rp_rom_data_view_update_multi.
 * @param page		[in] RomDataView object.
 * @param user_lc	[in] User-specified language code.
 * @param set_lc	[in/out] Set of LCs
 */
void
rp_rom_data_view_update_multi_RFT_LISTDATA_MULTI(RpRomDataView *page, uint32_t user_lc, set<uint32_t> &set_lc)
{
	_RpRomDataViewCxx *const cxx = page->cxx;

	// RFT_LISTDATA_MULTI
	for (const Data_ListDataMulti_t &vldm : cxx->vecListDataMulti) {
		GListStore *const listStore = vldm.listStore;
		const RomFields::Field *const pField = vldm.field;
		const auto *const pListData_multi = pField->data.list_data.data.multi;
		assert(pListData_multi != nullptr);
		assert(!pListData_multi->empty());
		if (!pListData_multi || pListData_multi->empty()) {
			// Invalid RFT_LISTDATA_MULTI...
			continue;
		}

		if (!page->cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			for (const auto &pldm : *pListData_multi) {
				set_lc.emplace(pldm.first);
			}
		}

		// Get the ListData_t.
		const auto *const pListData = RomFields::getFromListDataMulti(pListData_multi, cxx->def_lc, user_lc);
		assert(pListData != nullptr);
		if (pListData != nullptr) {
			const auto &listDataDesc = pField->desc.list_data;

			// Update the list.
			auto iter_listData = pListData->cbegin();
			const auto pListData_cend = pListData->cend();
			const int n_items = g_list_model_get_n_items(G_LIST_MODEL(listStore));
			for (int i = 0; i < n_items && iter_listData != pListData_cend; i++, ++iter_listData) {
				RpListDataItem *const item = RP_LIST_DATA_ITEM(g_list_model_get_item(G_LIST_MODEL(listStore), i));
				assert(item != nullptr);

				int col = 0;
				unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
				for (const string &str : *iter_listData) {
					if (unlikely((is_timestamp & 1) && str.size() == sizeof(int64_t))) {
						// Timestamp column. Format the timestamp.
						RomFields::TimeString_t time_string;
						memcpy(time_string.str, str.data(), 8);

						gchar *const str = rom_data_format_datetime(
							time_string.time, listDataDesc.col_attrs.dtflags);
						rp_list_data_item_set_column_text(item, col,
							(likely(str != nullptr) ? str : C_("RomData", "Unknown")));
						g_free(str);
					} else {
						rp_list_data_item_set_column_text(item, col, str.c_str());
					}

					// Next column
					is_timestamp >>= 1;
					col++;
				}
			}

			// NOTE: ListDataItem doesn't emit any signals if the text is changed.
			// As a workaround, remove the GtkColumnView's model, then re-add it.
			GtkSelectionModel *const selModel = gtk_column_view_get_model(GTK_COLUMN_VIEW(vldm.columnView));
			g_object_ref(selModel);
			gtk_column_view_set_model(GTK_COLUMN_VIEW(vldm.columnView), NULL);
			gtk_column_view_set_model(GTK_COLUMN_VIEW(vldm.columnView), GTK_SELECTION_MODEL(selModel));
			g_object_unref(selModel);

#if 0
			// Resize the columns to fit the contents.
			// NOTE: Only done on first load.
			if (!page->cboLanguage) {
				gtk_tree_view_columns_autosize(GTK_TREE_VIEW(vldm.treeView));
			}
#endif
		}
	}
}

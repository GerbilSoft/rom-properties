/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView_gtk3.cpp: RomData viewer widget. (GTK2/GTK3-specific)       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RomDataFormat.hpp"

#include "gtk3/sort_funcs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpTexture;

// C++ STL classes
using std::set;
using std::string;
using std::vector;

static void	tree_view_realize_signal_handler    (GtkTreeView	*treeView,
						     RpRomDataView	*page);

/**
 * GtkTreeView widget has been realized.
 * @param treeView GtkTreeView
 * @param page RomDataView
 */
static void
tree_view_realize_signal_handler(GtkTreeView	*treeView,
				 RpRomDataView	*page)
{
	// TODO: Redo this if the system font and/or style changes.
	// TODO: Remove the `page` parameter?
	RP_UNUSED(page);

	// Recalculate the row heights for this GtkTreeView.
	const int rows_visible = GPOINTER_TO_INT(
		g_object_get_qdata(G_OBJECT(treeView), RFT_LISTDATA_rows_visible_quark));
	if (rows_visible <= 0) {
		// This GtkTreeView doesn't have a fixed number of rows.
		return;
	}

	// Get the parent widget.
	// This should be a GtkScrolledWindow.
	GtkWidget *const scrolledWindow = gtk_widget_get_ancestor(
		GTK_WIDGET(treeView), GTK_TYPE_SCROLLED_WINDOW);
	if (!scrolledWindow || !GTK_IS_SCROLLED_WINDOW(scrolledWindow)) {
		// No parent widget, or not a GtkScrolledWindow.
		return;
	}

	// Get the height of the first item.
	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GdkRectangle rect;
	gtk_tree_view_get_background_area(GTK_TREE_VIEW(treeView), path, nullptr, &rect);
	gtk_tree_path_free(path);
	if (rect.height <= 0) {
		// GtkListStore probably doesn't have any items.
		return;
	}
	int height = rect.height * rows_visible;

	if (gtk_tree_view_get_headers_visible(treeView)) {
		// Get the header widget of the first column.
		GtkTreeViewColumn *const column = gtk_tree_view_get_column(treeView, 0);
		assert(column != nullptr);
		if (!column) {
			// No columns...
			return;
		}

		GtkWidget *header = gtk_tree_view_column_get_widget(column);
		if (!header) {
			header = gtk_tree_view_column_get_button(column);
		}
		if (header) {
			// TODO: gtk_widget_get_allocated_height() for GTK+ 3.x?
			GtkAllocation allocation;
			gtk_widget_get_allocation(header, &allocation);
			height += allocation.height;
		}
	}

#if GTK_CHECK_VERSION(3,0,0)
	// Get the GtkScrolledWindow's border, padding, and margin.
	GtkStyleContext *context = gtk_widget_get_style_context(scrolledWindow);
	GtkBorder border, padding, margin;
#  if GTK_CHECK_VERSION(4,0,0)
	gtk_style_context_get_border(context, &border);
	gtk_style_context_get_padding(context, &padding);
	gtk_style_context_get_margin(context, &margin);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_style_context_get_border(context, GTK_STATE_FLAG_NORMAL, &border);
	gtk_style_context_get_padding(context, GTK_STATE_FLAG_NORMAL, &padding);
	gtk_style_context_get_margin(context, GTK_STATE_FLAG_NORMAL, &margin);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
	height += border.top + border.bottom;
	height += padding.top + padding.bottom;
	height += margin.top + margin.bottom;
#else /* !GTK_CHECK_VERSION(3,0,0) */
	// Get the GtkScrolledWindow's border.
	// NOTE: Assuming we have a border set.
	GtkStyle *style = gtk_widget_get_style(scrolledWindow);
	height += (style->ythickness * 2);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Set the GtkScrolledWindow's height.
	// NOTE: gtk_scrolled_window_set_max_content_height() doesn't seem to
	// work properly for rows_visible=4, and it's GTK+ 3.x only.
	gtk_widget_set_size_request(scrolledWindow, -1, height);
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

	GtkListStore *listStore;
	int listStore_col_start;
	if (hasCheckboxes || hasIcons) {
		// Prepend an extra column for checkboxes or icons.
		GType *const types = new GType[colCount+1];
		types[0] = (hasCheckboxes ? G_TYPE_BOOLEAN : PIMGTYPE_GOBJECT_TYPE);
		std::fill(&types[1], &types[colCount+1], G_TYPE_STRING);
		listStore = gtk_list_store_newv(colCount+1, types);
		delete[] types;
		listStore_col_start = 1;	// Skip the checkbox column for strings.
	} else {
		// All strings.
		GType *const types = new GType[colCount];
		std::fill(types, &types[colCount], G_TYPE_STRING);
		listStore = gtk_list_store_newv(colCount, types);
		delete[] types;
		listStore_col_start = 0;
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

		GtkTreeIter treeIter;
		gtk_list_store_append(listStore, &treeIter);
		if (hasCheckboxes) {
			// Checkbox column
			gtk_list_store_set(listStore, &treeIter, 0, (checkboxes & 1), -1);
			checkboxes >>= 1;
		} else if (hasIcons) {
			// Icon column
			const rp_image_const_ptr &icon = field.data.list_data.mxd.icons->at(row);
			if (icon) {
				PIMGTYPE pixbuf = rp_image_to_PIMGTYPE(icon);
				if (pixbuf) {
					// TODO: Ideal icon size?
					// Using 32x32 for now.
					static const int icon_sz = 32;
					// NOTE: GtkCellRendererPixbuf can't scale the
					// pixbuf itself...
					if (!PIMGTYPE_size_check(pixbuf, icon_sz, icon_sz)) {
						// TODO: Use nearest-neighbor if upscaling.
						// Also, preserve the aspect ratio.
						PIMGTYPE scaled = PIMGTYPE_scale(pixbuf, icon_sz, icon_sz, true);
						if (scaled) {
							PIMGTYPE_unref(pixbuf);
							pixbuf = scaled;
						}
					}
					gtk_list_store_set(listStore, &treeIter, 0, pixbuf, -1);
					PIMGTYPE_unref(pixbuf);
				}
			}
		}

		if (!isMulti) {
			int col = listStore_col_start;
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			for (const string &str : data_row) {
				if (unlikely((is_timestamp & 1) && str.size() == sizeof(int64_t))) {
					// Timestamp column. Format the timestamp.
					RomFields::TimeString_t time_string;
					memcpy(time_string.str, str.data(), 8);

					gchar *const str = rom_data_format_datetime(
						time_string.time, listDataDesc.col_attrs.dtflags);
					gtk_list_store_set(listStore, &treeIter, col,
						(likely(str != nullptr) ? str : C_("RomData", "Unknown")), -1);
					g_free(str);
				} else {
					gtk_list_store_set(listStore, &treeIter, col, str.c_str(), -1);
				}

				is_timestamp >>= 1;
				col++;
			}
		}

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

	// Sort proxy model for the GtkListStore.
	GtkTreeModel *sortProxy = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(listStore));

	// Create the GtkTreeView.
	GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sortProxy));
	// NOTE: No name for this GtkWidget.
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView),
		(listDataDesc.names != nullptr));
#if !GTK_CHECK_VERSION(4,0,0)
	gtk_widget_show(treeView);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), treeView);

	// TODO: Set fixed height mode?
	// May require fixed columns...
	// Reference: https://developer.gnome.org/gtk3/stable/GtkTreeView.html#gtk-tree-view-set-fixed-height-mode

#if !GTK_CHECK_VERSION(3,0,0)
	// GTK+ 2.x: Use the "rules hint" for alternating row colors.
	// Deprecated in GTK+ 3.14 (and removed in GTK4), but it doesn't
	// work with GTK+ 3.x anyway.
	// TODO: GTK4's GtkListView might have a similar function.
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeView), true);
#endif /* !GTK_CHECK_VERSION(3,0,0) */

	// Extra GtkCellRenderer for icon and/or checkbox.
	// This is prepended to column 0.
	GtkCellRenderer *col0_renderer = nullptr;
	const char *col0_attr_name = nullptr;
	if (hasCheckboxes) {
		col0_renderer = gtk_cell_renderer_toggle_new();
		col0_attr_name = "active";
	} else if (hasIcons) {
		col0_renderer = gtk_cell_renderer_pixbuf_new();
		col0_attr_name = GTK_CELL_RENDERER_PIXBUF_PROPERTY;
	}

	// Format tables.
	// Pango enum values are known to fit in uint8_t.
	static const gfloat align_tbl_xalign[4] = {
		// Order: TXA_D, TXA_L, TXA_C, TXA_R
		0.0f, 0.0f, 0.5f, 1.0f
	};
	static const uint8_t align_tbl_pango[4] = {
		// Order: TXA_D, TXA_L, TXA_C, TXA_R
		PANGO_ALIGN_LEFT, PANGO_ALIGN_LEFT,
		PANGO_ALIGN_CENTER, PANGO_ALIGN_RIGHT
	};

	// Set up the columns.
	RomFields::ListDataColAttrs_t col_attrs = listDataDesc.col_attrs;
	for (int i = 0; i < colCount; i++, col_attrs.shiftRight()) {
		const int listStore_col_idx = i + listStore_col_start;

		// NOTE: Not skipping empty column names.
		// TODO: Hide them.
		GtkTreeViewColumn *const column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column,
			(listDataDesc.names ? listDataDesc.names->at(i).c_str() : ""));

		if (col0_renderer != nullptr) {
			// Prepend the icon/checkbox renderer.
			gtk_tree_view_column_pack_start(column, col0_renderer, FALSE);
			gtk_tree_view_column_add_attribute(column, col0_renderer, col0_attr_name, 0);
			col0_renderer = nullptr;
		}

		GtkCellRenderer *const renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", listStore_col_idx);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

		// Header/data alignment
		g_object_set(column,
			"alignment", align_tbl_xalign[col_attrs.align_headers & RomFields::TXA_MASK],
			nullptr);
		g_object_set(renderer,
			"xalign", align_tbl_xalign[col_attrs.align_data & RomFields::TXA_MASK],
			"alignment", static_cast<PangoAlignment>(align_tbl_pango[col_attrs.align_data & RomFields::TXA_MASK]),
			nullptr);

		// Column sizing
		// NOTE: We don't have direct equivalents to QHeaderView::ResizeMode.
		switch (col_attrs.sizing & RomFields::COLSZ_MASK) {
			case RomFields::ColSizing::COLSZ_INTERACTIVE:
				gtk_tree_view_column_set_resizable(column, true);
				gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
				break;
			/*case RomFields::ColSizing::COLSZ_FIXED:
				gtk_tree_view_column_set_resizable(column, true);
				gtk_tree_view_column_set_sizing(column, FIXED);
				break;*/
			case RomFields::ColSizing::COLSZ_STRETCH:
				// TODO: Wordwrapping and/or text elision?
				// NOTE: Allowing the user to resize the column because
				// unlike Qt, we can't shrink it by shrinking the window.
				gtk_tree_view_column_set_resizable(column, true);
				gtk_tree_view_column_set_expand(column, true);
				gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
				break;
			case RomFields::ColSizing::COLSZ_RESIZETOCONTENTS:
				gtk_tree_view_column_set_resizable(column, false);
				gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
				break;
		}

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
	}

	assert(col0_renderer == nullptr);
	if (col0_renderer) {
		// This should've been assigned to a GtkTreeViewColumn...
		g_object_unref(col0_renderer);
	}

	// Set the default sorting column.
	// NOTE: sort_dir maps directly to GtkSortType.
	if (col_attrs.sort_col >= 0) {
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortProxy),
			col_attrs.sort_col + listStore_col_start,
			static_cast<GtkSortType>(col_attrs.sort_dir));
	}

	// Set a minimum height for the scroll area.
	// TODO: Adjust for DPI, and/or use a font size?
	// TODO: Force maximum horizontal width somehow?
	gtk_widget_set_size_request(scrolledWindow, -1, 128);

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

	if (isMulti) {
		page->cxx->vecListDataMulti.emplace_back(listStore, GTK_TREE_VIEW(treeView), &field);
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
		GtkListStore *const listStore = vldm.listStore;
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
			// If we have checkboxes or icons, start at column 1.
			// Otherwise, start at column 0.
			int listStore_col_start;
			if (pField->flags & (RomFields::RFT_LISTDATA_CHECKBOXES | RomFields::RFT_LISTDATA_ICONS)) {
				// Checkboxes and/or icons are present.
				listStore_col_start = 1;
			} else {
				listStore_col_start = 0;
			}

			const auto &listDataDesc = pField->desc.list_data;

			// Update the list.
			GtkTreeIter treeIter;
			GtkTreeModel *const treeModel = GTK_TREE_MODEL(listStore);
			gboolean ok = gtk_tree_model_get_iter_first(treeModel, &treeIter);
			auto iter_listData = pListData->cbegin();
			const auto pListData_cend = pListData->cend();
			while (ok && iter_listData != pListData_cend) {
				// TODO: Verify GtkListStore column count?
				int col = listStore_col_start;
				unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
				for (const string &str : *iter_listData) {
					if (unlikely((is_timestamp & 1) && str.size() == sizeof(int64_t))) {
						// Timestamp column. Format the timestamp.
						RomFields::TimeString_t time_string;
						memcpy(time_string.str, str.data(), 8);

						gchar *const str = rom_data_format_datetime(
							time_string.time, listDataDesc.col_attrs.dtflags);
						gtk_list_store_set(listStore, &treeIter, col,
							(likely(str != nullptr) ? str : C_("RomData", "Unknown")), -1);
						g_free(str);
					} else {
						gtk_list_store_set(listStore, &treeIter, col, str.c_str(), -1);
					}

					// Next column
					is_timestamp >>= 1;
					col++;
				}

				// Next row
				++iter_listData;
				ok = gtk_tree_model_iter_next(treeModel, &treeIter);
			}

			// Resize the columns to fit the contents.
			// NOTE: Only done on first load.
			if (!page->cboLanguage) {
				gtk_tree_view_columns_autosize(GTK_TREE_VIEW(vldm.treeView));
			}
		}
	}
}

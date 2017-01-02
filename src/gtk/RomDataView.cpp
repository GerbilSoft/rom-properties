/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomData-view.cpp: RomData viewer widget.                                *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "GdkImageConv.hpp"

#include "libromdata/common.h"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/IconAnimData.hpp"
#include "libromdata/img/IconAnimHelper.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// References:
// - audio-tags plugin
// - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html

/* Property identifiers */
enum {
	PROP_0,
	PROP_FILENAME,
};

static void	rom_data_view_finalize		(GObject	*object);
static void	rom_data_view_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rom_data_view_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rom_data_view_filename_changed	(const gchar 	*filename,
						 RomDataView	*page);

static void	rom_data_view_init_header_row	(RomDataView	*page);
static void	rom_data_view_update_display	(RomDataView	*page);
static gboolean	rom_data_view_load_rom_data	(gpointer	 data);

/** Signal handlers. **/
static void	checkbox_no_toggle_signal_handler(GtkToggleButton	*togglebutton,
						  gpointer		 user_data);

/** Icon animation timer. **/
static void	start_anim_timer(RomDataView *page);
static void	stop_anim_timer (RomDataView *page);
static gboolean	anim_timer_func (RomDataView *page);

// XFCE property page class.
struct _RomDataViewClass {
#if GTK_CHECK_VERSION(3,0,0)
	GtkBoxClass __parent__;
#else
	GtkVBoxClass __parent__;
#endif
};

// XFCE property page.
struct _RomDataView {
#if GTK_CHECK_VERSION(3,0,0)
	GtkBox __parent__;
#else
	GtkVBox __parent__;
#endif

	/* Widgets */
	GtkWidget	*table;
	GtkWidget	*lblCredits;

	/* Timeouts */
	guint		changed_idle;

	// Header row.
	GtkWidget	*hboxHeaderRow;
	GtkWidget	*lblSysInfo;
	GtkWidget	*imgIcon;
	GtkWidget	*imgBanner;
	// TODO: Icon and banner.

	// Filename.
	string		filename;

	// ROM data.
	RomData		*romData;

	// Animated icon data.
	const IconAnimData *iconAnimData;
	// TODO: GdkPixmap or cairo_surface_t?
	GdkPixbuf	*iconFrames[IconAnimData::MAX_FRAMES];
	IconAnimHelper	iconAnimHelper;
	int		last_frame_number;	// Last frame number.

	// Icon animation timer.
	guint		tmrIconAnim;
	int		last_delay;		// Last delay value.
};

// FIXME: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
#if GTK_CHECK_VERSION(3,0,0)
//G_DEFINE_TYPE(RomDataView, rom_data_view, GTK_TYPE_BOX);
G_DEFINE_TYPE_EXTENDED(RomDataView, rom_data_view,
	GTK_TYPE_BOX, static_cast<GTypeFlags>(0), {});
#else
//G_DEFINE_TYPE(RomDataView, rom_data_view, GTK_TYPE_VBOX);
G_DEFINE_TYPE_EXTENDED(RomDataView, rom_data_view,
	GTK_TYPE_VBOX, static_cast<GTypeFlags>(0), {});
#endif

static inline void make_label_bold(GtkLabel *label)
{
	PangoAttrList *attr_lst = pango_attr_list_new();
	PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
	pango_attr_list_insert(attr_lst, attr);
	gtk_label_set_attributes(label, attr_lst);
	pango_attr_list_unref(attr_lst);
}

static void
rom_data_view_class_init(RomDataViewClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = rom_data_view_finalize;
	gobject_class->get_property = rom_data_view_get_property;
	gobject_class->set_property = rom_data_view_set_property;

	/**
	 * RomDataView:filename:
	 *
	 * The filename modified on this page.
	 **/
	g_object_class_install_property(gobject_class, PROP_FILENAME,
		g_param_spec_string("filename", "filename", "filename",
			"", G_PARAM_READWRITE));
}

static void
rom_data_view_init(RomDataView *page)
{
	// No ROM data initially.
	page->romData = nullptr;
	page->table = nullptr;
	page->lblCredits = nullptr;
	page->last_frame_number = 0;

	// Animation timer.
	page->tmrIconAnim = 0;
	page->last_delay = 0;

	// Base class is GtkVBox.

#if GTK_CHECK_VERSION(3,0,0)
	// Header row.
	page->hboxHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	// TODO: Needs testing.
	gtk_widget_set_halign(page->hboxHeaderRow, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow, FALSE, FALSE, 0);
	gtk_widget_show(page->hboxHeaderRow);
#else
	// Center-align the header row.
	GtkWidget *centerAlign = gtk_alignment_new(0.5f, 0.0f, 0.0f, 0.0f);
	gtk_box_pack_start(GTK_BOX(page), centerAlign, FALSE, FALSE, 0);
	gtk_widget_show(centerAlign);

	// Header row.
	page->hboxHeaderRow = gtk_hbox_new(FALSE, 8);
	gtk_container_add(GTK_CONTAINER(centerAlign), page->hboxHeaderRow);
	gtk_widget_show(page->hboxHeaderRow);
#endif

	// System information.
	page->lblSysInfo = gtk_label_new(nullptr);
	gtk_label_set_justify(GTK_LABEL(page->lblSysInfo), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->lblSysInfo, FALSE, FALSE, 0);
	gtk_widget_show(page->lblSysInfo);

	// Banner.
	page->imgBanner = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgBanner, FALSE, FALSE, 0);

	// Icon.
	page->imgIcon = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgIcon, FALSE, FALSE, 0);

	// Make lblSysInfo bold.
	make_label_bold(GTK_LABEL(page->lblSysInfo));

	// Table layout is created in rom_data_view_update_display().
}

static void
rom_data_view_finalize(GObject *object)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	/* Unregister the changed_idle */
	if (G_UNLIKELY(page->changed_idle != 0)) {
		g_source_remove(page->changed_idle);
	}

	// Delete the timer.
	if (page->tmrIconAnim > 0) {
		// TODO: Make sure there's no race conditions...
		g_source_remove(page->tmrIconAnim);
	}

	// Clear some widget variables to ensure that
	// rom_data_view_set_filename() doesn't try to do stuff.
	page->hboxHeaderRow = nullptr;
	page->table = nullptr;
	page->lblCredits = nullptr;

	// Free the file reference.
	// This also deletes romData and iconFrames.
	rom_data_view_set_filename(page, nullptr);

	(*G_OBJECT_CLASS(rom_data_view_parent_class)->finalize)(object);
}

GtkWidget*
rom_data_view_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_ROM_DATA_VIEW, nullptr));
}

static void
rom_data_view_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	switch (prop_id) {
		case PROP_FILENAME:
			g_value_set_string(value, rom_data_view_get_filename(page));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rom_data_view_set_property(GObject	*object,
			   guint	 prop_id,
			   const GValue	*value,
			   GParamSpec	*pspec)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	switch (prop_id) {
		case PROP_FILENAME:
			rom_data_view_set_filename(page, g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * rom_data_view_get_filename:
 * @page : a #RomDataView.
 *
 * Returns the current filename for the @page.
 *
 * Return value: the file associated with this property page.
 **/
const gchar*
rom_data_view_get_filename(RomDataView *page)
{
	g_return_val_if_fail(IS_ROM_DATA_VIEW(page), nullptr);
	return page->filename.c_str();
}

/**
 * rom_data_view_set_filename:
 * @page : a #RomDataView.
 * @file : a #ThunarxFileInfo
 *
 * Sets the filename for this @page.
 **/
void
rom_data_view_set_filename(RomDataView	*page,
			   const gchar	*file)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));

	/* Check if we already use this file */
	if (G_UNLIKELY(page->filename.empty() && !file))
		return;
	if (G_UNLIKELY(file != nullptr && page->filename == file))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(!page->filename.empty()))
	{
		page->filename.clear();

		// NULL out iconAnimData.
		// (This is owned by the RomData object.)
		page->iconAnimData = nullptr;

		// Delete the existing RomData object.
		delete page->romData;
		page->romData = nullptr;

		// Delete the icon frames.
		for (int i = ARRAY_SIZE(page->iconFrames)-1; i >= 0; i--) {
			if (page->iconFrames[i]) {
				g_object_unref(page->iconFrames[i]);
				page->iconFrames[i] = nullptr;
			}
		}
	}

	/* Assign the value */
	if (!file) {
		page->filename.clear();
	} else {
		page->filename = file;
	}

	/* Connect to the new file (if any) */
	if (G_LIKELY(!page->filename.empty())) {
		rom_data_view_filename_changed(page->filename.c_str(), page);
	} else {
		// Hide the header row and delete the table.
		if (page->hboxHeaderRow) {
			gtk_widget_hide(page->hboxHeaderRow);
		}
		if (page->table) {
			gtk_widget_destroy(page->table);
			page->table = nullptr;
		}
		if (page->lblCredits) {
			gtk_widget_destroy(page->lblCredits);
			page->lblCredits = nullptr;
		}
	}
}

static void
rom_data_view_filename_changed(const gchar	*file,
			       RomDataView	*page)
{
	g_return_if_fail(file != nullptr);
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(page->filename == file);

	if (page->changed_idle == 0) {
		page->changed_idle = g_idle_add(rom_data_view_load_rom_data, page);
	}
}

static void
rom_data_view_init_header_row(RomDataView *page)
{
	// Initialize the header row.
	// TODO: Icon, banner.
	assert(page != nullptr);

	// NOTE: romData might be nullptr in some cases.
	//assert(page->romData != nullptr);
	if (!page->romData) {
		// No ROM data.
		// Hide the widgets.
		gtk_widget_hide(page->hboxHeaderRow);
		return;
	}

	// System name and file type.
	const rp_char *const systemName = page->romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const rp_char *const fileType = page->romData->fileType_string();

	string sysInfo;
	sysInfo.reserve(128);
	if (systemName) {
		sysInfo = systemName;
	}
	if (fileType) {
		if (!sysInfo.empty()) {
			sysInfo += '\n';
		}
		sysInfo += fileType;
	}

	gtk_label_set_text(GTK_LABEL(page->lblSysInfo), sysInfo.c_str());

	// Supported image types.
	const uint32_t imgbf = page->romData->supportedImageTypes();

	// Banner.
	gtk_widget_hide(page->imgBanner);
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image *banner = page->romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(banner);
			if (pixbuf) {
				gtk_image_set_from_pixbuf(GTK_IMAGE(page->imgBanner), pixbuf);
				g_object_unref(pixbuf);
				gtk_widget_show(page->imgBanner);
			}
		}
	}

	// Icon.
	gtk_widget_hide(page->imgIcon);
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image *icon = page->romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(icon);
			if (pixbuf) {
				gtk_image_set_from_pixbuf(GTK_IMAGE(page->imgIcon), pixbuf);
				page->iconFrames[0] = pixbuf;
				gtk_widget_show(page->imgIcon);
			}

			// Get the animated icon data.
			// TODO: Skip if the first frame is nullptr?
			page->iconAnimData = page->romData->iconAnimData();
			if (page->iconAnimData) {
				// Convert the icons to QPixmaps.
				for (int i = 1; i < page->iconAnimData->count; i++) {
					const rp_image *const frame = page->iconAnimData->frames[i];
					if (frame && frame->isValid()) {
						GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(frame);
						if (pixbuf) {
							page->iconFrames[i] = pixbuf;
						}
					}
				}

				// Set up the IconAnimHelper.
				page->iconAnimHelper.setIconAnimData(page->iconAnimData);
				// Icon animation timer is set in startAnimTimer().
			}
		}
	}

	// Show the header row.
	gtk_widget_show(page->hboxHeaderRow);
}

static void
rom_data_view_update_display(RomDataView *page)
{
	assert(page != nullptr);

	// Initialize the header row.
	rom_data_view_init_header_row(page);

	// Delete the table if it's already present.
	if (page->table) {
		gtk_widget_destroy(page->table);
		page->table = nullptr;
	}

	// Delete the credits label if it's already present.
	if (page->lblCredits) {
		gtk_widget_destroy(page->lblCredits);
		page->lblCredits = nullptr;
	}

	if (!page->romData) {
		// No ROM data...
		return;
	}

	// Get the fields.
	const RomFields *fields = page->romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Create the table.
	page->table = gtk_table_new(count, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(page->table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(page->table), 8);
	gtk_container_set_border_width(GTK_CONTAINER(page->table), 8);
	gtk_box_pack_start(GTK_BOX(page), page->table, FALSE, FALSE, 0);
	gtk_widget_show(page->table);

	// Create the data widgets.
	// TODO: Types other than RFT_STRING.
#ifndef NDEBUG
	bool hasStrfCredits = false;
#endif
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		// TODO: Localization.
		std::string gtkdesc = desc->name;
		gtkdesc += ':';

		GtkWidget *lblDesc = gtk_label_new(gtkdesc.c_str());
		gtk_label_set_use_underline(GTK_LABEL(lblDesc), false);
		gtk_label_set_justify(GTK_LABEL(lblDesc), GTK_JUSTIFY_RIGHT);
		make_label_bold(GTK_LABEL(lblDesc));
		gtk_table_attach(GTK_TABLE(page->table), lblDesc, 0, 1, i, i+1,
			GTK_FILL, GTK_FILL, 0, 0);
		gtk_misc_set_alignment(GTK_MISC(lblDesc), 1.0f, 0.0f);
		gtk_widget_show(lblDesc);

		switch (desc->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				gtk_widget_destroy(lblDesc);
				break;

			case RomFields::RFT_STRING: {
				// String type.
				GtkWidget *lblString = gtk_label_new(nullptr);
				gtk_label_set_use_underline(GTK_LABEL(lblString), false);
				gtk_widget_show(lblString);

				if (desc->str_desc && (desc->str_desc->formatting & RomFields::StringDesc::STRF_CREDITS)) {
					// Credits text. Enable formatting and center alignment.
					gtk_label_set_justify(GTK_LABEL(lblString), GTK_JUSTIFY_CENTER);
					gtk_misc_set_alignment(GTK_MISC(lblString), 0.5f, 0.0f);
					if (data->str) {
						// NOTE: Pango markup does not support <br/>.
						// It uses standard newlines for line breaks.
						gtk_label_set_markup(GTK_LABEL(lblString), data->str);
					}
				} else {
					// Standard text with no formatting.
					gtk_label_set_selectable(GTK_LABEL(lblString), TRUE);
					gtk_label_set_justify(GTK_LABEL(lblString), GTK_JUSTIFY_LEFT);
					gtk_misc_set_alignment(GTK_MISC(lblString), 0.0f, 0.0f);
					gtk_label_set_text(GTK_LABEL(lblString), data->str);
				}

				// Check for any formatting options.
				if (desc->str_desc) {
					PangoAttrList *attr_lst = pango_attr_list_new();

					// Monospace font?
					if (desc->str_desc->formatting & RomFields::StringDesc::STRF_MONOSPACE) {
						PangoAttribute *attr = pango_attr_family_new("monospace");
						pango_attr_list_insert(attr_lst, attr);
					}

					// "Warning" font?
					if (desc->str_desc->formatting & RomFields::StringDesc::STRF_WARNING) {
						PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
						pango_attr_list_insert(attr_lst, attr);
						attr = pango_attr_foreground_new(65535, 0, 0);
						pango_attr_list_insert(attr_lst, attr);
					}

					gtk_label_set_attributes(GTK_LABEL(lblString), attr_lst);
					pango_attr_list_unref(attr_lst);
				}

				if (desc->str_desc && (desc->str_desc->formatting & RomFields::StringDesc::STRF_CREDITS)) {
					// Credits row goes at the end.
					// There should be a maximum of one STRF_CREDITS per RomData subclass.
#ifndef NDEBUG
					assert(hasStrfCredits == false);
					hasStrfCredits = true;
#endif

					// Credits row.
					gtk_box_pack_end(GTK_BOX(page), lblString, FALSE, FALSE, 0);

					// No description field.
					gtk_widget_destroy(lblDesc);
				} else {
					// Standard string row.
					gtk_table_attach(GTK_TABLE(page->table), lblString, 1, 2, i, i+1,
						GTK_FILL, GTK_FILL, 0, 0);
				}

				break;
			}

			case RomFields::RFT_BITFIELD: {
				// Bitfield type. Create a grid of checkboxes.
				// TODO: Description label needs some padding on the top...
				const RomFields::BitfieldDesc *const bitfieldDesc = desc->bitfield;

				// Determine the total number of rows and columns.
				int totalRows, totalCols;
				if (bitfieldDesc->elemsPerRow == 0) {
					// All elements are in one row.
					totalCols = bitfieldDesc->elements;
					totalRows = 1;
				} else {
					// Multiple rows.
					totalCols = bitfieldDesc->elemsPerRow;
					totalRows = bitfieldDesc->elements / bitfieldDesc->elemsPerRow;
					if ((bitfieldDesc->elements % bitfieldDesc->elemsPerRow) > 0) {
						totalRows++;
					}
				}
				GtkWidget *gridBitfield = gtk_table_new(totalRows, totalCols, FALSE);
				//gtk_table_set_row_spacings(GTK_TABLE(gridBitfield), 2);
				//gtk_table_set_col_spacings(GTK_TABLE(gridBitfield), 8);
				gtk_widget_show(gridBitfield);

				int row = 0, col = 0;
				for (int bit = 0; bit < bitfieldDesc->elements; bit++) {
					const rp_char *name = bitfieldDesc->names[bit];
					if (!name)
						continue;

					GtkWidget *checkBox = gtk_check_button_new_with_label(name);
					gtk_widget_show(checkBox);
					if (data->bitfield & (1 << bit)) {
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkBox), TRUE);
					}

					// Disable user modifications.
					// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
					// emitted for both user and program modifications, so we have to
					// connect this signal *after* setting the initial value.
					g_signal_connect(checkBox, "toggled",
						reinterpret_cast<GCallback>(checkbox_no_toggle_signal_handler),
						nullptr);

					gtk_table_attach(GTK_TABLE(gridBitfield), checkBox, col, col+1, row, row+1,
						GTK_FILL, GTK_FILL, 0, 0);
					col++;
					if (col == bitfieldDesc->elemsPerRow) {
						row++;
						col = 0;
					}
				}

				gtk_table_attach(GTK_TABLE(page->table), gridBitfield, 1, 2, i, i+1,
					GTK_FILL, GTK_FILL, 0, 0);
				break;
			}

			case RomFields::RFT_LISTDATA: {
				// ListData type. Create a QTreeWidget.
				const RomFields::ListDataDesc *listDataDesc = desc->list_data;
				const int count = listDataDesc->count;
				GType *types = new GType[listDataDesc->count];
				for (int i = 0; i < count; i++) {
					types[i] = G_TYPE_STRING;
				}
				GtkListStore *listStore = gtk_list_store_newv(count, types);
				delete[] types;

				// Add the row data.
				const RomFields::ListData *listData = data->list_data;
				for (int i = 0; i < (int)listData->data.size(); i++) {
					const vector<rp_string> &data_row = listData->data.at(i);
					GtkTreeIter treeIter;
					gtk_list_store_insert_before(listStore, &treeIter, nullptr);
					int field = 0;
					for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, ++field) {
						gtk_list_store_set(listStore, &treeIter, field, iter->c_str(), -1);
					}
				}

				// Scroll area for the GtkTreeView.
				GtkWidget *scrollArea = gtk_scrolled_window_new(nullptr, nullptr);
				gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollArea),
						GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
				gtk_widget_show(scrollArea);

				// Create the GtkTreeView.
				GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
				gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), TRUE);
				gtk_widget_show(treeView);
				//treeWidget->setRootIsDecorated(false);
				//treeWidget->setUniformRowHeights(true);
				gtk_container_add(GTK_CONTAINER(scrollArea), treeView);

				// Set up the column names.
				for (int i = 0; i < count; i++) {
					if (listDataDesc->names[i]) {
						GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
						GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
							listDataDesc->names[i], renderer,
							"text", i, nullptr);
						gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
					}
				}

				// Set a minimum height for the scroll area.
				// TODO: Adjust for DPI, and/or use a font size?
				// TODO: Force maximum horizontal width somehow?
				gtk_widget_set_size_request(scrollArea, -1, 128);

				// Resize the columns to fit the contents.
				gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeView));
				gtk_table_attach(GTK_TABLE(page->table), scrollArea, 1, 2, i, i+1,
					GTK_FILL, GTK_FILL, 0, 0);
				break;
			}

			case RomFields::RFT_DATETIME: {
				// Date/Time.
				const RomFields::DateTimeDesc *const dateTimeDesc = desc->date_time;

				GtkWidget *lblDateTime = gtk_label_new(nullptr);
				gtk_label_set_use_underline(GTK_LABEL(lblDateTime), false);
				gtk_label_set_selectable(GTK_LABEL(lblDateTime), TRUE);
				gtk_label_set_justify(GTK_LABEL(lblDateTime), GTK_JUSTIFY_LEFT);
				gtk_misc_set_alignment(GTK_MISC(lblDateTime), 0.0f, 0.0f);
				gtk_widget_show(lblDateTime);

				GDateTime *dateTime;
				if (dateTimeDesc->flags & RomFields::RFT_DATETIME_IS_UTC) {
					dateTime = g_date_time_new_from_unix_utc(data->date_time);
				} else {
					dateTime = g_date_time_new_from_unix_local(data->date_time);
				}

				gchar *str = nullptr;
				switch (dateTimeDesc->flags &
					(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME))
				{
					case RomFields::RFT_DATETIME_HAS_DATE:
						// Date only.
						str = g_date_time_format(dateTime, "%x");
						break;

					case RomFields::RFT_DATETIME_HAS_TIME:
						// Time only.
						str = g_date_time_format(dateTime, "%X");
						break;

					case RomFields::RFT_DATETIME_HAS_DATE |
					     RomFields::RFT_DATETIME_HAS_TIME:
						// Date and time.
						str = g_date_time_format(dateTime, "%x %X");
						break;

					default:
						// Invalid combination.
						assert(!"Invalid Date/Time formatting.");
						break;
				}
				g_date_time_unref(dateTime);

				if (str) {
					gtk_label_set_text(GTK_LABEL(lblDateTime), str);
					gtk_table_attach(GTK_TABLE(page->table), lblDateTime, 1, 2, i, i+1,
						GTK_FILL, GTK_FILL, 0, 0);
					g_free(str);
				} else {
					// Invalid date/time.
					gtk_widget_destroy(lblDateTime);
					gtk_widget_destroy(lblDesc);
				}

				break;
			}

			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				gtk_widget_destroy(lblDesc);
				break;
		}
	}
}

static gboolean
rom_data_view_load_rom_data(gpointer data)
{
	RomDataView *page = ROM_DATA_VIEW(data);
	g_return_val_if_fail(page != nullptr || IS_ROM_DATA_VIEW(page), FALSE);
	g_return_val_if_fail(!page->filename.empty(), FALSE);

	// Open the ROM file.
	// TODO: gvfs support.
	if (G_LIKELY(!page->filename.empty())) {
		// Open the ROM file.
		RpFile *file = new RpFile(page->filename, RpFile::FM_OPEN_READ);
		if (file->isOpen()) {
			// Create the RomData object.
			page->romData = RomDataFactory::getInstance(file, false);

			// Update the display widgets.
			rom_data_view_update_display(page);
		}
		delete file;
	}

	// Start the animation timer.
	// TODO: Start/stop on window show/hide?
	start_anim_timer(page);

	// Clear the timeout.
	page->changed_idle = 0;
	return FALSE;
}

/** Signal handlers. **/

static void
checkbox_no_toggle_signal_handler(GtkToggleButton	*togglebutton,
				  gpointer		 user_data)
{
	((void)user_data);
	if (gtk_toggle_button_get_active(togglebutton)) {
		// Uncheck this box.
		gtk_toggle_button_set_active(togglebutton, FALSE);
	}
}

/** Icon animation timer. **/

/**
 * Start the animation timer.
 */
static void start_anim_timer(RomDataView *page)
{
	if (!page->iconAnimData || !page->iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	page->last_frame_number = page->iconAnimHelper.frameNumber();
	const int delay = page->iconAnimHelper.frameDelay();
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Stop the animation timer.
	stop_anim_timer(page);

	// Set a timer for the current frame.
	page->last_delay = delay;
	page->tmrIconAnim = g_timeout_add(delay,
		reinterpret_cast<GSourceFunc>(anim_timer_func), page);
}

/**
 * Stop the animation timer.
 */
static void stop_anim_timer(RomDataView *page)
{
	if (page->tmrIconAnim > 0) {
		g_source_remove(page->tmrIconAnim);
		page->tmrIconAnim = 0;
		page->last_delay = 0;
	}
}

/**
 * Animated icon timer.
 */
static gboolean anim_timer_func(RomDataView *page)
{
	// Next frame.
	int delay = 0;
	int frame = page->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		return FALSE;
	}

	if (frame != page->last_frame_number) {
		// New frame number.
		// Update the icon.
		gtk_image_set_from_pixbuf(GTK_IMAGE(page->imgIcon), page->iconFrames[frame]);
		page->last_frame_number = frame;
	}

	// Set a new timer and unset the current one.
	if (page->last_delay != delay) {
		page->last_delay = delay;
		page->tmrIconAnim = g_timeout_add(delay,
			reinterpret_cast<GSourceFunc>(anim_timer_func), page);
		return FALSE;
	}
	return TRUE;
}

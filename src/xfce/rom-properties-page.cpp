/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * rom-properties-page.cpp: ThunarX Properties Page.                       *
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

#include "rom-properties-page.hpp"

#include "libromdata/file/RpFile.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RpFile;
using LibRomData::RomData;
using LibRomData::RomDataFactory;
using LibRomData::RomFields;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::string;

// References:
// - audio-tags plugin
// - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html

/* Property identifiers */
enum {
	PROP_0,
	PROP_FILE,
};

static void	rom_properties_page_finalize(GObject *object);
static void	rom_properties_page_get_property        (GObject		*object,
							 guint			 prop_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_set_property	(GObject		*object,
							 guint			 prop_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	rom_properties_page_file_changed	(ThunarxFileInfo	*file,
							 RomPropertiesPage      *page);

static void	rom_properties_page_init_header_row	(RomPropertiesPage	*page);
static void	rom_properties_page_update_display	(RomPropertiesPage	*page);
static gboolean	rom_properties_page_load_rom_data	(gpointer		 data);

// XFCE property page class.
struct _RomPropertiesPageClass {
	ThunarxPropertyPageClass __parent__;
};

// XFCE property page.
struct _RomPropertiesPage {
	ThunarxPropertyPage __parent__;

	/* Widgets */
	GtkWidget	*vboxMain;
	GtkWidget	*table;
	GtkWidget	*lblCredits;

	/* Timeouts */
	guint		changed_idle;

	// Header row.
	GtkWidget	*lblSysInfo;
	// TODO: Icon and banner.

	/* Properties */
	ThunarxFileInfo	*file;

	// ROM data.
	RomData		*romData;
};

// FIXME: THUNARX_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
//THUNARX_DEFINE_TYPE(RomPropertiesPage, rom_properties_page, THUNARX_TYPE_PROPERTY_PAGE);
THUNARX_DEFINE_TYPE_EXTENDED(RomPropertiesPage, rom_properties_page,
	THUNARX_TYPE_PROPERTY_PAGE, static_cast<GTypeFlags>(0), {});

static inline void make_label_bold(GtkLabel *label)
{
	PangoAttrList *attr_lst = pango_attr_list_new();
	PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
	pango_attr_list_insert(attr_lst, attr);
	gtk_label_set_attributes(label, attr_lst);
	pango_attr_list_unref(attr_lst);
}

static void
rom_properties_page_class_init(RomPropertiesPageClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = rom_properties_page_finalize;
	gobject_class->get_property = rom_properties_page_get_property;
	gobject_class->set_property = rom_properties_page_set_property;

	/**
	 * RomPropertiesPage:file:
	 *
	 * The #ThunarxFileInfo modified on this page.
	 **/
	g_object_class_install_property(gobject_class, PROP_FILE,
		g_param_spec_object("file", "file", "file",
			THUNARX_TYPE_FILE_INFO, G_PARAM_READWRITE));
}

static void
rom_properties_page_init(RomPropertiesPage *page)
{
	// No ROM data initially.
	page->romData = nullptr;
	page->table = nullptr;
	page->lblCredits = nullptr;

	// NOTE: GTK+3 adds halign/valign properties.
	// For GTK+2, we have to use a VBox.
	page->vboxMain = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(page), page->vboxMain);
	gtk_widget_show(page->vboxMain);

	// Header row.
	// TODO: Update with RomData.
	GtkWidget *hboxHeaderRow = gtk_hbox_new(FALSE, 8);
	gtk_box_pack_start(GTK_BOX(page->vboxMain), hboxHeaderRow, FALSE, FALSE, 0);
	gtk_widget_show(hboxHeaderRow);

	// System information.
	page->lblSysInfo = gtk_label_new("System information\nwill go here.");
	gtk_label_set_justify(GTK_LABEL(page->lblSysInfo), GTK_JUSTIFY_CENTER);
	gtk_misc_set_alignment(GTK_MISC(page->lblSysInfo), 0.5f, 0.0f);
	gtk_box_pack_start(GTK_BOX(hboxHeaderRow), page->lblSysInfo, TRUE, TRUE, 0);
	gtk_widget_show(page->lblSysInfo);

	// Make lblSysInfo bold.
	make_label_bold(GTK_LABEL(page->lblSysInfo));

	// Table layout is created in rom_properties_page_update_display().
}

static void
rom_properties_page_finalize(GObject *object)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	/* Unregister the changed_idle */
	if (G_UNLIKELY(page->changed_idle != 0)) {
		g_source_remove(page->changed_idle);
	}

	// Delete the RomData object.
	delete page->romData;
	page->romData = nullptr;

	/* Free file reference */
	rom_properties_page_set_file(page, nullptr);

	(*G_OBJECT_CLASS(rom_properties_page_parent_class)->finalize)(object);
}

RomPropertiesPage*
rom_properties_page_new(void)
{
	RomPropertiesPage *page = static_cast<RomPropertiesPage*>(g_object_new(TYPE_ROM_PROPERTIES_PAGE, nullptr));
	thunarx_property_page_set_label(THUNARX_PROPERTY_PAGE(page), "ROM Properties");
	return page;
}

static void
rom_properties_page_get_property(GObject	*object,
				 guint		 prop_id,
				 GValue		*value,
				 GParamSpec	*pspec)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	switch (prop_id) {
		case PROP_FILE:
			g_value_set_object(value, rom_properties_page_get_file(page));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rom_properties_page_set_property(GObject	*object,
				 guint		 prop_id,
				 const GValue	*value,
				 GParamSpec	*pspec)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(object);

	switch (prop_id) {
		case PROP_FILE:
			rom_properties_page_set_file(page, static_cast<ThunarxFileInfo*>(g_value_get_object(value)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * rom_properties_page_get_file:
 * @page : a #RomPropertiesPage.
 *
 * Returns the current #ThunarxFileInfo
 * for the @page.
 *
 * Return value: the file associated with this property page.
 **/
ThunarxFileInfo*
rom_properties_page_get_file(RomPropertiesPage *page)
{
	g_return_val_if_fail(IS_ROM_PROPERTIES_PAGE(page), nullptr);
	return page->file;
}

/**
 * rom_properties_page_set_file:
 * @page : a #RomPropertiesPage.
 * @file : a #ThunarxFileInfo
 *
 * Sets the #ThunarxFileInfo for this @page.
 **/
void
rom_properties_page_set_file	(RomPropertiesPage	*page,
				 ThunarxFileInfo	*file)
{
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(file == nullptr || THUNARX_IS_FILE_INFO(file));

	/* Check if we already use this file */
	if (G_UNLIKELY(page->file == file))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(page->file != nullptr))
	{
		g_signal_handlers_disconnect_by_func(G_OBJECT(page->file),
			reinterpret_cast<gpointer>(rom_properties_page_file_changed), page);
		g_object_unref(G_OBJECT(page->file));

		// TODO: Delete data widgets.

		// Delete the existing RomData object.
		delete page->romData;
		page->romData = nullptr;
	}

	/* Assign the value */
	page->file = file;

	/* Connect to the new file (if any) */
	if (G_LIKELY(file != nullptr))
	{
		/* Take a reference on the info file */
		g_object_ref(G_OBJECT(page->file));

		rom_properties_page_file_changed(file, page);
		g_signal_connect(G_OBJECT(file), "changed", G_CALLBACK(rom_properties_page_file_changed), page);
	}
}

static void
rom_properties_page_file_changed(ThunarxFileInfo	*file,
				 RomPropertiesPage	*page)
{
	g_return_if_fail(THUNARX_IS_FILE_INFO(file));
	g_return_if_fail(IS_ROM_PROPERTIES_PAGE(page));
	g_return_if_fail(page->file == file);

	if (page->changed_idle == 0) {
		page->changed_idle = g_idle_add(rom_properties_page_load_rom_data, page);
	}
}

static void
rom_properties_page_init_header_row(RomPropertiesPage *page)
{
	// Initialize the header row.
	// TODO: Icon, banner.
	assert(page != nullptr);

	// NOTE: romData might be nullptr in some cases.
	//assert(page->romData != nullptr);
	if (!page->romData) {
		// No ROM data.
		// Hide the widgets.
		gtk_widget_hide(page->lblSysInfo);
		return;
	}

	// TODO: Create widgets.
	// For now, just showing system information.
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
}

static void
rom_properties_page_update_display(RomPropertiesPage *page)
{
	assert(page != nullptr);

	// Initialize the header row.
	rom_properties_page_init_header_row(page);

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
	gtk_box_pack_start(GTK_BOX(page->vboxMain), page->table, FALSE, FALSE, 0);
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
				// TODO: String formatting options.
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
					gtk_box_pack_end(GTK_BOX(page->vboxMain), lblString, FALSE, FALSE, 0);

					// No description field.
					gtk_widget_destroy(lblDesc);
				} else {
					// Standard string row.
					gtk_table_attach(GTK_TABLE(page->table), lblString, 1, 2, i, i+1,
						GTK_FILL, GTK_FILL, 0, 0);
				}

				break;
			}

			default:
				// TODO: Other types.
				// Unsupported right now.
				//assert(!"Unsupported RomFields::RomFieldsType.");
				gtk_widget_destroy(lblDesc);
				break;
		}
	}
}

static gboolean
rom_properties_page_load_rom_data(gpointer data)
{
	RomPropertiesPage *page = ROM_PROPERTIES_PAGE(data);
	g_return_val_if_fail(page != nullptr || IS_ROM_PROPERTIES_PAGE(page), FALSE);
	g_return_val_if_fail(page->file != nullptr || THUNARX_IS_FILE_INFO(page->file), FALSE);

	/* Determine filename */
	gchar *uri = thunarx_file_info_get_uri(page->file);
	gchar *filename = g_filename_from_uri(uri, nullptr, nullptr);
	g_free(uri);

	// Open the ROM file.
	// TODO: gvfs support.
	if (G_LIKELY(filename)) {
		// Open the ROM file.
		RpFile *file = new RpFile(filename, RpFile::FM_OPEN_READ);
		if (file->isOpen()) {
			// Create the RomData object.
			page->romData = RomDataFactory::getInstance(file, false);

			// Update the display widgets.
			rom_properties_page_update_display(page);
		}
		delete file;
	}
	g_free(filename);

	/* Reset timeout */
	page->changed_idle = 0;

	return FALSE;
}

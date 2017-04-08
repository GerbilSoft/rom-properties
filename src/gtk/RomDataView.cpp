/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget.                                 *
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
#include "libromdata/TextFuncs.hpp"
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
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using std::ostringstream;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

// References:
// - audio-tags plugin
// - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html

/* Property identifiers */
enum {
	PROP_0,
	PROP_FILENAME,
	PROP_DESC_FORMAT_TYPE,
	PROP_LAST
};

static void	rom_data_view_dispose		(GObject	*object);
static void	rom_data_view_finalize		(GObject	*object);
static void	rom_data_view_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rom_data_view_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
// TODO: Make 'page' the first argument?
static void	rom_data_view_filename_changed	(const gchar 	*filename,
						 RomDataView	*page);
static void	rom_data_view_desc_format_type_changed(RpDescFormatType desc_format_type,
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

	/* Timeouts */
	guint		changed_idle;

	// Header row.
	GtkWidget	*hboxHeaderRow;
	GtkWidget	*lblSysInfo;
	GtkWidget	*imgIcon;
	GtkWidget	*imgBanner;
	// TODO: Icon and banner.

	// Filename.
	gchar		*filename;

	// ROM data.
	RomData		*romData;

	// Animated icon data.
	const IconAnimData *iconAnimData;
	// TODO: GdkPixmap or cairo_surface_t?
	// TODO: Use std::array<>?
	GdkPixbuf	*iconFrames[IconAnimData::MAX_FRAMES];
	IconAnimHelper	*iconAnimHelper;
	int		last_frame_number;	// Last frame number.

	// Icon animation timer.
	guint		tmrIconAnim;
	int		last_delay;		// Last delay value.

	// Tab layout.
	GtkWidget	*tabWidget;
	struct tab {
		GtkWidget	*vbox;		// Either page or a GtkVBox/GtkBox.
		GtkWidget	*table;		// GtkTable (2.x); GtkGrid (3.x)
		GtkWidget	*lblCredits;
	};
	vector<tab> *tabs;

	// Description labels.
	RpDescFormatType		desc_format_type;
	vector<GtkWidget*>		*vecDescLabels;
	unordered_set<GtkWidget*>	*setDescLabelIsWarning;

	// Bitfield checkboxes.
	unordered_map<GtkWidget*, gboolean> *mapBitfields;
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

static GParamSpec *properties[PROP_LAST];

static void
rom_data_view_class_init(RomDataViewClass *klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rom_data_view_dispose;
	gobject_class->finalize = rom_data_view_finalize;
	gobject_class->get_property = rom_data_view_get_property;
	gobject_class->set_property = rom_data_view_set_property;

	/**
	 * RomDataView:filename:
	 *
	 * The name of the file being displayed on this page.
	 **/
	properties[PROP_FILENAME] = g_param_spec_string(
		"filename", "Filename", "Filename of the ROM image being displayed.",
		nullptr,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * RomDataView:desc_format_type:
	 *
	 * The formatting to use for description labels.
	 **/
	properties[PROP_DESC_FORMAT_TYPE] = g_param_spec_enum(
		"desc-format-type", "desc-format-type",
		"Description format type.",
		TYPE_RP_DESC_FORMAT_TYPE,
		RP_DFT_XFCE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_property(gobject_class, PROP_FILENAME, properties[PROP_FILENAME]);
	g_object_class_install_property(gobject_class, PROP_DESC_FORMAT_TYPE, properties[PROP_DESC_FORMAT_TYPE]);
}

/**
 * Set the label format type.
 * @param page RomDataView.
 * @param label GtkLabel.
 * @param desc_format_type Format type.
 */
static inline void
set_label_format_type(RomDataView *page, GtkLabel *label, RpDescFormatType desc_format_type)
{
	PangoAttrList *attr_lst = pango_attr_list_new();

	auto iter = page->setDescLabelIsWarning->find(GTK_WIDGET(label));
	const bool is_warning = (iter != page->setDescLabelIsWarning->end());
	if (is_warning) {
		// Use the "Warning" format.
		PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
		pango_attr_list_insert(attr_lst, attr);
		attr = pango_attr_foreground_new(65535, 0, 0);
		pango_attr_list_insert(attr_lst, attr);
	}

	// Check for DE-specific formatting.
	switch (desc_format_type) {
		case RP_DFT_XFCE:
		default:
			// TODO: Changes for XFCE/GTK3.

			// Text alignment: Right
			gtk_label_set_justify(label, GTK_JUSTIFY_RIGHT);
#if GTK_CHECK_VERSION(3,16,0)
			// NOTE: gtk_widget_set_?align() doesn't work properly
			// when using a GtkSizeGroup on GTK+ 3.x.
			// gtk_label_set_?align() was introduced in GTK+ 3.16.
			gtk_label_set_xalign(label, 1.0);
			gtk_label_set_yalign(label, 0.0);
#else
			// NOTE: GtkMisc is deprecated on GTK+ 3.x, but it's
			// needed for proper text alignment when using
			// GtkSizeGroup prior to GTK+ 3.16.
			gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.0f);
#endif

			if (!is_warning) {
				// Text style: Bold
				PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
				pango_attr_list_insert(attr_lst, attr);
			}
			break;

		case RP_DFT_GNOME:
			// TODO: Changes for GNOME 2.

			// Text alignment: Left
			gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
#if GTK_CHECK_VERSION(3,16,0)
			// NOTE: gtk_widget_set_?align() doesn't work properly
			// when using a GtkSizeGroup on GTK+ 3.x.
			// gtk_label_set_?align() was introduced in GTK+ 3.16.
			gtk_label_set_xalign(label, 0.0);
			gtk_label_set_yalign(label, 0.0);
#else
			// NOTE: GtkMisc is deprecated on GTK+ 3.x, but it's
			// needed for proper text alignment when using
			// GtkSizeGroup prior to GTK+ 3.16.
			gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f);
#endif

			// Text style: Normal (no Pango attributes)
			break;
	}

	gtk_label_set_attributes(label, attr_lst);
	pango_attr_list_unref(attr_lst);
}

static void
rom_data_view_init(RomDataView *page)
{
	// No ROM data initially.
	page->filename = nullptr;
	page->romData = nullptr;
	page->last_frame_number = 0;
	page->iconAnimHelper = new IconAnimHelper();
	page->tabWidget = nullptr;
	page->tabs = new vector<RomDataView::tab>();

	page->desc_format_type = RP_DFT_XFCE;
	page->vecDescLabels = new vector<GtkWidget*>();
	page->setDescLabelIsWarning = new unordered_set<GtkWidget*>();
	page->mapBitfields = new unordered_map<GtkWidget*, gboolean>();

	// Animation timer.
	page->tmrIconAnim = 0;
	page->last_delay = 0;

	/**
	 * Base class is:
	 * - GTK+ 2.x: GtkVBox
	 * - GTK+ 3.x: GtkBox
	 */

#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(page), GTK_ORIENTATION_VERTICAL);

	// Header row.
	page->hboxHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
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
	PangoAttrList *attr_lst = pango_attr_list_new();
	PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
	pango_attr_list_insert(attr_lst, attr);
	gtk_label_set_attributes(GTK_LABEL(page->lblSysInfo), attr_lst);
	pango_attr_list_unref(attr_lst);

	// Table layout is created in rom_data_view_update_display().
}

static void
rom_data_view_dispose(GObject *object)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	/* Unregister the changed_idle */
	if (G_UNLIKELY(page->changed_idle != 0)) {
		g_source_remove(page->changed_idle);
		page->changed_idle = 0;
	}

	// Delete the timer.
	if (page->tmrIconAnim > 0) {
		// TODO: Make sure there's no race conditions...
		g_source_remove(page->tmrIconAnim);
		page->tmrIconAnim = 0;
	}

	// Delete the icon frames.
	for (int i = ARRAY_SIZE(page->iconFrames)-1; i >= 0; i--) {
		if (page->iconFrames[i]) {
			g_object_unref(page->iconFrames[i]);
			page->iconFrames[i] = nullptr;
		}
	}

	// Clear the widget reference containers.
	page->tabs->clear();
	page->vecDescLabels->clear();
	page->setDescLabelIsWarning->clear();
	page->mapBitfields->clear();

	// Call the superclass dispose() function.
	(*G_OBJECT_CLASS(rom_data_view_parent_class)->dispose)(object);
}

static void
rom_data_view_finalize(GObject *object)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	// Free the filename.
	g_free(page->filename);

	// Delete the C++ objects.
	delete page->iconAnimHelper;
	delete page->tabs;
	delete page->vecDescLabels;
	delete page->setDescLabelIsWarning;
	delete page->mapBitfields;

	// Unreference romData.
	if (page->romData) {
		page->romData->unref();
	}

	// Call the superclass finalize() function.
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
			g_value_set_string(value, page->filename);
			break;

		case PROP_DESC_FORMAT_TYPE:
			g_value_set_enum(value, page->desc_format_type);
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

		case PROP_DESC_FORMAT_TYPE:
			rom_data_view_set_desc_format_type(page,
				static_cast<RpDescFormatType>(g_value_get_enum(value)));
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
	return page->filename;
}

/**
 * rom_data_view_set_filename:
 * @page : a #RomDataView.
 * @filename : a filename.
 *
 * Sets the filename for this @page.
 **/
void
rom_data_view_set_filename(RomDataView	*page,
			   const gchar	*filename)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));

	/* Check if we already use this file */
	if (G_UNLIKELY(!g_strcmp0(page->filename, filename)))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(page->filename != nullptr)) {
		g_free(page->filename);
		page->filename = nullptr;

		// NULL out iconAnimData.
		// (This is owned by the RomData object.)
		page->iconAnimData = nullptr;

		// Unreference the existing RomData object.
		if (page->romData) {
			page->romData->unref();
			page->romData = nullptr;
		}

		// Delete the icon frames.
		for (int i = ARRAY_SIZE(page->iconFrames)-1; i >= 0; i--) {
			if (page->iconFrames[i]) {
				g_object_unref(page->iconFrames[i]);
				page->iconFrames[i] = nullptr;
			}
		}
	}

	/* Assign the value */
	page->filename = g_strdup(filename);

	/* Connect to the new file (if any) */
	if (G_LIKELY(page->filename != nullptr)) {
		rom_data_view_filename_changed(page->filename, page);
	} else {
		// Hide the header row
		if (page->hboxHeaderRow) {
			gtk_widget_hide(page->hboxHeaderRow);
		}

		// Delete the tabs.
		for (int i = (int)page->tabs->size()-1; i >= 0; i--) {
			auto &tab = page->tabs->at(i);
			if (tab.lblCredits) {
				gtk_widget_destroy(tab.lblCredits);
			}
			if (tab.table) {
				gtk_widget_destroy(tab.table);
			}
			if (tab.vbox && tab.vbox != GTK_WIDGET(page)) {
				gtk_widget_destroy(tab.vbox);
			}
		}
		page->tabs->clear();

		// Clear the various widget references.
		page->vecDescLabels->clear();
		page->setDescLabelIsWarning->clear();
		page->mapBitfields->clear();
	}

	// Filename has been changed.
	g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_FILENAME]);
}

static void
rom_data_view_filename_changed(const gchar	*filename,
			       RomDataView	*page)
{
	g_return_if_fail(filename != nullptr);
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(!g_strcmp0(page->filename, filename));

	if (page->changed_idle == 0) {
		page->changed_idle = g_idle_add(rom_data_view_load_rom_data, page);
	}
}

RpDescFormatType
rom_data_view_get_desc_format_type(RomDataView *page)
{
	g_return_val_if_fail(IS_ROM_DATA_VIEW(page), RP_DFT_XFCE);
	return page->desc_format_type;
}

void
rom_data_view_set_desc_format_type(RomDataView *page, RpDescFormatType desc_format_type)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);
	if (desc_format_type == page->desc_format_type) {
		// Nothing to change.
		// NOTE: g_return_if_fail() prints an assertion warning,
		// so we can't use that for this check.
		return;
	}

	page->desc_format_type = desc_format_type;
	rom_data_view_desc_format_type_changed(desc_format_type, page);
	g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_DESC_FORMAT_TYPE]);
}

static void
rom_data_view_desc_format_type_changed(RpDescFormatType	desc_format_type,
				       RomDataView	*page)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);

	for (auto iter = page->vecDescLabels->cbegin();
	     iter != page->vecDescLabels->cend(); ++iter)
	{
		set_label_format_type(page, GTK_LABEL(*iter), desc_format_type);
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
			// Is this an animated icon?
			page->iconAnimData = page->romData->iconAnimData();
			if (page->iconAnimData) {
				// Convert the icons to GdkPixbuf.
				for (int i = 0; i < page->iconAnimData->count; i++) {
					const rp_image *const frame = page->iconAnimData->frames[i];
					if (frame && frame->isValid()) {
						GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(frame);
						if (pixbuf) {
							page->iconFrames[i] = pixbuf;
						}
					}
				}

				// Set up the IconAnimHelper.
				page->iconAnimHelper->setIconAnimData(page->iconAnimData);
				// Initialize the animation.
				page->last_frame_number = page->iconAnimHelper->frameNumber();

				// Show the first frame.
				gtk_image_set_from_pixbuf(GTK_IMAGE(page->imgIcon),
					page->iconFrames[page->last_frame_number]);
				gtk_widget_show(page->imgIcon);

				// Icon animation timer is set in startAnimTimer().
			} else {
				// Not an animated icon.
				page->last_frame_number = 0;
				GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(icon);
				if (pixbuf) {
					gtk_image_set_from_pixbuf(GTK_IMAGE(page->imgIcon), pixbuf);
					page->iconFrames[0] = pixbuf;
					gtk_widget_show(page->imgIcon);
				}
			}
		}
	}

	// Show the header row.
	gtk_widget_show(page->hboxHeaderRow);
}

#if GTK_CHECK_VERSION(3,0,0)
#define GTK_WIDGET_HALIGN_LEFT(widget)		gtk_widget_set_halign((widget), GTK_ALIGN_START)
#define GTK_WIDGET_HALIGN_CENTER(widget)	gtk_widget_set_halign((widget), GTK_ALIGN_CENTER)
#define GTK_WIDGET_HALIGN_RIGHT(widget)		gtk_widget_set_halign((widget), GTK_ALIGN_END)
#else
#define GTK_WIDGET_HALIGN_LEFT(widget)		gtk_misc_set_alignment(GTK_MISC(widget), 0.0f, 0.0f)
#define GTK_WIDGET_HALIGN_CENTER(widget)	gtk_misc_set_alignment(GTK_MISC(widget), 0.5f, 0.0f)
#define GTK_WIDGET_HALIGN_RIGHT(widget)		gtk_misc_set_alignment(GTK_MISC(widget), 1.0f, 0.0f)
#endif

/**
 * Initialize a string field.
 * @param page RomDataView object.
 * @param field RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_string(RomDataView *page, const RomFields::Field *field)
{
	// String type.
	GtkWidget *widget = gtk_label_new(nullptr);
	gtk_label_set_use_underline(GTK_LABEL(widget), false);
	gtk_widget_show(widget);

	if (field->desc.flags & RomFields::STRF_CREDITS) {
		// Credits text. Enable formatting and center alignment.
		gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);
		GTK_WIDGET_HALIGN_CENTER(widget);
		if (field->data.str) {
			// NOTE: Pango markup does not support <br/>.
			// It uses standard newlines for line breaks.
			gtk_label_set_markup(GTK_LABEL(widget), rp_string_to_utf8(*(field->data.str)).c_str());
		}
	} else {
		// Standard text with no formatting.
		gtk_label_set_selectable(GTK_LABEL(widget), TRUE);
		gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
		GTK_WIDGET_HALIGN_LEFT(widget);
		if (field->data.str) {
			gtk_label_set_text(GTK_LABEL(widget), rp_string_to_utf8(*(field->data.str)).c_str());
		}
	}

	// Check for any formatting options.
	if (field->desc.flags != 0) {
		PangoAttrList *attr_lst = pango_attr_list_new();

		// Monospace font?
		if (field->desc.flags & RomFields::STRF_MONOSPACE) {
			PangoAttribute *attr = pango_attr_family_new("monospace");
			pango_attr_list_insert(attr_lst, attr);
		}

		// "Warning" font?
		if (field->desc.flags & RomFields::STRF_WARNING) {
			PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_HEAVY);
			pango_attr_list_insert(attr_lst, attr);
			attr = pango_attr_foreground_new(65535, 0, 0);
			pango_attr_list_insert(attr_lst, attr);
		}

		gtk_label_set_attributes(GTK_LABEL(widget), attr_lst);
		pango_attr_list_unref(attr_lst);
	}

	if (field->desc.flags & RomFields::STRF_CREDITS) {
		// Credits row goes at the end.
		// There should be a maximum of one STRF_CREDITS per tab.
		auto &tab = page->tabs->at(field->tabIdx);
		assert(tab.lblCredits == nullptr);

		// Credits row.
		gtk_box_pack_end(GTK_BOX(tab.vbox), widget, FALSE, FALSE, 0);

		// NULL out widget to hide the description field.
		// NOTE: Not destroying the widget since we still
		// need it to be displayed.
		widget = nullptr;
	}

	return widget;
}

/**
 * Initialize a bitfield.
 * @param page RomDataView object.
 * @param field RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_bitfield(RomDataView *page, const RomFields::Field *field)
{
	// Bitfield type. Create a grid of checkboxes.
	// TODO: Description label needs some padding on the top...
	const auto &bitfieldDesc = field->desc.bitfield;

#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *widget = gtk_grid_new();
	//gtk_grid_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_grid_set_column_spacings(GTK_TABLE(widget), 8);
#else
	// Determine the total number of rows and columns.
	int totalRows, totalCols;
	if (bitfieldDesc.elemsPerRow == 0) {
		// All elements are in one row.
		totalCols = bitfieldDesc.elements;
		totalRows = 1;
	} else {
		// Multiple rows.
		totalCols = bitfieldDesc.elemsPerRow;
		totalRows = bitfieldDesc.elements / bitfieldDesc.elemsPerRow;
		if ((bitfieldDesc.elements % bitfieldDesc.elemsPerRow) > 0) {
			totalRows++;
		}
	}

	GtkWidget *widget = gtk_table_new(totalRows, totalCols, FALSE);
	//gtk_table_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_table_set_col_spacings(GTK_TABLE(widget), 8);
#endif
	gtk_widget_show(widget);

	// Reserve space in the bitfield widget map.
	page->mapBitfields->reserve(page->mapBitfields->size() + bitfieldDesc.elements);

	int count = bitfieldDesc.elements;
	assert(count <= (int)bitfieldDesc.names->size());
	if (count > (int)bitfieldDesc.names->size()) {
		count = (int)bitfieldDesc.names->size();
	}

	int row = 0, col = 0;
	for (int bit = 0; bit < count; bit++) {
		const rp_string &name = bitfieldDesc.names->at(bit);
		if (name.empty())
			continue;

		GtkWidget *checkBox = gtk_check_button_new_with_label(rp_string_to_utf8(name).c_str());
		gtk_widget_show(checkBox);
		gboolean value = !!(field->data.bitfield & (1 << bit));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkBox), value);

		// Save the bitfield checkbox in the map.
		page->mapBitfields->insert(std::make_pair(checkBox, value));

		// Disable user modifications.
		// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
		// emitted for both user and program modifications, so we have to
		// connect this signal *after* setting the initial value.
		g_signal_connect(checkBox, "toggled",
			reinterpret_cast<GCallback>(checkbox_no_toggle_signal_handler),
			page);

#if GTK_CHECK_VERSION(3,0,0)
		// TODO: GTK_FILL
		gtk_grid_attach(GTK_GRID(widget), checkBox, col, row, 1, 1);
#else
		gtk_table_attach(GTK_TABLE(widget), checkBox, col, col+1, row, row+1,
			GTK_FILL, GTK_FILL, 0, 0);
#endif
		col++;
		if (col == bitfieldDesc.elemsPerRow) {
			row++;
			col = 0;
		}
	}

	return widget;
}

/**
 * Initialize a list data field.
 * @param page RomDataView object.
 * @param field RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_listdata(G_GNUC_UNUSED RomDataView *page, const RomFields::Field *field)
{
	// ListData type. Create a GtkListStore for the data.
	const auto &listDataDesc = field->desc.list_data;
	assert(listDataDesc.names != nullptr);
	if (!listDataDesc.names) {
		// No column names...
		return nullptr;
	}

	// Set up the column names.
	const int count = (int)listDataDesc.names->size();
	GType *types = new GType[count];
	for (int i = 0; i < count; i++) {
		types[i] = G_TYPE_STRING;
	}
	GtkListStore *listStore = gtk_list_store_newv(count, types);
	delete[] types;

	// Add the row data.
	auto list_data = field->data.list_data;
	assert(list_data != nullptr);
	if (list_data) {
		const int count = (int)list_data->size();
		for (int i = 0; i < count; i++) {
			const vector<rp_string> &data_row = list_data->at(i);
			GtkTreeIter treeIter;
			gtk_list_store_insert_before(listStore, &treeIter, nullptr);
			int col = 0;
			for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, ++col) {
				gtk_list_store_set(listStore, &treeIter,
					col, rp_string_to_utf8(*iter).c_str(), -1);
			}
		}
	}

	// Scroll area for the GtkTreeView.
	GtkWidget *widget = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(widget);

	// Create the GtkTreeView.
	GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), TRUE);
	gtk_widget_show(treeView);
	//treeWidget->setRootIsDecorated(false);
	//treeWidget->setUniformRowHeights(true);
	gtk_container_add(GTK_CONTAINER(widget), treeView);

	// Set up the column names.
	for (int i = 0; i < count; i++) {
		const rp_string &name = listDataDesc.names->at(i);
		if (!name.empty()) {
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
				rp_string_to_utf8(name).c_str(), renderer,
				"text", i, nullptr);
			gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
		}
	}

	// Set a minimum height for the scroll area.
	// TODO: Adjust for DPI, and/or use a font size?
	// TODO: Force maximum horizontal width somehow?
	gtk_widget_set_size_request(widget, -1, 128);

	// Resize the columns to fit the contents.
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeView));

	return widget;
}

/**
 * Initialize a Date/Time field.
 * @param page RomDataView object.
 * @param field RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_datetime(G_GNUC_UNUSED RomDataView *page, const RomFields::Field *field)
{
	// Date/Time.
	GtkWidget *widget = gtk_label_new(nullptr);
	gtk_label_set_use_underline(GTK_LABEL(widget), false);
	gtk_label_set_selectable(GTK_LABEL(widget), TRUE);
	gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
	gtk_widget_show(widget);
	GTK_WIDGET_HALIGN_LEFT(widget);

	if (field->data.date_time == -1) {
		// Invalid date/time.
		gtk_label_set_text(GTK_LABEL(widget), "Unknown");
		return widget;
	}

	GDateTime *dateTime;
	if (field->desc.flags & RomFields::RFT_DATETIME_IS_UTC) {
		dateTime = g_date_time_new_from_unix_utc(field->data.date_time);
	} else {
		dateTime = g_date_time_new_from_unix_local(field->data.date_time);
	}

	gchar *str = nullptr;
	switch (field->desc.flags &
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
		gtk_label_set_text(GTK_LABEL(widget), str);
		g_free(str);
	} else {
		// Invalid date/time.
		gtk_widget_destroy(widget);
		widget = nullptr;
	}
	return widget;
}

/**
 * Initialize an Age Ratings field.
 * @param page RomDataView object.
 * @param field RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_age_ratings(G_GNUC_UNUSED RomDataView *page, const RomFields::Field *field)
{
	// Age ratings.
	GtkWidget *widget = gtk_label_new(nullptr);
	gtk_label_set_use_underline(GTK_LABEL(widget), false);
	gtk_label_set_selectable(GTK_LABEL(widget), TRUE);
	gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
	gtk_widget_show(widget);
	GTK_WIDGET_HALIGN_LEFT(widget);

	const RomFields::age_ratings_t *age_ratings = field->data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		gtk_label_set_text(GTK_LABEL(widget), "ERROR");
		return widget;
	}

	// Convert the age ratings field to a string.
	ostringstream oss;
	unsigned int ratings_count = 0;
	for (int i = 0; i < (int)age_ratings->size(); i++) {
		const uint16_t rating = age_ratings->at(i);
		if (!(rating & RomFields::AGEBF_ACTIVE))
			continue;

		if (ratings_count > 0) {
			// Append a comma.
			if (ratings_count % 4 == 0) {
				// 4 ratings per line.
				oss << ",\n";
			} else {
				oss << ", ";
			}
		}

		const char *abbrev = RomFields::ageRatingAbbrev(i);
		if (abbrev) {
			oss << abbrev;
		} else {
			// Invalid age rating.
			// Use the numeric index.
			oss << i;
		}
		oss << '=' << RomFields::ageRatingDecode(i, rating);
		ratings_count++;
	}

	if (ratings_count == 0) {
		// No age ratings.
		oss << "None";
	}

	gtk_label_set_text(GTK_LABEL(widget), oss.str().c_str());
	return widget;
}

static void
rom_data_view_update_display(RomDataView *page)
{
	assert(page != nullptr);

	// Initialize the header row.
	rom_data_view_init_header_row(page);

	// Delete the tabs.
	for (int i = (int)page->tabs->size()-1; i >= 0; i--) {
		auto &tab = page->tabs->at(i);
		if (tab.lblCredits) {
			gtk_widget_destroy(tab.lblCredits);
		}
		if (tab.table) {
			gtk_widget_destroy(tab.table);
		}
		if (tab.vbox && tab.vbox != GTK_WIDGET(page)) {
			gtk_widget_destroy(tab.vbox);
		}
	}
	page->tabs->clear();

	// Clear the various widget references.
	page->vecDescLabels->clear();
	page->setDescLabelIsWarning->clear();
	page->mapBitfields->clear();

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

	// Create the GtkNotebook.
	if (fields->tabCount() > 1) {
		page->tabs->resize(fields->tabCount());
		page->tabWidget = gtk_notebook_new();
		for (int i = 0; i < fields->tabCount(); i++) {
			// Create a tab.
			const rp_char *name = fields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = page->tabs->at(i);
#if GTK_CHECK_VERSION(3,0,0)
			tab.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
			tab.table = gtk_grid_new();
			gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
			gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else
			tab.vbox = gtk_vbox_new(FALSE, 8);
			// TODO: Adjust the table size?
			tab.table = gtk_table_new(count, 2, FALSE);
			gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
			gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif

			gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
			gtk_box_pack_start(GTK_BOX(tab.vbox), tab.table, FALSE, FALSE, 0);
			gtk_widget_show(tab.table);
			gtk_widget_show(tab.vbox);

			// Add the tab.
			GtkWidget *label = gtk_label_new(rp_string_to_utf8(name).c_str());
			gtk_notebook_append_page(GTK_NOTEBOOK(page->tabWidget), tab.vbox, label);
		}
		gtk_box_pack_start(GTK_BOX(page), page->tabWidget, TRUE, TRUE, 0);
		gtk_widget_show(page->tabWidget);
	} else {
		// No tabs.
		// Don't create a GtkNotebook, but simulate a single
		// tab in page->tabs[] to make it easier to work with.
		page->tabs->resize(1);
		auto &tab = page->tabs->at(0);
		tab.vbox = GTK_WIDGET(page);
#if GTK_CHECK_VERSION(3,0,0)
		tab.table = gtk_grid_new();
		gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
		gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else
		// TODO: Adjust the table size?
		tab.table = gtk_table_new(count, 2, FALSE);
		gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
		gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif

		gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
		gtk_box_pack_start(GTK_BOX(page), tab.table, FALSE, FALSE, 0);
		gtk_widget_show(tab.table);
	}

	// Reserve enough space for vecDescLabels.
	page->vecDescLabels->reserve(count);
	// Per-tab row counts.
	vector<int> tabRowCount(page->tabs->size());

	// Use a GtkSizeGroup to ensure that the description
	// labels on all tabs have the same width.
	// NOTE: GtkSizeGroup automatically unreferences itself
	// once all referenced widgets are deleted, so we don't
	// need to manage it ourselves.
	GtkSizeGroup *size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	// Create the data widgets.
	for (int i = 0; i < count; i++) {
		const RomFields::Field *field = fields->field(i);
		assert(field != nullptr);
		if (!field || !field->isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field->tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)page->tabs->size());
		if (tabIdx < 0 || tabIdx >= (int)page->tabs->size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!page->tabs->at(tabIdx).table) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		GtkWidget *widget = nullptr;
		switch (field->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				break;

			case RomFields::RFT_STRING:
				widget = rom_data_view_init_string(page, field);
				break;

			case RomFields::RFT_BITFIELD:
				widget = rom_data_view_init_bitfield(page, field);
				break;

			case RomFields::RFT_LISTDATA:
				widget = rom_data_view_init_listdata(page, field);
				break;

			case RomFields::RFT_DATETIME:
				widget = rom_data_view_init_datetime(page, field);
				break;

			case RomFields::RFT_AGE_RATINGS:
				widget = rom_data_view_init_age_ratings(page, field);
				break;

			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;
		}

		if (widget) {
			// Add the widget to the table.
			auto &tab = page->tabs->at(tabIdx);

			// TODO: Localization.
			std::string gtkdesc = rp_string_to_utf8(field->name);
			gtkdesc += ':';

			// Description label.
			GtkWidget *lblDesc = gtk_label_new(gtkdesc.c_str());
			gtk_label_set_use_underline(GTK_LABEL(lblDesc), false);
			gtk_widget_show(lblDesc);
			gtk_size_group_add_widget(size_group, lblDesc);
			page->vecDescLabels->push_back(lblDesc);

			// Check if this is an RFT_STRING with warning set.
			if (field->type == RomFields::RFT_STRING &&
			    field->desc.flags & RomFields::STRF_WARNING)
			{
				// Description label should use the "warning" style.
				page->setDescLabelIsWarning->insert(lblDesc);
			}
			set_label_format_type(page, GTK_LABEL(lblDesc), page->desc_format_type);

			// Value widget.
			int &row = tabRowCount[tabIdx];
#if GTK_CHECK_VERSION(3,0,0)
			// TODO: GTK_FILL
			gtk_grid_attach(GTK_GRID(tab.table), lblDesc, 0, row, 1, 1);

			// Widget halign is set above.
			gtk_widget_set_valign(widget, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(tab.table), widget, 1, row, 1, 1);
#else
			gtk_table_attach(GTK_TABLE(tab.table), lblDesc, 0, 1, row, row+1,
				GTK_FILL, GTK_FILL, 0, 0);
			gtk_table_attach(GTK_TABLE(tab.table), widget, 1, 2, row, row+1,
				GTK_FILL, GTK_FILL, 0, 0);
#endif
			row++;
		}
	}
}

static gboolean
rom_data_view_load_rom_data(gpointer data)
{
	RomDataView *page = ROM_DATA_VIEW(data);
	g_return_val_if_fail(page != nullptr || IS_ROM_DATA_VIEW(page), FALSE);
	g_return_val_if_fail(page->filename != nullptr, FALSE);

	// Open the ROM file.
	// TODO: gvfs support.
	if (G_LIKELY(page->filename != nullptr)) {
		// Open the ROM file.
		RpFile *file = new RpFile(page->filename, RpFile::FM_OPEN_READ);
		if (file->isOpen()) {
			// Create the RomData object.
			page->romData = RomDataFactory::create(file, false);

			// Update the display widgets.
			rom_data_view_update_display(page);

			// Make sure the underlying file handle is closed,
			// since we don't need it once the RomData has been
			// loaded by RomDataView.
			if (page->romData) {
				page->romData->close();
			}
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

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param togglebutton Bitfield checkbox.
 * @param user_data RomDataView*.
 */
static void
checkbox_no_toggle_signal_handler(GtkToggleButton	*togglebutton,
				  gpointer		 user_data)
{
	((void)user_data);
	RomDataView *page = static_cast<RomDataView*>(user_data);

	// Check if this GtkToggleButton is present in the map.
	auto iter = page->mapBitfields->find(GTK_WIDGET(togglebutton));
	if (iter != page->mapBitfields->end()) {
		// Check if the status is correct.
		gboolean status = iter->second;
		if (gtk_toggle_button_get_active(togglebutton) != status) {
			// Toggle this box.
			gtk_toggle_button_set_active(togglebutton, status);
		}
	}
}

/** Icon animation timer. **/

/**
 * Start the animation timer.
 */
static void start_anim_timer(RomDataView *page)
{
	if (!page->iconAnimData || !page->iconAnimHelper->isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	page->last_frame_number = page->iconAnimHelper->frameNumber();
	const int delay = page->iconAnimHelper->frameDelay();
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
	int frame = page->iconAnimHelper->nextFrame(&delay);
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

/** RpDescFormatType **/

// TODO: Use glib-mkenums to generate the enum type functions.
// Reference: https://arosenfeld.wordpress.com/2010/08/11/glib-mkenums/
GType rp_desc_format_type_get_type(void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			{ RP_DFT_XFCE, "RP_DFT_XFCE", "XFCE style (default)" },
			{ RP_DFT_GNOME, "RP_DFT_GNOME", "GNOME style" },
			{ RP_DFT_LAST, "RP_DFT_LAST", "last" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("RpDescFormatType", values);
	}
	return etype;
}

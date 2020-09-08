/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget.                                 *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataView.hpp"
#include "rp-gtk-enums.h"

// ENABLE_MESSAGESOUND is set by CMakeLists.txt.
#ifdef ENABLE_MESSAGESOUND
#  include "MessageSound.hpp"
#endif /* ENABLE_MESSAGESOUND */

// Custom widgets
#include "DragImage.hpp"
#include "MessageWidget.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;
using LibRpFile::RpFile;
using LibRpTexture::rp_image;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C++ STL classes.
#include <fstream>
#include <sstream>
using std::array;
using std::ofstream;
using std::ostringstream;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

// GTK+ 2.x compatibility macros.
// Reference: https://github.com/kynesim/wireshark/blob/master/ui/gtk/old-gtk-compat.h
#if !GTK_CHECK_VERSION(3,0,0)
static inline GtkWidget *gtk_tree_view_column_get_button(GtkTreeViewColumn *tree_column)
{
	/* This is too late, see https://bugzilla.gnome.org/show_bug.cgi?id=641089
	 * According to
	 * http://ftp.acc.umu.se/pub/GNOME/sources/gtk+/2.13/gtk+-2.13.4.changes
	 * access to the button element was sealed during 2.13. They also admit that
	 * they missed a use case and thus failed to provide an accessor function:
	 * http://mail.gnome.org/archives/commits-list/2010-December/msg00578.html
	 * An accessor function was finally added in 3.0.
	 */
# if (GTK_CHECK_VERSION(2,14,0) && defined(GSEAL_ENABLE))
	return tree_column->_g_sealed__button;
# else
	return tree_column->button;
# endif
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */


// References:
// - audio-tags plugin
// - http://api.xfce.m8t.in/xfce-4.10/thunarx-1.4.0/ThunarxPropertyPage.html

/* Property identifiers */
typedef enum {
	PROP_0,
	PROP_URI,
	PROP_DESC_FORMAT_TYPE,
	PROP_SHOWING_DATA,
	PROP_LAST
} RomDataViewPropID;
static GParamSpec *properties[PROP_LAST];

// Use GtkButton or GtkMenuButton?
#if GTK_CHECK_VERSION(3,6,0)
#  define USE_GTK_MENU_BUTTON 1
#endif

// Uncomment to enable the automatic timeout for the ROM Operations MessageWidget.
//#define AUTO_TIMEOUT_MESSAGEWIDGET 1

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

static void	rom_data_view_desc_format_type_changed(RomDataView *page,
						 RpDescFormatType desc_format_type);

static void	rom_data_view_init_header_row	(RomDataView	*page);
static void	rom_data_view_update_display	(RomDataView	*page);
static gboolean	rom_data_view_load_rom_data	(gpointer	 data);
static void	rom_data_view_delete_tabs	(RomDataView	*page);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler   (GtkToggleButton	*togglebutton,
						     gpointer		 user_data);
static void	rom_data_view_map_signal_handler    (RomDataView	*page,
						     gpointer		 user_data);
static void	rom_data_view_unmap_signal_handler  (RomDataView	*page,
						     gpointer		 user_data);
static void	tree_view_realize_signal_handler    (GtkTreeView	*treeView,
						     RomDataView	*page);
static void	cboLanguage_changed_signal_handler  (GtkComboBox	*widget,
						     gpointer		 user_data);
#ifndef USE_GTK_MENU_BUTTON
static gboolean	btnOptions_clicked_signal_handler   (GtkButton		*button,
						     GdkEvent	 	*event);
#endif /* !USE_GTK_MENU_BUTTON */
static void	menuOptions_triggered_signal_handler(GtkMenuItem	*menuItem,
						     gpointer		 user_data);

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif

// GTK+ property page class.
struct _RomDataViewClass {
	superclass __parent__;
};

// Multi-language stuff.
typedef enum _StringMultiColumns { SM_COL_ICON, SM_COL_TEXT, SM_COL_LC } StringMultiColumns;
typedef std::pair<GtkWidget*, const RomFields::Field*> Data_StringMulti_t;

enum StandardOptionID {
	OPTION_EXPORT_TEXT = -1,
	OPTION_EXPORT_JSON = -2,
	OPTION_COPY_TEXT = -3,
	OPTION_COPY_JSON = -4,
};

struct Data_ListDataMulti_t {
	GtkListStore *listStore;
	GtkTreeView *treeView;
	const RomFields::Field *field;

	Data_ListDataMulti_t(
		GtkListStore *listStore,
		GtkTreeView *treeView,
		const RomFields::Field *field)
		: listStore(listStore)
		, treeView(treeView)
		, field(field) { }
};

// GTK+ property page instance.
struct _RomDataView {
	super __parent__;

	/* Timeouts */
	guint		changed_idle;

	// "Options" button.
	GtkWidget	*btnOptions;
	GtkWidget	*menuOptions;
	gchar		*prevExportDir;

	// Header row.
	GtkWidget	*hboxHeaderRow_outer;
	GtkWidget	*hboxHeaderRow;
	GtkWidget	*lblSysInfo;
	GtkWidget	*imgIcon;
	GtkWidget	*imgBanner;

	// URI. (GVfs)
	gchar		*uri;

	// ROM data.
	RomData		*romData;

	// Tab layout.
	GtkWidget	*tabWidget;
	struct tab {
		GtkWidget	*vbox;		// Either page or a GtkVBox/GtkBox.
		GtkWidget	*table;		// GtkTable (2.x); GtkGrid (3.x)
		GtkWidget	*lblCredits;

		tab() : vbox(nullptr), table(nullptr), lblCredits(nullptr) { }
	};
	vector<tab>	*tabs;

	// Mapping of field index to GtkWidget*.
	// For rom_data_view_update_field().
	unordered_map<int, GtkWidget*> *map_fieldIdx;
	bool inhibit_checkbox_no_toggle;

	// MessageWidget for ROM operation notifications.
	GtkWidget	*messageWidget;

	// Description labels.
	RpDescFormatType	desc_format_type;
	vector<GtkWidget*>	*vecDescLabels;

	// Multi-language functionality.
	uint32_t	def_lc;
	set<uint32_t>	*set_lc;	// Set of supported language codes.
	GtkWidget	*cboLanguage;
	GtkListStore	*lstoreLanguage;

	// RFT_STRING_MULTI value labels.
	vector<Data_StringMulti_t> *vecStringMulti;

	// RFT_LISTDATA_MULTI value GtkListStores.
	vector<Data_ListDataMulti_t> *vecListDataMulti;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RomDataView, rom_data_view,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rom_data_view_class_init(RomDataViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->dispose = rom_data_view_dispose;
	gobject_class->finalize = rom_data_view_finalize;
	gobject_class->get_property = rom_data_view_get_property;
	gobject_class->set_property = rom_data_view_set_property;

	/** Properties **/

	properties[PROP_URI] = g_param_spec_string(
		"uri", "URI", "URI of the ROM image being displayed.",
		nullptr,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	properties[PROP_DESC_FORMAT_TYPE] = g_param_spec_enum(
		"desc-format-type", "desc-format-type", "Description format type.",
		RP_TYPE_DESC_FORMAT_TYPE, RP_DFT_XFCE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	properties[PROP_SHOWING_DATA] = g_param_spec_boolean(
		"showing-data", "showing-data", "Is a valid RomData object being displayed?",
		false,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, properties);
}

/**
 * Set the label format type.
 * @param page RomDataView.
 * @param label GtkLabel.
 * @param desc_format_type Format type.
 */
static inline void
set_label_format_type(GtkLabel *label, RpDescFormatType desc_format_type)
{
	PangoAttrList *attr_lst = pango_attr_list_new();

	// Check if this label has the "Warning" flag set.
	const gboolean is_warning = (gboolean)GPOINTER_TO_UINT(
		g_object_get_data(G_OBJECT(label), "RFT_STRING_warning"));
	if (is_warning) {
		// Use the "Warning" format.
		pango_attr_list_insert(attr_lst,
			pango_attr_weight_new(PANGO_WEIGHT_BOLD));
		pango_attr_list_insert(attr_lst,
			pango_attr_foreground_new(65535, 0, 0));
	}

	// Check for DE-specific formatting.
	GtkJustification justify;
	float xalign, yalign;
	switch (desc_format_type) {
		case RP_DFT_XFCE:
		default:
			// XFCE style.
			// NOTE: No changes between GTK+ 2.x and 3.x.

			// Text alignment: Right
			justify = GTK_JUSTIFY_RIGHT;
			xalign = 1.0f;
			yalign = 0.0f;

			if (!is_warning) {
				// Text style: Bold
				pango_attr_list_insert(attr_lst,
					pango_attr_weight_new(PANGO_WEIGHT_BOLD));
			}
			break;

		case RP_DFT_GNOME:
			// GNOME style. (Also used for MATE and Cinnamon.)
			// TODO: Changes for GNOME 2.

			// Text alignment: Left
			justify = GTK_JUSTIFY_LEFT;
			xalign = 0.0f;
			yalign = 0.0f;

			// Text style: Normal (no Pango attributes)
			break;
	}

	gtk_label_set_justify(label, justify);

#if GTK_CHECK_VERSION(3,16,0)
	// NOTE: gtk_widget_set_?align() doesn't work properly
	// when using a GtkSizeGroup on GTK+ 3.x.
	// gtk_label_set_?align() was introduced in GTK+ 3.16.
	gtk_label_set_xalign(label, xalign);
	gtk_label_set_yalign(label, yalign);
#else
	// NOTE: GtkMisc is deprecated on GTK+ 3.x, but it's
	// needed for proper text alignment when using
	// GtkSizeGroup prior to GTK+ 3.16.
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
#endif

	gtk_label_set_attributes(label, attr_lst);
	pango_attr_list_unref(attr_lst);
}

static void
rom_data_view_init(RomDataView *page)
{
	// No ROM data initially.
	page->uri = nullptr;
	page->romData = nullptr;
	page->tabWidget = nullptr;
	page->tabs = new vector<RomDataView::tab>();
	page->map_fieldIdx = new unordered_map<int, GtkWidget*>();
	page->inhibit_checkbox_no_toggle = false;

	page->messageWidget = nullptr;
	page->desc_format_type = RP_DFT_XFCE;

	page->def_lc = 0;
	page->set_lc = new set<uint32_t>();
	page->cboLanguage = nullptr;
	page->lstoreLanguage = nullptr;
	page->vecDescLabels = new vector<GtkWidget*>();
	page->vecStringMulti = new vector<Data_StringMulti_t>();
	page->vecListDataMulti = new vector<Data_ListDataMulti_t>();

	/**
	 * Base class is:
	 * - GTK+ 2.x: GtkVBox
	 * - GTK+ 3.x: GtkBox
	 */

	// NOTE: This matches Thunar (GTK+2) and Nautilus (GTK+3).
	g_object_set(page, "border-width", 8, nullptr);

#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(page), GTK_ORIENTATION_VERTICAL);

	// Header row. (outer box)
	// NOTE: Not visible initially.
	page->hboxHeaderRow_outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow_outer, false, false, 0);

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_halign(page->hboxHeaderRow, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow_outer), page->hboxHeaderRow, true, false, 0);
	gtk_widget_show(page->hboxHeaderRow);
#else /* !GTK_CHECK_VERSION(3,0,0) */
	// Header row. (outer box)
	// NOTE: Not visible initially.
	page->hboxHeaderRow_outer = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow_outer, false, false, 0);

	// Center-align the header row.
	GtkWidget *centerAlign = gtk_alignment_new(0.5f, 0.0f, 1.0f, 0.0f);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow_outer), centerAlign, true, false, 0);
	gtk_widget_show(centerAlign);

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_hbox_new(false, 8);
	gtk_container_add(GTK_CONTAINER(page->hboxHeaderRow_outer), page->hboxHeaderRow);
	gtk_widget_show(page->hboxHeaderRow);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// System information.
	page->lblSysInfo = gtk_label_new(nullptr);
	gtk_label_set_justify(GTK_LABEL(page->lblSysInfo), GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->lblSysInfo, false, false, 0);
	gtk_widget_show(page->lblSysInfo);

	// Banner.
	page->imgBanner = drag_image_new();
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgBanner, false, false, 0);

	// Icon.
	page->imgIcon = drag_image_new();
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgIcon, false, false, 0);

	// Make lblSysInfo bold.
	PangoAttrList *attr_lst = pango_attr_list_new();
	pango_attr_list_insert(attr_lst,
		pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes(GTK_LABEL(page->lblSysInfo), attr_lst);
	pango_attr_list_unref(attr_lst);

	// Connect the "map" and "unmap" signals.
	// These are needed in order to start and stop the animation.
	g_signal_connect(page, "map",
		G_CALLBACK(rom_data_view_map_signal_handler), nullptr);
	g_signal_connect(page, "unmap",
		G_CALLBACK(rom_data_view_unmap_signal_handler), nullptr);

	// Table layout is created in rom_data_view_update_display().
}

static void
rom_data_view_dispose(GObject *object)
{
	RomDataView *const page = ROM_DATA_VIEW(object);

	/* Unregister the changed_idle */
	if (G_UNLIKELY(page->changed_idle > 0)) {
		g_source_remove(page->changed_idle);
		page->changed_idle = 0;
	}

#ifndef USE_GTK_MENU_BUTTON
	// Delete the "Options" button menu.
	if (page->menuOptions) {
		gtk_widget_destroy(page->menuOptions);
		page->menuOptions = nullptr;
	}
#endif /* !USE_GTK_MENU_BUTTON */

	// Delete the icon frames and tabs.
	rom_data_view_delete_tabs(page);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rom_data_view_parent_class)->dispose(object);
}

static void
rom_data_view_finalize(GObject *object)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	// Free the strings.
	g_free(page->prevExportDir);
	g_free(page->uri);

	// Delete the C++ objects.
	delete page->tabs;
	delete page->map_fieldIdx;
	delete page->vecDescLabels;

	delete page->set_lc;
	delete page->vecStringMulti;
	delete page->vecListDataMulti;

	// Unreference romData.
	UNREF(page->romData);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rom_data_view_parent_class)->finalize(object);
}

GtkWidget*
rom_data_view_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(TYPE_ROM_DATA_VIEW, nullptr));
}

GtkWidget*
rom_data_view_new_with_uri(const gchar *uri, RpDescFormatType desc_format_type)
{
	RomDataView *const page = static_cast<RomDataView*>(g_object_new(TYPE_ROM_DATA_VIEW, nullptr));
	page->desc_format_type = desc_format_type;
	page->uri = g_strdup(uri);
	if (G_LIKELY(page->uri != nullptr)) {
		page->changed_idle = g_idle_add(rom_data_view_load_rom_data, page);
	}

	return reinterpret_cast<GtkWidget*>(page);
}

/** Properties **/

static void
rom_data_view_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	RomDataView *page = ROM_DATA_VIEW(object);

	switch (prop_id) {
		case PROP_URI:
			g_value_set_string(value, page->uri);
			break;

		case PROP_DESC_FORMAT_TYPE:
			g_value_set_enum(value, page->desc_format_type);
			break;

		case PROP_SHOWING_DATA:
			g_value_set_boolean(value, (page->romData != nullptr));
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
		case PROP_URI:
			rom_data_view_set_uri(page, g_value_get_string(value));
			break;

		case PROP_DESC_FORMAT_TYPE:
			rom_data_view_set_desc_format_type(page,
				static_cast<RpDescFormatType>(g_value_get_enum(value)));
			break;

		case PROP_SHOWING_DATA:	// TODO: "Non-writable property" warning?
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/**
 * rom_data_view_get_uri:
 * @page : a #RomDataView.
 *
 * Returns the current URI for the @page.
 *
 * Return value: the file associated with this property page.
 **/
const gchar*
rom_data_view_get_uri(RomDataView *page)
{
	g_return_val_if_fail(IS_ROM_DATA_VIEW(page), nullptr);
	return page->uri;
}

/**
 * rom_data_view_set_uri:
 * @page : a #RomDataView.
 * @uri : a URI.
 *
 * Sets the URI for this @page.
 **/
void
rom_data_view_set_uri(RomDataView	*page,
		      const gchar	*uri)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));

	/* Check if we already use this file */
	if (G_UNLIKELY(!g_strcmp0(page->uri, uri)))
		return;

	/* Disconnect from the previous file (if any) */
	if (G_LIKELY(page->uri != nullptr)) {
		g_free(page->uri);
		page->uri = nullptr;

		// Unreference the existing RomData object.
		UNREF_AND_NULL(page->romData);

		// Delete the icon frames and tabs.
		rom_data_view_delete_tabs(page);
	}

	/* Assign the value */
	page->uri = g_strdup(uri);

	/* Connect to the new file (if any) */
	if (G_LIKELY(page->uri != nullptr)) {
		if (page->changed_idle == 0) {
			page->changed_idle = g_idle_add(rom_data_view_load_rom_data, page);
		}
	} else {
		// Hide the header row. (outerbox)
		if (page->hboxHeaderRow_outer) {
			gtk_widget_hide(page->hboxHeaderRow_outer);
		}
	}

	// URI has been changed.
	g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_URI]);
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
	rom_data_view_desc_format_type_changed(page, desc_format_type);
	g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_DESC_FORMAT_TYPE]);
}

static void
rom_data_view_desc_format_type_changed(RomDataView	*page,
				       RpDescFormatType	desc_format_type)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);

	std::for_each(page->vecDescLabels->cbegin(), page->vecDescLabels->cend(),
		[desc_format_type](GtkWidget *label) {
			set_label_format_type(GTK_LABEL(label), desc_format_type);
		}
	);
}

gboolean
rom_data_view_is_showing_data(RomDataView *page)
{
	// FIXME: This was intended to be used to determine if
	// the RomData was valid, but the RomData isn't loaded
	// until idle is processed...
	g_return_val_if_fail(IS_ROM_DATA_VIEW(page), false);
	return (page->romData != nullptr);
}

static void
rom_data_view_init_header_row(RomDataView *page)
{
	// Initialize the header row.
	assert(page != nullptr);

	// NOTE: romData might be nullptr in some cases.
	const RomData *const romData = page->romData;
	//assert(romData != nullptr);
	if (!romData) {
		// No ROM data.
		// Hide the widgets.
		gtk_widget_hide(page->hboxHeaderRow);
		return;
	}

	// System name and file type.
	// TODO: System logo and/or game title?
	const char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *fileType = romData->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);
	if (!systemName) {
		systemName = C_("RomDataView", "(unknown system)");
	}
	if (!fileType) {
		fileType = C_("RomDataView", "(unknown filetype)");
	}

	const string sysInfo = rp_sprintf_p(
		// tr: %1$s == system name, %2$s == file type
		C_("RomDataView", "%1$s\n%2$s"), systemName, fileType);
	gtk_label_set_text(GTK_LABEL(page->lblSysInfo), sysInfo.c_str());

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	gtk_widget_hide(page->imgBanner);
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		bool ok = drag_image_set_rp_image(DRAG_IMAGE(page->imgBanner), romData->image(RomData::IMG_INT_BANNER));
		if (ok) {
			gtk_widget_show(page->imgBanner);
		}
	}

	// Icon.
	gtk_widget_hide(page->imgIcon);
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image *const icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Is this an animated icon?
			bool ok = drag_image_set_icon_anim_data(DRAG_IMAGE(page->imgIcon), romData->iconAnimData());
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = drag_image_set_rp_image(DRAG_IMAGE(page->imgIcon), icon);
			}
			if (ok) {
				gtk_widget_show(page->imgIcon);
			}
		}
	}

	// Show the header row. (outer box)
	gtk_widget_show(page->hboxHeaderRow_outer);
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
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @param str		[in,opt] String data (If nullptr, field data is used)
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_string(RomDataView *page,
	const RomFields::Field &field, int fieldIdx,
	const char *str = nullptr)
{
	// String type.
	GtkWidget *widget = gtk_label_new(nullptr);
	gtk_label_set_use_underline(GTK_LABEL(widget), false);
	gtk_widget_show(widget);

	if (!str && field.data.str) {
		str = field.data.str->c_str();
	}

	if (field.type == RomFields::RFT_STRING &&
	    (field.desc.flags & RomFields::STRF_CREDITS))
	{
		// Credits text. Enable formatting and center alignment.
		gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_CENTER);
		GTK_WIDGET_HALIGN_CENTER(widget);
		if (str) {
			// NOTE: Pango markup does not support <br/>.
			// It uses standard newlines for line breaks.
			gtk_label_set_markup(GTK_LABEL(widget), str);
		}
	} else {
		// Standard text with no formatting.
		gtk_label_set_selectable(GTK_LABEL(widget), true);
		gtk_label_set_justify(GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
		GTK_WIDGET_HALIGN_LEFT(widget);
		if (str) {
			gtk_label_set_text(GTK_LABEL(widget), str);
		}
	}

	// NOTE: Add the widget to the field index map here,
	// since `widget` might be NULLed out later.
	page->map_fieldIdx->insert(std::make_pair(fieldIdx, widget));

	// Check for any formatting options. (RFT_STRING only)
	if (field.type == RomFields::RFT_STRING && field.desc.flags != 0) {
		PangoAttrList *const attr_lst = pango_attr_list_new();

		// Monospace font?
		if (field.desc.flags & RomFields::STRF_MONOSPACE) {
			pango_attr_list_insert(attr_lst,
				pango_attr_family_new("monospace"));
		}

		// "Warning" font?
		if (field.desc.flags & RomFields::STRF_WARNING) {
			pango_attr_list_insert(attr_lst,
				pango_attr_weight_new(PANGO_WEIGHT_BOLD));
			pango_attr_list_insert(attr_lst,
				pango_attr_foreground_new(65535, 0, 0));
		}

		gtk_label_set_attributes(GTK_LABEL(widget), attr_lst);
		pango_attr_list_unref(attr_lst);

		if (field.desc.flags & RomFields::STRF_CREDITS) {
			// Credits row goes at the end.
			// There should be a maximum of one STRF_CREDITS per tab.
			auto &tab = page->tabs->at(field.tabIdx);
			assert(tab.lblCredits == nullptr);

			// Credits row.
			gtk_box_pack_end(GTK_BOX(tab.vbox), widget, false, false, 0);

			// NULL out widget to hide the description field.
			// NOTE: Not destroying the widget since we still
			// need it to be displayed.
			widget = nullptr;
		}
	}

	return widget;
}

/**
 * Initialize a bitfield.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_bitfield(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// Bitfield type. Create a grid of checkboxes.
	// TODO: Description label needs some padding on the top...
	const auto &bitfieldDesc = field.desc.bitfield;

	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32)
		count = 32;

#ifdef USE_GTK_GRID
	GtkWidget *widget = gtk_grid_new();
	//gtk_grid_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_grid_set_column_spacings(GTK_TABLE(widget), 8);
#else /* !USE_GTK_GRID */
	// Determine the total number of rows and columns.
	int totalRows, totalCols;
	if (bitfieldDesc.elemsPerRow == 0) {
		// All elements are in one row.
		totalCols = count;
		totalRows = 1;
	} else {
		// Multiple rows.
		totalCols = bitfieldDesc.elemsPerRow;
		totalRows = count / bitfieldDesc.elemsPerRow;
		if ((count % bitfieldDesc.elemsPerRow) > 0) {
			totalRows++;
		}
	}

	GtkWidget *widget = gtk_table_new(totalRows, totalCols, false);
	//gtk_table_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_table_set_col_spacings(GTK_TABLE(widget), 8);
#endif /* USE_GTK_GRID */
	gtk_widget_show(widget);

	int row = 0, col = 0;
	uint32_t bitfield = field.data.bitfield;
	const auto names_cend = bitfieldDesc.names->cend();
	for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter, bitfield >>= 1) {
		const string &name = *iter;
		if (name.empty())
			continue;

		GtkWidget *checkBox = gtk_check_button_new_with_label(name.c_str());
		gtk_widget_show(checkBox);
		gboolean value = (bitfield & 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkBox), value);

		// Save the bitfield checkbox's value in the GObject.
		g_object_set_data(G_OBJECT(checkBox), "RFT_BITFIELD_value", GUINT_TO_POINTER((guint)value));

		// Disable user modifications.
		// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
		// emitted for both user and program modifications, so we have to
		// connect this signal *after* setting the initial value.
		g_signal_connect(checkBox, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), page);

#if USE_GTK_GRID
		// TODO: GTK_FILL
		gtk_grid_attach(GTK_GRID(widget), checkBox, col, row, 1, 1);
#else /* !USE_GTK_GRID */
		gtk_table_attach(GTK_TABLE(widget), checkBox, col, col+1, row, row+1,
			GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */
		col++;
		if (col == bitfieldDesc.elemsPerRow) {
			row++;
			col = 0;
		}
	}

	page->map_fieldIdx->insert(std::make_pair(fieldIdx, widget));
	return widget;
}

/**
 * Initialize a list data field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_listdata(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// ListData type. Create a GtkListStore for the data.
	const auto &listDataDesc = field.desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	// Single language ListData_t.
	// For RFT_LISTDATA_MULTI, this is only used for row and column count.
	const RomFields::ListData_t *list_data;
	const bool isMulti = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_MULTI);
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
	assert(!list_data->empty());
	if (!list_data || list_data->empty()) {
		// No data...
		return nullptr;
	}

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_ICONS);
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

	GtkListStore *listStore;
	int col_start = 0;
	if (hasCheckboxes) {
		// Prepend an extra column for checkboxes.
		GType *types = new GType[colCount+1];
		types[0] = G_TYPE_BOOLEAN;
		for (int i = colCount; i > 0; i--) {
			types[i] = G_TYPE_STRING;
		}
		listStore = gtk_list_store_newv(colCount+1, types);
		delete[] types;
		col_start = 1;	// Skip the checkbox column for strings.
	} else if (hasIcons) {
		// Prepend an extra column for icons.
		GType *types = new GType[colCount+1];
		types[0] = PIMGTYPE_GOBJECT_TYPE;
		for (int i = colCount; i > 0; i--) {
			types[i] = G_TYPE_STRING;
		}
		listStore = gtk_list_store_newv(colCount+1, types);
		delete[] types;
		col_start = 1;	// Skip the icon column for strings.
	} else {
		// All strings.
		GType *types = new GType[colCount];
		for (int i = colCount-1; i >= 0; i--) {
			types[i] = G_TYPE_STRING;
		}
		listStore = gtk_list_store_newv(colCount, types);
		delete[] types;
	}

	// Add the row data.
	uint32_t checkboxes = 0;
	if (hasCheckboxes) {
		checkboxes = field.data.list_data.mxd.checkboxes;
	}
	unsigned int row = 0;	// for icons [TODO: Use iterator?]
	const auto list_data_cend = list_data->cend();
	for (auto iter = list_data->cbegin(); iter != list_data_cend; ++iter, row++) {
		const vector<string> &data_row = *iter;
		// FIXME: Skip even if we don't have checkboxes?
		// (also check other UI frontends)
		if (hasCheckboxes && data_row.empty()) {
			// Skip this row.
			checkboxes >>= 1;
			continue;
		}

		GtkTreeIter treeIter;
		gtk_list_store_append(listStore, &treeIter);
		if (hasCheckboxes) {
			// Checkbox column.
			gtk_list_store_set(listStore, &treeIter,
				0, (checkboxes & 1), -1);
			checkboxes >>= 1;
		} else if (hasIcons) {
			// Icon column.
			const rp_image *const icon = field.data.list_data.mxd.icons->at(row);
			assert(icon != nullptr);
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
							PIMGTYPE_destroy(pixbuf);
							pixbuf = scaled;
						}
					}
					gtk_list_store_set(listStore, &treeIter,
						0, pixbuf, -1);
					PIMGTYPE_destroy(pixbuf);
				}
			}
		}

		if (!isMulti) {
			int col = col_start;
			const auto data_row_cend = data_row.cend();
			for (auto iter = data_row.cbegin(); iter != data_row_cend; ++iter, col++) {
				gtk_list_store_set(listStore, &treeIter, col, iter->c_str(), -1);
			}
		}
	}

	// Scroll area for the GtkTreeView.
	GtkWidget *widget = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget), GTK_SHADOW_IN);
	gtk_widget_show(widget);

	// Create the GtkTreeView.
	GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView),
		(listDataDesc.names != nullptr));
	gtk_widget_show(treeView);
	gtk_container_add(GTK_CONTAINER(widget), treeView);

	// TODO: Set fixed height mode?
	// May require fixed columns...
	// Reference: https://developer.gnome.org/gtk3/stable/GtkTreeView.html#gtk-tree-view-set-fixed-height-mode

#if GTK_CHECK_VERSION(3,0,0)
	// FIXME: Alternating row colors isn't working in GTK+ 3.x...
#else
	// GTK+ 2.x: Use the "rules hint" for alternating row colors.
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeView), true);
#endif

	if (hasCheckboxes) {
		// Prepend an extra column for checkboxes.
		GtkCellRenderer *const renderer = gtk_cell_renderer_toggle_new();
		GtkTreeViewColumn *const column = gtk_tree_view_column_new_with_attributes(
			"", renderer, "active", 0, nullptr);
		gtk_tree_view_column_set_resizable(column, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
	} else if (hasIcons) {
		// Prepend an extra column for icons.
		GtkCellRenderer *const renderer = gtk_cell_renderer_pixbuf_new();
		GtkTreeViewColumn *const column = gtk_tree_view_column_new_with_attributes(
			"", renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, 0, nullptr);
		gtk_tree_view_column_set_resizable(column, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
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

	// Set up the column names.
	uint32_t align_headers = listDataDesc.alignment.headers;
	uint32_t align_data = listDataDesc.alignment.data;
	for (int i = 0; i < colCount; i++, align_headers >>= 2, align_data >>= 2) {
		// NOTE: Not skipping empty column names.
		// TODO: Hide them.
		GtkCellRenderer *const renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *const column = gtk_tree_view_column_new_with_attributes(
			(listDataDesc.names ? listDataDesc.names->at(i).c_str() : ""),
			renderer, "text", i+col_start, nullptr);
		gtk_tree_view_column_set_resizable(column, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

		// Header alignment
		const float header_xalign = align_tbl_xalign[align_headers & 3];
		// Data alignment
		const float data_xalign = align_tbl_xalign[align_data & 3];
		const PangoAlignment data_alignment =
			static_cast<PangoAlignment>(align_tbl_pango[align_data & 3]);

		g_object_set(column, "alignment", header_xalign, nullptr);
		g_object_set(renderer,
			"xalign", data_xalign,
			"alignment", data_alignment, nullptr);
	}

	// Set a minimum height for the scroll area.
	// TODO: Adjust for DPI, and/or use a font size?
	// TODO: Force maximum horizontal width somehow?
	gtk_widget_set_size_request(widget, -1, 128);

	if (!isMulti) {
		// Resize the columns to fit the contents.
		gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeView));
	}

	// Row height is recalculated when the window is first visible
	// and/or the system theme is changed.
	// TODO: Set an actual default number of rows, or let GTK+ handle it?
	// (Windows uses 5.)
	g_object_set_data(G_OBJECT(treeView), "RFT_LISTDATA_rows_visible",
		GINT_TO_POINTER(listDataDesc.rows_visible));
	if (listDataDesc.rows_visible > 0) {
		g_signal_connect(treeView, "realize", G_CALLBACK(tree_view_realize_signal_handler), page);
	}

	if (isMulti) {
		page->vecListDataMulti->emplace_back(
			Data_ListDataMulti_t(listStore, GTK_TREE_VIEW(treeView), &field));
	}

	page->map_fieldIdx->insert(std::make_pair(fieldIdx, widget));
	return widget;
}

/**
 * Initialize a Date/Time field.
 * @param page	[in] RomDataView object
 * @param field	[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_datetime(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// Date/Time.
	if (field.data.date_time == -1) {
		// tr: Invalid date/time.
		return rom_data_view_init_string(page, field, fieldIdx, C_("RomDataView", "Unknown"));
	}

	GDateTime *dateTime;
	if (field.desc.flags & RomFields::RFT_DATETIME_IS_UTC) {
		dateTime = g_date_time_new_from_unix_utc(field.data.date_time);
	} else {
		dateTime = g_date_time_new_from_unix_local(field.data.date_time);
	}

	static const char *const formats[8] = {
		nullptr,	// No date or time.
		"%x",		// Date
		"%X",		// Time
		"%x %X",	// Date Time

		// TODO: Better localization here.
		nullptr,	// No date or time.
		"%b %d",	// Date (no year)
		"%X",		// Time
		"%b %d %X",	// Date Time (no year)
	};

	const char *const format = formats[field.desc.flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK];
	assert(format != nullptr);
	GtkWidget *widget = nullptr;
	if (format) {
		gchar *const str = g_date_time_format(dateTime, format);
		if (str) {
			widget = rom_data_view_init_string(page, field, fieldIdx, str);
			g_free(str);
		}
	}

	g_date_time_unref(dateTime);
	return widget;
}

/**
 * Initialize an Age Ratings field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_age_ratings(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// Age ratings.
	const RomFields::age_ratings_t *const age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// tr: No age ratings data.
		return rom_data_view_init_string(page, field, fieldIdx, C_("RomDataView", "ERROR"));
	}

	// Convert the age ratings field to a string.
	string str = RomFields::ageRatingsDecode(age_ratings);
	return rom_data_view_init_string(page, field, fieldIdx, str.c_str());
}

/**
 * Initialize a Dimensions field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_dimensions(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// Dimensions.
	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field.data.dimensions;
	char buf[64];
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			snprintf(buf, sizeof(buf), "%dx%dx%d",
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			snprintf(buf, sizeof(buf), "%dx%d",
				dimensions[0], dimensions[1]);
		}
	} else {
		snprintf(buf, sizeof(buf), "%d", dimensions[0]);
	}

	return rom_data_view_init_string(page, field, fieldIdx, buf);
}

/**
 * Initialize a multi-language string field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rom_data_view_init_string_multi(RomDataView *page,
	const RomFields::Field &field, int fieldIdx)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	GtkWidget *const lblStringMulti = rom_data_view_init_string(page, field, fieldIdx, "");
	if (lblStringMulti) {
		page->vecStringMulti->emplace_back(lblStringMulti, &field);
	}
	return lblStringMulti;
}

/**
 * Update the cboLanguage images.
 * @param page		[in] RomDataView object.
 */
static void
rom_data_view_update_cboLanguage_images(RomDataView *page)
{
	// TODO:
	// - High-DPI scaling on GTK+ earlier than 3.10
	// - Fractional scaling
	// - Runtime adjustment via "configure" event
	// Reference: https://developer.gnome.org/gdk3/stable/gdk3-Windows.html#gdk-window-get-scale-factor
	unsigned int iconSize = 16;
#if GTK_CHECK_VERSION(3,10,0)
# if 0
	// FIXME: gtk_widget_get_window() doesn't work unless the window is realized.
	// We might need to initialize the dropdown in the "realize" signal handler.
	GdkWindow *const gdk_window = gtk_widget_get_window(GTK_WIDGET(page));
	assert(gdk_window != nullptr);
	if (gdk_window) {
		const gint scale_factor = gdk_window_get_scale_factor(gdk_window);
		if (scale_factor >= 2) {
			// 2x scaling or higher.
			// TODO: Larger icon sizes?
			iconSize = 32;
		}
	}
# endif /* 0 */
#endif /* GTK_CHECK_VERSION(3,10,0) */
	char flags_filename[64];
	snprintf(flags_filename, sizeof(flags_filename),
		"/com/gerbilsoft/rom-properties/flags/flags-%ux%u.png",
		iconSize, iconSize);
	PIMGTYPE flags_spriteSheet = PIMGTYPE_load_png_from_gresource(flags_filename);
	assert(flags_spriteSheet != nullptr);
	if (!flags_spriteSheet) {
		// Unable to load the flags sprite sheet.
		return;
	}

	GtkTreeIter treeIter;
	gboolean ok = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(page->lstoreLanguage), &treeIter);
	while (ok) {
		uint32_t lc = 0;
		int col, row;

		gtk_tree_model_get(GTK_TREE_MODEL(page->lstoreLanguage), &treeIter, SM_COL_LC, &lc, -1);
		if (!SystemRegion::getFlagPosition(lc, &col, &row)) {
			// Found a matching icon.
			PIMGTYPE icon = PIMGTYPE_get_subsurface(flags_spriteSheet,
				col*iconSize, row*iconSize, iconSize, iconSize);
			gtk_list_store_set(page->lstoreLanguage, &treeIter, SM_COL_ICON, icon, -1);
			PIMGTYPE_destroy(icon);
		}

		ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(page->lstoreLanguage), &treeIter);
	};

	// We're done using the flags sprite sheets,
	// so unreference them to prevent memory leaks.
	PIMGTYPE_destroy(flags_spriteSheet);
}

/**
 * Update all multi-language fields.
 * @param page		[in] RomDataView object.
 * @param user_lc	[in] User-specified language code.
 */
static void
rom_data_view_update_multi(RomDataView *page, uint32_t user_lc)
{
	// RFT_STRING_MULTI
	const auto vecStringMulti_cend = page->vecStringMulti->cend();
	for (auto iter = page->vecStringMulti->cbegin();
	     iter != vecStringMulti_cend; ++iter)
	{
		GtkWidget *const lblString = iter->first;
		const RomFields::Field *const pField = iter->second;
		const auto *const pStr_multi = pField->data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (!pStr_multi || pStr_multi->empty()) {
			// Invalid multi-string...
			continue;
		}

		if (!page->cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			const auto pStr_multi_cend = pStr_multi->cend();
			for (auto iter_sm = pStr_multi->cbegin();
			     iter_sm != pStr_multi_cend; ++iter_sm)
			{
				page->set_lc->insert(iter_sm->first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, page->def_lc, user_lc);
		assert(pStr != nullptr);
		gtk_label_set_text(GTK_LABEL(lblString), (pStr ? pStr->c_str() : ""));
	}

	// RFT_LISTDATA_MULTI
	const auto vecListDataMulti_cend = page->vecListDataMulti->cend();
	for (auto iter = page->vecListDataMulti->cbegin();
	     iter != vecListDataMulti_cend; ++iter)
	{
		GtkListStore *const listStore = iter->listStore;
		const RomFields::Field *const pField = iter->field;
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
			const auto pListData_multi_cend = pListData_multi->cend();
			for (auto iter_sm = pListData_multi->cbegin();
			     iter_sm != pListData_multi_cend; ++iter_sm)
			{
				page->set_lc->insert(iter_sm->first);
			}
		}

		// Get the ListData_t.
		const auto *const pListData = RomFields::getFromListDataMulti(pListData_multi, page->def_lc, user_lc);
		assert(pListData != nullptr);
		if (pListData != nullptr) {
			const auto &listDataDesc = pField->desc.list_data;

			// If we have checkboxes or icons, start at column 1.
			// Otherwise, start at column 0.
			int col_start;
			if (listDataDesc.flags & (RomFields::RFT_LISTDATA_CHECKBOXES | RomFields::RFT_LISTDATA_ICONS)) {
				// Checkboxes and/or icons are present.
				col_start = 1;
			} else {
				col_start = 0;
			}

			// Update the list.
			GtkTreeIter treeIter;
			gboolean ok = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(listStore), &treeIter);
			auto iter_listData = pListData->cbegin();
			const auto pListData_cend = pListData->cend();
			while (ok && iter_listData != pListData_cend) {
				// TODO: Verify GtkListStore column count?
				int col = col_start;
				const auto iter_listData_cend = iter_listData->cend();
				for (auto iter_row = iter_listData->cbegin();
				     iter_row != iter_listData_cend; ++iter_row, col++)
				{
					gtk_list_store_set(listStore, &treeIter, col, iter_row->c_str(), -1);
				}

				// Next row.
				++iter_listData;
				ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(listStore), &treeIter);
			}

			// Resize the columns to fit the contents.
			// NOTE: Only done on first load.
			if (!page->cboLanguage) {
				gtk_tree_view_columns_autosize(GTK_TREE_VIEW(iter->treeView));
			}
		}
	}

	if (!page->cboLanguage && page->set_lc->size() > 1) {
		// Create the language combobox.
		// Columns:
		// - 0: Icon
		// - 1: Display text
		// - 2: Language code
		page->lstoreLanguage = gtk_list_store_new(3, PIMGTYPE_GOBJECT_TYPE, G_TYPE_STRING, G_TYPE_UINT);

		int sel_idx = -1;
		const auto set_lc_cend = page->set_lc->cend();
		for (auto iter = page->set_lc->cbegin(); iter != set_lc_cend; ++iter) {
			const uint32_t lc = *iter;
			const char *const name = SystemRegion::getLocalizedLanguageName(lc);

			GtkTreeIter treeIter;
			gtk_list_store_append(page->lstoreLanguage, &treeIter);
			gtk_list_store_set(page->lstoreLanguage, &treeIter, SM_COL_ICON, nullptr, SM_COL_LC, lc, -1);
			if (name) {
				gtk_list_store_set(page->lstoreLanguage, &treeIter, SM_COL_TEXT, name, -1);
			} else {
				string s_lc;
				s_lc.reserve(4);
				for (uint32_t tmp_lc = lc; tmp_lc != 0; tmp_lc <<= 8) {
					char chr = (char)(tmp_lc >> 24);
					if (chr != 0) {
						s_lc += chr;
					}
				}
				gtk_list_store_set(page->lstoreLanguage, &treeIter, SM_COL_TEXT, s_lc.c_str(), -1);
			}
			int cur_idx = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(page->lstoreLanguage), nullptr)-1;

			// Save the default index:
			// - ROM-default language code.
			// - English if it's not available.
			if (lc == page->def_lc) {
				// Select this item.
				sel_idx = cur_idx;
			} else if (lc == 'en') {
				// English. Select this item if def_lc hasn't been found yet.
				if (sel_idx < 0) {
					sel_idx = cur_idx;
				}
			}
		}

		// Initialize the images.
		rom_data_view_update_cboLanguage_images(page);

		// Create a VBox for the combobox to reduce its vertical height.
#if GTK_CHECK_VERSION(3,0,0)
		GtkWidget *const vboxCboLanguage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_valign(vboxCboLanguage, GTK_ALIGN_START);
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), vboxCboLanguage, false, false, 0);
		gtk_widget_show(vboxCboLanguage);
#else /* !GTK_CHECK_VERSION(3,0,0) */
		GtkWidget *const topAlign = gtk_alignment_new(0.5f, 0.0f, 0.0f, 0.0f);
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), topAlign, false, false, 0);
		gtk_widget_show(topAlign);

		GtkWidget *const vboxCboLanguage = gtk_vbox_new(false, 0);
		gtk_container_add(GTK_CONTAINER(topAlign), vboxCboLanguage);
		gtk_widget_show(vboxCboLanguage);
#endif /* GTK_CHECK_VERSION(3,0,0) */

		// Create the combobox and set the list store.
		page->cboLanguage = gtk_combo_box_new_with_model(GTK_TREE_MODEL(page->lstoreLanguage));
		g_object_unref(page->lstoreLanguage);	// remove our reference
		gtk_box_pack_end(GTK_BOX(vboxCboLanguage), page->cboLanguage, false, false, 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(page->cboLanguage), sel_idx);
		gtk_widget_show(page->cboLanguage);

		// cboLanguage: Icon renderer
		GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(page->cboLanguage), renderer, false);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(page->cboLanguage),
			renderer, GTK_CELL_RENDERER_PIXBUF_PROPERTY, SM_COL_ICON, NULL);

		// cboLanguage: Text renderer
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(page->cboLanguage), renderer, true);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(page->cboLanguage),
			renderer, "text", SM_COL_TEXT, NULL);

		// Connect the "changed" signal.
		g_signal_connect(page->cboLanguage, "changed", G_CALLBACK(cboLanguage_changed_signal_handler), page);
	}
}

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param page		[in] RomDataView object.
 * @param fieldIdx	[in] Field index.
 * @return 0 on success; non-zero on error.
 */
static int
rom_data_view_update_field(RomDataView *page, int fieldIdx)
{
	const RomFields *const pFields = page->romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return 1;
	}

	assert(fieldIdx >= 0);
	assert(fieldIdx < pFields->count());
	if (fieldIdx < 0 || fieldIdx >= pFields->count())
		return 2;

	const RomFields::Field *const field = pFields->at(fieldIdx);
	assert(field != nullptr);
	if (!field)
		return 3;

	// Get the GtkWidget*.
	auto iter = page->map_fieldIdx->find(fieldIdx);
	if (iter == page->map_fieldIdx->end()) {
		// Not found.
		return 4;
	}
	GtkWidget *const widget = iter->second;

	// Update the value widget(s).
	int ret;
	switch (field->type) {
		case RomFields::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RFT_STRING: {
			// GtkWidget is a GtkLabel.
			assert(GTK_IS_LABEL(widget));
			if (!GTK_IS_LABEL(widget)) {
				ret = 7;
				break;
			}

			gtk_label_set_text(GTK_LABEL(widget), field->data.str
				? field->data.str->c_str()
				: nullptr);
			ret = 0;
			break;
		}

		case RomFields::RFT_BITFIELD: {
			// GtkWidget is a GtkGrid/GtkTable GtkCheckButton widgets.
#ifdef USE_GTK_GRID
			assert(GTK_IS_GRID(widget));
			if (!GTK_IS_GRID(widget)) {
				ret = 8;
				break;
			}
#else /* !USE_GTK_GRID */
			assert(GTK_IS_TABLE(widget));
			if (!GTK_IS_TABLE(widget)) {
				ret = 8;
				break;
			}
#endif /* USE_GTK_GRID */

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;
			int count = (int)bitfieldDesc.names->size();
			assert(count <= 32);
			if (count > 32)
				count = 32;

			GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(widget));
			if (!widgetList) {
				ret = 9;
				break;
			}
			// NOTE: Widgets are enumerated in reverse.
			GList *checkBoxIter = g_list_last(widgetList);

			// Inhibit the "no-toggle" signal while updating.
			page->inhibit_checkbox_no_toggle = true;

			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend && checkBoxIter != nullptr;
			     ++iter, checkBoxIter = checkBoxIter->prev, bitfield >>= 1)
			{
				const string &name = *iter;
				if (name.empty())
					continue;

				GtkWidget *const checkBox = GTK_WIDGET(checkBoxIter->data);
				assert(GTK_IS_CHECK_BUTTON(checkBox));
				if (!GTK_IS_CHECK_BUTTON(checkBox))
					break;

				const bool value = (bitfield & 1);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkBox), value);
				g_object_set_data(G_OBJECT(checkBox), "RFT_BITFIELD_value", GUINT_TO_POINTER((guint)value));
			}
			g_list_free(widgetList);

			// Done updating.
			page->inhibit_checkbox_no_toggle = false;
			ret = 0;
			break;
		}
	}

	return ret;
}

static string
convert_accel_to_gtk(const char *str)
{
	// GTK+ uses '_' for accelerators, not '&'.
	string s_ret = str;
	size_t accel_pos = s_ret.find('&');
	if (accel_pos != string::npos) {
		s_ret[accel_pos] = '_';
	}
	return s_ret;
}

static void
rom_data_view_create_options_button(RomDataView *page)
{
	assert(!page->btnOptions);
	if (page->btnOptions != nullptr)
		return;

	GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(page));
	if (page->desc_format_type == RP_DFT_XFCE) {
		// On XFCE, the first parent is ThunarxPropertyPage.
		// On GNOME, this is skipped and we go directly to GtkNotebook.
		parent = gtk_widget_get_parent(parent);
	}

	// On GNOME, the first parent is GtkNotebook.
	assert(GTK_IS_NOTEBOOK(parent));
	if (!GTK_IS_NOTEBOOK(parent))
		return;

	// Next: GtkBox (or GtkVBox for GTK+ 2.x).
	parent = gtk_widget_get_parent(parent);
#if GTK_CHECK_VERSION(3,0,0)
	assert(GTK_IS_BOX(parent) || GTK_IS_VBOX(parent));
	if (!GTK_IS_BOX(parent) && !GTK_IS_VBOX(parent))
		return;
#else /* !GTK_CHECK_VERSION(3,0,0) */
	assert(GTK_IS_VBOX(parent));
	if (!GTK_IS_VBOX(parent))
		return;
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Next: GtkDialog subclass.
	// - XFCE: ThunarPropertiesDialog
	// - Nautilus: NautilusPropertiesWindow
	// - Caja: FMPropertiesWindow
	// - Nemo: NemoPropertiesWindow
	parent = gtk_widget_get_parent(parent);
	assert(GTK_IS_DIALOG(parent));
	if (!GTK_IS_DIALOG(parent))
		return;

	string s_title = convert_accel_to_gtk(C_("RomDataView", "Op&tions"));

	// Add an "Options" button.
	GtkWidget *const lblOptions = gtk_label_new(nullptr);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(lblOptions), s_title.c_str());
	gtk_widget_show(lblOptions);
	GtkWidget *const imgOptions = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(imgOptions);
#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *const hboxOptions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else /* !GTK_CHECK_VERSION(3,0,0) */
	GtkWidget *const hboxOptions = gtk_hbox_new(false, 4);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_widget_show(hboxOptions);
	gtk_box_pack_start(GTK_BOX(hboxOptions), lblOptions, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxOptions), imgOptions, false, false, 0);
#ifdef USE_GTK_MENU_BUTTON
	// GTK+ 3.6 has GtkMenuButton.
	// NOTE: GTK+ 4.x's GtkMenuButton supports both label and image.
	// GTK+ 3.x's only supports image by default, so we'll have to
	// build a GtkBox with both label and image.
	page->btnOptions = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(page->btnOptions), GTK_ARROW_UP);
#else /* !USE_GTK_MENU_BUTTON */
	// Use plain old GtkButton.
	// TODO: Menu functionality.
	page->btnOptions = gtk_button_new();
#endif /* USE_GTK_MENU_BUTTON */
	gtk_container_add(GTK_CONTAINER(page->btnOptions), hboxOptions);

	// TODO: Connect the signal.
	// TODO: Submenu.
	gtk_dialog_add_action_widget(GTK_DIALOG(parent), page->btnOptions, GTK_RESPONSE_NONE);
	gtk_widget_hide(page->btnOptions);
	assert(page->btnOptions != nullptr);
	if (!page->btnOptions)
		return;

	// Disconnect the "clicked" signal from the default GtkDialog response handler.
	guint signal_id = g_signal_lookup("clicked", GTK_TYPE_BUTTON);
	gulong handler_id = g_signal_handler_find(page->btnOptions, G_SIGNAL_MATCH_ID,
		signal_id, 0, nullptr, 0, 0);
	g_signal_handler_disconnect(page->btnOptions, handler_id);

#ifndef USE_GTK_MENU_BUTTON
	// Connect the "event" signal for the GtkButton.
	// NOTE: We need to pass the event details. Otherwise, we'll
	// end up with the menu getting "stuck" to the mouse.
	// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
	g_object_set_data(G_OBJECT(page->btnOptions), "RomDataView", page);
	g_signal_connect(page->btnOptions, "event", G_CALLBACK(btnOptions_clicked_signal_handler), page);
#endif /* !USE_GTK_MENU_BUTTON */

#if GTK_CHECK_VERSION(3,12,0)
	GtkWidget *const headerBar = gtk_dialog_get_header_bar(GTK_DIALOG(parent));
	if (headerBar) {
		// Dialog has a Header Bar instead of an action area.
		// No reordering is necessary.

		// Change the arrow to point down instead of up.
		gtk_image_set_from_icon_name(GTK_IMAGE(imgOptions), "pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_menu_button_set_direction(GTK_MENU_BUTTON(page->btnOptions), GTK_ARROW_DOWN);
	} else
#endif /* GTK_CHECK_VERSION(3,12,0) */
	{
		// Reorder the "Options" button so it's to the right of "Help".
		// NOTE: GTK+ 3.10 introduced the GtkHeaderBar, but
		// gtk_dialog_get_header_bar() was added in GTK+ 3.12.
		// Hence, this might not work properly on GTK+ 3.10.
		GtkWidget *const btnBox = gtk_widget_get_parent(page->btnOptions);
		//assert(GTK_IS_BUTTON_BOX(btnBox));
		if (GTK_IS_BUTTON_BOX(btnBox))
		{
			gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(btnBox), page->btnOptions, TRUE);
		}
	}

	// Create the menu.
	// TODO: Switch to GMenu? (glib-2.32)
	page->menuOptions = gtk_menu_new();
#ifdef USE_GTK_MENU_BUTTON
	// NOTE: GtkMenuButton takes ownership of the menu.
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(page->btnOptions), page->menuOptions);
#endif /* USE_GTK_MENU_BUTTON */

	/** Standard actions. **/
	static const struct {
		const char *desc;
		int id;
	} stdacts[] = {
		{NOP_C_("RomDataView|Options", "Export to Text"),	OPTION_EXPORT_TEXT},
		{NOP_C_("RomDataView|Options", "Export to JSON"),	OPTION_EXPORT_JSON},
		{NOP_C_("RomDataView|Options", "Copy as Text"),		OPTION_COPY_TEXT},
		{NOP_C_("RomDataView|Options", "Copy as JSON"),		OPTION_COPY_JSON},
		{nullptr, 0}
	};

	for (const auto *p = stdacts; p->desc != nullptr; p++) {
		GtkWidget *const menuItem = gtk_menu_item_new_with_label(
			dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p->desc));
		g_object_set_data(G_OBJECT(menuItem), "menuOptions_id", GINT_TO_POINTER(p->id));
		g_signal_connect(menuItem, "activate", G_CALLBACK(menuOptions_triggered_signal_handler), page);
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(page->menuOptions), menuItem);
	}

	/** ROM operations. **/
	const vector<RomData::RomOps> ops = page->romData->romOps();
	if (!ops.empty()) {
		// FIXME: Not showing up properly with the KDE Breeze theme,
		// though other separators *do* show up...
		GtkWidget *menuItem = gtk_separator_menu_item_new();
		gtk_widget_show(menuItem);
		gtk_menu_shell_append(GTK_MENU_SHELL(page->menuOptions), menuItem);

		int i = 0;
		const auto ops_end = ops.cend();
		for (auto iter = ops.cbegin(); iter != ops_end; ++iter, i++) {
			const string desc = convert_accel_to_gtk(iter->desc.c_str());
			menuItem = gtk_menu_item_new_with_mnemonic(desc.c_str());
			gtk_widget_set_sensitive(menuItem, !!(iter->flags & RomData::RomOps::ROF_ENABLED));
			g_object_set_data(G_OBJECT(menuItem), "menuOptions_id", GINT_TO_POINTER(i));
			g_signal_connect(menuItem, "activate", G_CALLBACK(menuOptions_triggered_signal_handler), page);
			gtk_widget_show(menuItem);
			gtk_menu_shell_append(GTK_MENU_SHELL(page->menuOptions), menuItem);
		}
	}
}

static void
rom_data_view_update_display(RomDataView *page)
{
	assert(page != nullptr);

	// Delete the icon frames and tabs.
	rom_data_view_delete_tabs(page);

	// Create the "Options" button.
	if (!page->btnOptions) {
		rom_data_view_create_options_button(page);
	}

	// Initialize the header row.
	rom_data_view_init_header_row(page);

	if (!page->romData) {
		// No ROM data...
		return;
	}

	// Get the fields.
	const RomFields *const pFields = page->romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = pFields->count();
#if !GTK_CHECK_VERSION(3,0,0)
	int rowCount = count;
#endif

	// Create the GtkNotebook.
	int tabCount = pFields->tabCount();
	if (tabCount > 1) {
		page->tabs->resize(tabCount);
		page->tabWidget = gtk_notebook_new();

		// Add spacing between the system info header and the table.
		g_object_set(page, "spacing", 8, nullptr);

		auto tabIter = page->tabs->begin();
		for (int i = 0; i < tabCount; i++, ++tabIter) {
			// Create a tab.
			const char *name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = *tabIter;
#if USE_GTK_GRID
			tab.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
			tab.table = gtk_grid_new();
			gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
			gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else /* !USE_GTK_GRID */
			tab.vbox = gtk_vbox_new(false, 8);
			// TODO: Adjust the table size?
			tab.table = gtk_table_new(rowCount, 2, false);
			gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
			gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif /* USE_GTK_GRID */

			gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
			gtk_box_pack_start(GTK_BOX(tab.vbox), tab.table, false, false, 0);
			gtk_widget_show(tab.table);
			gtk_widget_show(tab.vbox);

			// Add the tab.
			GtkWidget *label = gtk_label_new(name);
			gtk_notebook_append_page(GTK_NOTEBOOK(page->tabWidget), tab.vbox, label);
		}
		gtk_box_pack_start(GTK_BOX(page), page->tabWidget, true, true, 0);
		gtk_widget_show(page->tabWidget);
	} else {
		// No tabs.
		// Don't create a GtkNotebook, but simulate a single
		// tab in page->tabs[] to make it easier to work with.
		tabCount = 1;
		page->tabs->resize(1);
		auto &tab = page->tabs->at(0);
		tab.vbox = GTK_WIDGET(page);

#if USE_GTK_GRID
		tab.table = gtk_grid_new();
		gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
		gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else /* !USE_GTK_GRID */
		// TODO: Adjust the table size?
		tab.table = gtk_table_new(count, 2, false);
		gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
		gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif /* USE_GTK_GRID */

		gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
		gtk_box_pack_start(GTK_BOX(page), tab.table, false, false, 0);
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
	GtkSizeGroup *const size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");

	// Create the data widgets.
	const auto pFields_cend = pFields->cend();
	int fieldIdx = 0;
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, fieldIdx++) {
		const RomFields::Field &field = *iter;
		if (!field.isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)page->tabs->size());
		if (tabIdx < 0 || tabIdx >= (int)page->tabs->size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!page->tabs->at(tabIdx).table) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		GtkWidget *widget = nullptr;
		bool separate_rows = false;
		switch (field.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				break;
			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;

			case RomFields::RFT_STRING:
				widget = rom_data_view_init_string(page, field, fieldIdx);
				break;
			case RomFields::RFT_BITFIELD:
				widget = rom_data_view_init_bitfield(page, field, fieldIdx);
				break;
			case RomFields::RFT_LISTDATA:
				separate_rows = !!(field.desc.list_data.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW);
				widget = rom_data_view_init_listdata(page, field, fieldIdx);
				break;
			case RomFields::RFT_DATETIME:
				widget = rom_data_view_init_datetime(page, field, fieldIdx);
				break;
			case RomFields::RFT_AGE_RATINGS:
				widget = rom_data_view_init_age_ratings(page, field, fieldIdx);
				break;
			case RomFields::RFT_DIMENSIONS:
				widget = rom_data_view_init_dimensions(page, field, fieldIdx);
				break;
			case RomFields::RFT_STRING_MULTI:
				widget = rom_data_view_init_string_multi(page, field, fieldIdx);
				break;
		}

		if (widget) {
			// Add the widget to the table.
			auto &tab = page->tabs->at(tabIdx);

			// tr: Field description label.
			const string txt = rp_sprintf(desc_label_fmt, field.name.c_str());
			GtkWidget *lblDesc = gtk_label_new(txt.c_str());
			gtk_label_set_use_underline(GTK_LABEL(lblDesc), false);
			gtk_widget_show(lblDesc);
			gtk_size_group_add_widget(size_group, lblDesc);
			page->vecDescLabels->emplace_back(lblDesc);

			// Check if this is an RFT_STRING with warning set.
			// If it is, set the "RFT_STRING_warning" flag.
			const gboolean is_warning = (field.type == RomFields::RFT_STRING &&
						     field.desc.flags & RomFields::STRF_WARNING);
			g_object_set_data(G_OBJECT(lblDesc), "RFT_STRING_warning", GUINT_TO_POINTER((guint)is_warning));

			// Set the label format type.
			set_label_format_type(GTK_LABEL(lblDesc), page->desc_format_type);

			// Value widget.
			int &row = tabRowCount[tabIdx];
#if USE_GTK_GRID
			// TODO: GTK_FILL
			gtk_grid_attach(GTK_GRID(tab.table), lblDesc, 0, row, 1, 1);
			// Widget halign is set above.
			gtk_widget_set_valign(widget, GTK_ALIGN_START);
#else /* !USE_GTK_GRID */
			gtk_table_attach(GTK_TABLE(tab.table), lblDesc, 0, 1, row, row+1,
				GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */

			if (separate_rows) {
				// Separate rows.
				// Make sure the description label is left-aligned.
#if GTK_CHECK_VERSION(3,16,0)
				gtk_label_set_xalign(GTK_LABEL(lblDesc), 0.0);
#else /* !GTK_CHECK_VERSION(3,16,0) */
				gtk_misc_set_alignment(GTK_MISC(lblDesc), 0.0f, 0.0f);
#endif /* GTK_CHECK_VERSION(3,16,0) */

				// If this is the last field in the tab,
				// put the RFT_LISTDATA in the GtkGrid instead.
				bool doVBox = false;
				RomFields::const_iterator nextIter = iter;
				++nextIter;
				if (tabIdx + 1 == tabCount && (nextIter == pFields_cend)) {
					// Last tab, and last field.
					doVBox = true;
				} else {
					// Check if the next field is on the next tab.
					const RomFields::Field &nextField = *nextIter;
					if (nextField.tabIdx != tabIdx) {
						// Next field is on the next tab.
						doVBox = true;
					}
				}

				if (doVBox) {
					// FIXME: There still seems to be a good amount of space
					// between tab.vbox and the RFT_LISTDATA widget here...
					// (Moreso on Thunar GTK2 than Nautilus.)

					// Unset this property to prevent the event filter from
					// setting a fixed height.
					g_object_set_data(G_OBJECT(widget), "RFT_LISTDATA_rows_visible",
						GINT_TO_POINTER(0));

#if USE_GTK_GRID
					// Change valign to FILL.
					gtk_widget_set_valign(widget, GTK_ALIGN_FILL);

					// Set margin, since it's located outside of the GtkTable/GtkGrid.
					// NOTE: Setting top margin to 0 due to spacing from the
					// GtkTable/GtkGrid. (Still has extra spacing that needs to be fixed...)
					g_object_set(widget,
						"margin-left", 8, "margin-right",  8,
						"margin-top",  0, "margin-bottom", 8,
						nullptr);

					// Add the widget to the GtkBox.
					gtk_box_pack_start(GTK_BOX(tab.vbox), widget, true, true, 0);
					if (tab.lblCredits) {
						// Need to move it before credits.
						// TODO: Verify this.
						gtk_box_reorder_child(GTK_BOX(tab.vbox), widget, 1);
					}
#else /* !USE_GTK_GRID */
					// Need to use GtkAlignment on GTK+ 2.x.
					GtkWidget *const alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
					gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 8, 8, 8);
					gtk_container_add(GTK_CONTAINER(alignment), widget);
					gtk_widget_show(alignment);

					// Add the GtkAlignment to the GtkBox.
					gtk_box_pack_start(GTK_BOX(tab.vbox), alignment, true, true, 0);
					if (tab.lblCredits) {
						// Need to move it before credits.
						// TODO: Verify this.
						gtk_box_reorder_child(GTK_BOX(tab.vbox), alignment, 1);
					}
#endif /* USE_GTK_GRID */
					// Increment row by one, since only one widget is
					// actually being added to the GtkTable/GtkGrid.
					row++;
				} else {
					// Add the widget to the GtkTable/GtkGrid.
#if USE_GTK_GRID
					gtk_grid_attach(GTK_GRID(tab.table), widget, 0, row+1, 2, 1);
#else /* !USE_GTK_GRID */
					rowCount++;
					gtk_table_resize(GTK_TABLE(tab.table), rowCount, 2);
					gtk_table_attach(GTK_TABLE(tab.table), widget, 0, 2, row+1, row+2,
						GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */
					row += 2;
				}
			} else {
				// Single row.
#if USE_GTK_GRID
				gtk_grid_attach(GTK_GRID(tab.table), widget, 1, row, 1, 1);
#else /* !USE_GTK_GRID */
				gtk_table_attach(GTK_TABLE(tab.table), widget, 1, 2, row, row+1,
					GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */
				row++;
			}
		}
	}

	// Initial update of RFT_STRING_MULTI and RFT_LISTDATA_MULTI fields.
	if (!page->vecStringMulti->empty() || !page->vecListDataMulti->empty()) {
		page->def_lc = pFields->defaultLanguageCode();
		rom_data_view_update_multi(page, 0);
	}
}

static gboolean
rom_data_view_load_rom_data(gpointer data)
{
	RomDataView *const page = ROM_DATA_VIEW(data);
	g_return_val_if_fail(page != nullptr || IS_ROM_DATA_VIEW(page), G_SOURCE_REMOVE);

	if (G_UNLIKELY(page->uri == nullptr)) {
		// No URI.
		// TODO: Remove widgets?
		page->changed_idle = 0;
		return G_SOURCE_REMOVE;
	}

	// Check if the URI maps to a local file.
	IRpFile *file = nullptr;
	gchar *const filename = g_filename_from_uri(page->uri, nullptr, nullptr);
	if (filename) {
		// Local file. Use RpFile.
		file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
		g_free(filename);
	} else {
		// Not a local file. Use RpFileGio.
		file = new RpFileGio(page->uri);
	}

	if (file->isOpen()) {
		// Create the RomData object.
		// file is ref()'d by RomData.
		RomData *const romData = RomDataFactory::create(file);
		if (romData != page->romData) {
			page->romData = romData;
			g_object_notify_by_pspec(G_OBJECT(page), properties[PROP_SHOWING_DATA]);
		}

		// Update the display widgets.
		rom_data_view_update_display(page);

		// Make sure the underlying file handle is closed,
		// since we don't need it once the RomData has been
		// loaded by RomDataView.
		if (romData) {
			romData->close();
		}
	}
	file->unref();

	// Animation timer will be started when the page
	// receives the "map" signal.

	// Clear the timeout.
	page->changed_idle = 0;
	return G_SOURCE_REMOVE;
}

/**
 * Delete tabs and related widgets.
 * @param page RomDataView.
 */
static void
rom_data_view_delete_tabs(RomDataView *page)
{
	assert(page != nullptr);
	assert(page->tabs != nullptr);
	assert(page->map_fieldIdx != nullptr);
	assert(page->vecDescLabels != nullptr);
	assert(page->vecStringMulti != nullptr);
	assert(page->vecListDataMulti != nullptr);

	// Delete the tab contents.
	std::for_each(page->tabs->begin(), page->tabs->end(),
		[page](_RomDataView::tab &tab) {
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
	);
	page->tabs->clear();
	page->map_fieldIdx->clear();

	if (page->tabWidget) {
		// Delete the tab widget.
		gtk_widget_destroy(page->tabWidget);
		page->tabWidget = nullptr;
	}

	// Delete the language dropdown and related.
	// NOTE: We released our reference to lstoreLanguage,
	// so destroying cboLanguage will destroy lstoreLanguage.
	if (page->cboLanguage) {
		gtk_widget_destroy(page->cboLanguage);
		page->cboLanguage = nullptr;
		page->lstoreLanguage = nullptr;
	}

	// Clear the various widget references.
	page->vecDescLabels->clear();
	page->set_lc->clear();
	page->vecStringMulti->clear();
	page->vecListDataMulti->clear();
}

/** Signal handlers **/

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param togglebutton Bitfield checkbox.
 * @param user_data RomDataView*.
 */
static void
checkbox_no_toggle_signal_handler(GtkToggleButton	*togglebutton,
				  gpointer		 user_data)
{
	RomDataView *const page = ROM_DATA_VIEW(user_data);
	if (page->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved RFT_BITFIELD value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(togglebutton), "RFT_BITFIELD_value"));
	if (gtk_toggle_button_get_active(togglebutton) != value) {
		// Toggle this box.
		gtk_toggle_button_set_active(togglebutton, value);
	}
}

/**
 * RomDataView is being mapped onto the screen.
 * @param page RomDataView
 * @param user_data User data.
 */
static void
rom_data_view_map_signal_handler(RomDataView	*page,
				 gpointer	 user_data)
{
	RP_UNUSED(user_data);
	drag_image_start_anim_timer(DRAG_IMAGE(page->imgIcon));
	if (page->btnOptions) {
		gtk_widget_show(page->btnOptions);
	}
}

/**
 * RomDataView is being unmapped from the screen.
 * @param page RomDataView
 * @param user_data User data.
 */
static void
rom_data_view_unmap_signal_handler(RomDataView	*page,
				   gpointer	 user_data)
{
	RP_UNUSED(user_data);
	drag_image_stop_anim_timer(DRAG_IMAGE(page->imgIcon));
	if (page->btnOptions) {
		gtk_widget_hide(page->btnOptions);
	}
}

/**
 * GtkTreeView widget has been realized.
 * @param treeView GtkTreeView
 * @param page RomDataView
 */
static void
tree_view_realize_signal_handler(GtkTreeView	*treeView,
				 RomDataView	*page)
{
	// TODO: Redo this if the system font and/or style changes.
	// TODO: Remove the `page` parameter?
	RP_UNUSED(page);

	// Recalculate the row heights for this GtkTreeView.
	const int rows_visible = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(treeView), "RFT_LISTDATA_rows_visible"));
	if (rows_visible <= 0) {
		// This GtkTreeView doesn't have a fixed number of rows.
		return;
	}

	// Get the parent widget.
	// This should be a GtkScrolledWindow.
	GtkWidget *scrolledWindow = gtk_widget_get_ancestor(GTK_WIDGET(treeView), GTK_TYPE_SCROLLED_WINDOW);
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
		GtkTreeViewColumn *column = gtk_tree_view_get_column(treeView, 0);
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
	gtk_style_context_get_border(context, GTK_STATE_FLAG_NORMAL, &border);
	gtk_style_context_get_padding(context, GTK_STATE_FLAG_NORMAL, &padding);
	gtk_style_context_get_margin(context, GTK_STATE_FLAG_NORMAL, &margin);
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
 * Get the selected language code.
 * @param page RomDataView
 * @return Selected language code, or 0 for none (default).
 */
static uint32_t
rom_data_view_get_sel_lc(RomDataView *page)
{
	if (!page->cboLanguage) {
		// No language dropdown...
		return 0;
	}

	const gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(page->cboLanguage));
	if (index < 0) {
		// Invalid index...
		return 0;
	}

	// Get the language code from the GtkListStore.
	GtkTreeIter iter;
	GtkTreePath *const path = gtk_tree_path_new_from_indices(index, -1);
	gboolean success = gtk_tree_model_get_iter(
		GTK_TREE_MODEL(page->lstoreLanguage), &iter, path);
	gtk_tree_path_free(path);
	assert(success);
	if (!success) {
		// Shouldn't happen...
		return 0;
	}

	uint32_t lc = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(page->lstoreLanguage), &iter, SM_COL_LC, &lc, -1);
	return lc;
}

/**
 * The RFT_MULTI_STRING language was changed.
 * @param widget	GtkComboBox
 * @param user_data	RomDataView
 */
static void
cboLanguage_changed_signal_handler(GtkComboBox *widget,
				   gpointer     user_data)
{
	RP_UNUSED(widget);
	RomDataView *const page = ROM_DATA_VIEW(user_data);
	const uint32_t lc = rom_data_view_get_sel_lc(page);
	rom_data_view_update_multi(page, lc);
}

#ifndef USE_GTK_MENU_BUTTON
/**
 * Menu positioning function.
 * @param menu		[in] GtkMenu*
 * @param x		[out] X position
 * @param y		[out] Y position
 * @param push_in
 * @param user_data	[in] GtkButton* the menu is attached to.
 */
static void
btnOptions_menu_pos_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GtkWidget *const button = (GtkWidget*)GTK_BUTTON(user_data);
#if GTK_CHECK_VERSION(2,14,0)
	GdkWindow *const window = gtk_widget_get_window(button);
#else /* !GTK_CHECK_VERSION(2,14,0) */
	GdkWindow *const window = button->window;
#endif /* GTK_CHECK_VERSION(2,14,0) */

	GtkAllocation button_alloc, menu_alloc;
	gtk_widget_get_allocation(GTK_WIDGET(button), &button_alloc);
	gtk_widget_get_allocation(GTK_WIDGET(menu), &menu_alloc);
	gdk_window_get_origin(window, x, y);
	*x += button_alloc.x;
	*y += button_alloc.y - menu_alloc.height;

	*push_in = false;
}

/**
 * The "Options" button was pressed. (Non-GtkMenuButton version)
 * @param button	GtkButton
 * @param event		GdkEvent
 */
static gboolean
btnOptions_clicked_signal_handler(GtkButton *button,
				  GdkEvent  *event)
{
	if (event->type == GDK_BUTTON_PRESS) {
		// Reference: https://developer.gnome.org/gtk-tutorial/stable/x1577.html
		RomDataView *const page = ROM_DATA_VIEW(g_object_get_data(G_OBJECT(button), "RomDataView"));
		assert(page->menuOptions != nullptr);
		if (!page->menuOptions) {
			// No menu...
			return FALSE;
		}

		GtkMenuPositionFunc menuPositionFunc;
#if GTK_CHECK_VERSION(3,12,0)
		// If we're using a GtkHeaderBar, don't use a custom menu positioning function.
		if (gtk_dialog_get_header_bar(GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(page)))) != nullptr) {
			menuPositionFunc = nullptr;
		} else
#endif /* GTK_CHECK_VERSION(3,12,0) */
		{
			menuPositionFunc = btnOptions_menu_pos_func;
		}

		// Pop up the menu.
		// FIXME: Improve button appearance so it's more pushed-in.
		GdkEventButton *const bevent = reinterpret_cast<GdkEventButton*>(event);
		gtk_menu_popup(GTK_MENU(page->menuOptions),
			nullptr, nullptr,
			menuPositionFunc, button,
			bevent->button, bevent->time);

		// Tell the caller that we handled the event.
		return TRUE;
	}

	// Tell the caller that we did NOT handle the event.
	return FALSE;
}
#endif /* !USE_GTK_MENU_BUTTON */

/**
 * An "Options" menu action was triggered.
 * @param menuItem	Menu item (Get the "menuOptions_id" data.)
 * @param user_data	RomDataView
 */
static void
menuOptions_triggered_signal_handler(GtkMenuItem *menuItem,
				     gpointer	  user_data)
{
	RomDataView *const page = ROM_DATA_VIEW(user_data);
	const gint id = (gboolean)GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(menuItem), "menuOptions_id"));

	if (id < 0) {
		// Export/copy to text or JSON.
		const char *const rom_filename = page->romData->filename();
		if (!rom_filename)
			return;

		bool toClipboard;
		GtkFileFilter *filter = gtk_file_filter_new();
		const char *s_title = nullptr;
		const char *s_default_ext = nullptr;
		switch (id) {
			case OPTION_EXPORT_TEXT:
				toClipboard = false;
				s_title = C_("RomDataView", "Export to Text File");
				s_default_ext = ".txt";
				gtk_file_filter_set_name(filter, C_("RomDataView", "Text Files"));
				gtk_file_filter_add_mime_type(filter, "text/plain");
				gtk_file_filter_add_pattern(filter, "*.txt");
				break;
			case OPTION_EXPORT_JSON:
				toClipboard = false;
				s_title = C_("RomDataView", "Export to JSON File");
				s_default_ext = ".json";
				gtk_file_filter_set_name(filter, C_("RomDataView", "JSON Files"));
				gtk_file_filter_add_mime_type(filter, "application/json");
				gtk_file_filter_add_pattern(filter, "*.json");
				break;
			case OPTION_COPY_TEXT:
			case OPTION_COPY_JSON:
				toClipboard = true;
				break;
			default:
				assert(!"Invalid action ID.");
				return;
		}

		// TODO: GIO wrapper for ostream.
		// For now, we'll use ofstream.
		ofstream ofs;

		if (!toClipboard) {
			GtkWidget *const dialog = gtk_file_chooser_dialog_new(s_title,
				GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(page))),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				nullptr);
			gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);

			// Set the filters.
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, C_("RomDataView", "All Files"));
			gtk_file_filter_add_pattern(filter, "*");
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

			if (!page->prevExportDir) {
				page->prevExportDir = g_path_get_dirname(rom_filename);
			}

			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), page->prevExportDir);

			gchar *const basename = g_path_get_basename(rom_filename);
			string defaultName = basename;
			g_free(basename);
			// Remove the extension, if present.
			size_t extpos = defaultName.rfind('.');
			if (extpos != string::npos) {
				defaultName.resize(extpos);
			}
			defaultName += s_default_ext;
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultName.c_str());

			gint res = gtk_dialog_run(GTK_DIALOG(dialog));
			if (res != GTK_RESPONSE_ACCEPT) {
				// Cancelled...
				gtk_widget_destroy(dialog);
				return;
			}

			gchar *out_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gtk_widget_destroy(dialog);
			if (!out_filename) {
				// No filename...
				return;
			}

			// Save the previous export directory.
			if (page->prevExportDir) {
				g_free(page->prevExportDir);
				page->prevExportDir = nullptr;
			}
			page->prevExportDir = g_path_get_dirname(out_filename);

			ofs.open(out_filename, ofstream::out);
			g_free(out_filename);
			if (ofs.fail()) {
				return;
			}
		}

		// TODO: Optimize this such that we can pass ofstream or ostringstream
		// to a factored-out function.

		switch (id) {
			case OPTION_EXPORT_TEXT: {
				ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(page->romData, rom_data_view_get_sel_lc(page));
				ofs << ro;
				break;
			}
			case OPTION_EXPORT_JSON: {
				JSONROMOutput jsro(page->romData);
				ofs << jsro << std::endl;
				break;
			}
			case OPTION_COPY_TEXT: {
				ostringstream oss;
				oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(page->romData, rom_data_view_get_sel_lc(page));
				oss << ro;

				const string str = oss.str();
				GtkClipboard *const clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				gtk_clipboard_set_text(clip, str.data(), static_cast<gint>(str.size()));
				break;
			}
			case OPTION_COPY_JSON: {
				ostringstream oss;
				JSONROMOutput jsro(page->romData);
				oss << jsro << std::endl;

				const string str = oss.str();
				GtkClipboard *const clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				gtk_clipboard_set_text(clip, str.data(), static_cast<gint>(str.size()));
				break;
			}
			default:
				assert(!"Invalid action ID.");
				return;
		}
	} else {
		// Run a ROM operation.
		GtkMessageType messageType;
		RomData::RomOpResult result;
		int ret = page->romData->doRomOp(id, &result);
		if (ret == 0) {
			// ROM operation completed.

			// Update fields.
			std::for_each(result.fieldIdx.cbegin(), result.fieldIdx.cend(),
				[page](int fieldIdx) {
					rom_data_view_update_field(page, fieldIdx);
				}
			);

			// Update the RomOp menu entry in case it changed.
			// NOTE: Assuming the RomOps vector order hasn't changed.
			// TODO: Have RomData store the RomOps vector instead of
			// rebuilding it here?
			const vector<RomData::RomOps> ops = page->romData->romOps();
			assert(id < (int)ops.size());
			if (id < (int)ops.size()) {
				const RomData::RomOps &op = ops[id];
				const string desc = convert_accel_to_gtk(op.desc.c_str());
				gtk_menu_item_set_label(menuItem, desc.c_str());
				gtk_widget_set_sensitive(GTK_WIDGET(menuItem), !!(op.flags & RomData::RomOps::ROF_ENABLED));
			}

			messageType = GTK_MESSAGE_INFO;
		} else {
			// An error occurred...
			// TODO: Show an error message.
			messageType = GTK_MESSAGE_WARNING;
		}

#ifdef ENABLE_MESSAGESOUND
		MessageSound::play(messageType, result.msg.c_str(), GTK_WIDGET(page));
#endif /* ENABLE_MESSAGESOUND */

		if (!result.msg.empty()) {
			// Show the MessageWidget.
			if (!page->messageWidget) {
				page->messageWidget = message_widget_new();
				gtk_box_pack_end(GTK_BOX(page), page->messageWidget, false, false, 0);
			}

			MessageWidget *const messageWidget = MESSAGE_WIDGET(page->messageWidget);
			message_widget_set_message_type(messageWidget, messageType);
			message_widget_set_text(messageWidget, result.msg.c_str());
#ifdef AUTO_TIMEOUT_MESSAGEWIDGET
			message_widget_show_with_timeout(messageWidget);
#else /* AUTO_TIMEOUT_MESSAGEWIDGET */
			gtk_widget_show(page->messageWidget);
#endif /* AUTO_TIMEOUT_MESSAGEWIDGET */
		}
	}

	// TODO: RomOps
}

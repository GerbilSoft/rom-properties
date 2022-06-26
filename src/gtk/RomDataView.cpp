/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget.                                 *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.gtk.h"

#include "RomDataView.hpp"
#include "rp-gtk-enums.h"
#include "RpGtk.hpp"
#include "sort_funcs.h"
#include "is-supported.hpp"

// ENABLE_MESSAGESOUND is set by CMakeLists.txt.
#ifdef ENABLE_MESSAGESOUND
#  include "MessageSound.hpp"
#endif /* ENABLE_MESSAGESOUND */

// Custom widgets
#include "DragImage.hpp"
#include "MessageWidget.hpp"
#include "LanguageComboBox.hpp"
#include "OptionsMenuButton.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// libdl
#ifdef HAVE_DLVSYM
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE 1
#  endif /* _GNU_SOURCE */
#else /* !HAVE_DLVSYM */
#  define dlvsym(handle, symbol, version) dlsym((handle), (symbol))
#endif /* HAVE_DLVSYM */
#include <dlfcn.h>

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
static void	cboLanguage_lc_changed_signal_handler(GtkComboBox	*widget,
						     uint32_t		 lc,
						     gpointer		 user_data);
static void	btnOptions_triggered_signal_handler (OptionsMenuButton	*menuButton,
						     gint		 id,
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

#if GTK_CHECK_VERSION(3,0,0)
// libhandy function pointers.
// Only initialized if libhandy is linked into the process.
struct HdyHeaderBar;
typedef GType (*pfnGlibGetType_t)(void);
typedef GType (*pfnHdyHeaderBarPackEnd_t)(HdyHeaderBar *self, GtkWidget *child);
static bool has_checked_hdy = false;
static pfnGlibGetType_t pfn_hdy_deck_get_type = nullptr;
static pfnGlibGetType_t pfn_hdy_header_bar_get_type = nullptr;
static pfnHdyHeaderBarPackEnd_t pfn_hdy_header_bar_pack_end = nullptr;
#endif /* GTK_CHECK_VERSION(3,0,0) */

static GParamSpec *props[PROP_LAST];

// GTK+ property page class.
struct _RomDataViewClass {
	superclass __parent__;
};

// Multi-language stuff.
typedef std::pair<GtkWidget*, const RomFields::Field*> Data_StringMulti_t;

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

// C++ objects.
struct _RomDataViewCxx {
	struct tab {
		GtkWidget	*vbox;		// Either parent page or a GtkVBox/GtkBox.
		GtkWidget	*table;		// GtkTable (2.x); GtkGrid (3.x)
		GtkWidget	*lblCredits;

		tab() : vbox(nullptr), table(nullptr), lblCredits(nullptr) { }
	};
	vector<tab> tabs;

	// Description labels.
	vector<GtkWidget*>	vecDescLabels;

	// Multi-language functionality.
	uint32_t	def_lc;

	// RFT_STRING_MULTI value labels.
	vector<Data_StringMulti_t> vecStringMulti;

	// RFT_LISTDATA_MULTI value GtkListStores.
	vector<Data_ListDataMulti_t> vecListDataMulti;
};

// GTK+ property page instance.
struct _RomDataView {
	super __parent__;

	_RomDataViewCxx	*cxx;		// C++ objects
	RomData		*romData;	// ROM data
	gchar		*uri;		// URI (GVfs)

	// "Options" button. (OptionsMenuButton)
	GtkWidget	*btnOptions;
	gchar		*prevExportDir;

	// Header row.
	GtkWidget	*hboxHeaderRow_outer;
	GtkWidget	*hboxHeaderRow;
	GtkWidget	*lblSysInfo;
	GtkWidget	*imgIcon;
	GtkWidget	*imgBanner;

	// Tab layout.
	GtkWidget	*tabWidget;
	// Tabs moved to: cxx->tabs

	// MessageWidget for ROM operation notifications.
	GtkWidget	*messageWidget;

	// Multi-language combo box.
	GtkWidget	*cboLanguage;

	/* Timeouts */
	guint		changed_idle;

	// Description label format type.
	RpDescFormatType	desc_format_type;

	// Inhibit checkbox toggling for RFT_BITFIELD while updating.
	bool inhibit_checkbox_no_toggle;
	// Have we checked for achievements?
	bool hasCheckedAchievements;
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

	props[PROP_URI] = g_param_spec_string(
		"uri", "URI", "URI of the ROM image being displayed.",
		nullptr,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_DESC_FORMAT_TYPE] = g_param_spec_enum(
		"desc-format-type", "desc-format-type", "Description format type.",
		RP_TYPE_DESC_FORMAT_TYPE, RP_DFT_XFCE,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	props[PROP_SHOWING_DATA] = g_param_spec_boolean(
		"showing-data", "showing-data", "Is a valid RomData object being displayed?",
		false,
		(GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);

#if GTK_CHECK_VERSION(3,0,0)
	/** libhandy **/

	// Check if libhandy-1 is loaded in the process.
	// TODO: Verify that it is in fact 1.x if symbol versioning isn't available.
	if (!has_checked_hdy) {
		has_checked_hdy = true;
		pfn_hdy_deck_get_type = (pfnGlibGetType_t)dlvsym(
			RTLD_DEFAULT, "hdy_deck_get_type", "LIBHANDY_1_0");
		if (pfn_hdy_deck_get_type) {
			pfn_hdy_header_bar_get_type = (pfnGlibGetType_t)dlvsym(
				RTLD_DEFAULT, "hdy_header_bar_get_type", "LIBHANDY_1_0");
			pfn_hdy_header_bar_pack_end = (pfnHdyHeaderBarPackEnd_t)dlvsym(
				RTLD_DEFAULT, "hdy_header_bar_pack_end", "LIBHANDY_1_0");
		}
	}
#endif /* GTK_CHECK_VERSION(3,0,0) */
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
	// g_object_new() guarantees that all values are initialized to 0.

	// Initialize C++ objects.
	page->cxx = new _RomDataViewCxx();

	// Default description format type.
	page->desc_format_type = RP_DFT_XFCE;

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
	gtk_widget_hide(page->hboxHeaderRow_outer);	// GTK4 shows widgets by default.

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_halign(page->hboxHeaderRow, GTK_ALIGN_CENTER);
	gtk_widget_show(page->hboxHeaderRow);

#  if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(page), page->hboxHeaderRow_outer);
	gtk_box_append(GTK_BOX(page->hboxHeaderRow_outer), page->hboxHeaderRow);
	gtk_widget_set_hexpand(page->hboxHeaderRow_outer, true);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow_outer, false, false, 0);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow_outer), page->hboxHeaderRow, true, false, 0);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#else /* !GTK_CHECK_VERSION(3,0,0) */
	// Header row. (outer box)
	// NOTE: Not visible initially.
	page->hboxHeaderRow_outer = gtk_hbox_new(false, 0);

	// Center-align the header row.
	GtkWidget *centerAlign = gtk_alignment_new(0.5f, 0.0f, 1.0f, 0.0f);
	gtk_widget_show(centerAlign);

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_hbox_new(false, 8);
	gtk_widget_show(page->hboxHeaderRow);

	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow_outer, false, false, 0);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow_outer), centerAlign, true, false, 0);
	gtk_container_add(GTK_CONTAINER(page->hboxHeaderRow_outer), page->hboxHeaderRow);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// System information.
	page->lblSysInfo = gtk_label_new(nullptr);
	gtk_label_set_justify(GTK_LABEL(page->lblSysInfo), GTK_JUSTIFY_CENTER);
	gtk_widget_show(page->lblSysInfo);

	// Banner and icon.
	page->imgBanner = drag_image_new();
	page->imgIcon = drag_image_new();

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(page->hboxHeaderRow), page->lblSysInfo);
	gtk_box_append(GTK_BOX(page->hboxHeaderRow), page->imgBanner);
	gtk_box_append(GTK_BOX(page->hboxHeaderRow), page->imgIcon);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->lblSysInfo, false, false, 0);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgBanner, false, false, 0);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow), page->imgIcon, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

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

	// Delete the icon frames and tabs.
	rom_data_view_delete_tabs(page);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rom_data_view_parent_class)->dispose(object);
}

static void
rom_data_view_finalize(GObject *object)
{
	RomDataView *const page = ROM_DATA_VIEW(object);

	// Delete the C++ objects.
	delete page->cxx;

	// Free the strings.
	g_free(page->prevExportDir);
	g_free(page->uri);

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
	return rom_data_view_new_with_romData(uri, nullptr, desc_format_type);
}

GtkWidget*
rom_data_view_new_with_romData(const gchar *uri, RomData *romData, RpDescFormatType desc_format_type)
{
	// At least URI needs to be set.
	assert(uri != nullptr);

	RomDataView *const page = static_cast<RomDataView*>(g_object_new(TYPE_ROM_DATA_VIEW, nullptr));
	page->desc_format_type = desc_format_type;
	if (uri) {
		page->uri = g_strdup(uri);
		if (romData) {
			page->romData = romData->ref();
		}
	}
	if (G_LIKELY(uri != nullptr)) {
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
	RomDataView *const page = ROM_DATA_VIEW(object);

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
	RomDataView *const page = ROM_DATA_VIEW(object);

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
		page->hasCheckedAchievements = false;

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
	// FIXME: If called from rom_data_view_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(page), props[PROP_URI]);
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

	// FIXME: If called from rom_data_view_set_property(), this might
	// result in *two* notifications.
	page->desc_format_type = desc_format_type;
	rom_data_view_desc_format_type_changed(page, desc_format_type);
	g_object_notify_by_pspec(G_OBJECT(page), props[PROP_DESC_FORMAT_TYPE]);
}

static void
rom_data_view_desc_format_type_changed(RomDataView	*page,
				       RpDescFormatType	desc_format_type)
{
	g_return_if_fail(IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);

	for (GtkWidget *label : page->cxx->vecDescLabels) {
		set_label_format_type(GTK_LABEL(label), desc_format_type);
	}
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

	// NOTE: Set the field index here, since `widget` might be NULLed out later.
	g_object_set_data(G_OBJECT(widget), "RFT_fieldIdx", GINT_TO_POINTER(fieldIdx+1));

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
			auto &tab = page->cxx->tabs.at(field.tabIdx);
			assert(tab.lblCredits == nullptr);
			tab.lblCredits = widget;

			// Credits row.
#if GTK_CHECK_VERSION(4,0,0)
			// TODO: "end"?
			gtk_box_append(GTK_BOX(tab.vbox), widget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gtk_box_pack_end(GTK_BOX(tab.vbox), widget, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

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

	g_object_set_data(G_OBJECT(widget), "RFT_fieldIdx", GINT_TO_POINTER(fieldIdx+1));
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
	assert(colCount > 0);
	if (colCount <= 0) {
		// No columns...
		return nullptr;
	}

	GtkListStore *listStore;
	int listStore_col_start;
	if (hasCheckboxes) {
		// Prepend an extra column for checkboxes.
		GType *types = new GType[colCount+1];
		types[0] = G_TYPE_BOOLEAN;
		for (int i = colCount; i > 0; i--) {
			types[i] = G_TYPE_STRING;
		}
		listStore = gtk_list_store_newv(colCount+1, types);
		delete[] types;
		listStore_col_start = 1;	// Skip the checkbox column for strings.
	} else if (hasIcons) {
		// Prepend an extra column for icons.
		GType *types = new GType[colCount+1];
		types[0] = PIMGTYPE_GOBJECT_TYPE;
		for (int i = colCount; i > 0; i--) {
			types[i] = G_TYPE_STRING;
		}
		listStore = gtk_list_store_newv(colCount+1, types);
		delete[] types;
		listStore_col_start = 1;	// Skip the icon column for strings.
	} else {
		// All strings.
		GType *types = new GType[colCount];
		for (int i = colCount-1; i >= 0; i--) {
			types[i] = G_TYPE_STRING;
		}
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
			int col = listStore_col_start;
			const auto data_row_cend = data_row.cend();
			for (auto iter = data_row.cbegin(); iter != data_row_cend; ++iter, col++) {
				gtk_list_store_set(listStore, &treeIter, col, iter->c_str(), -1);
			}
		}
	}

	// Scroll area for the GtkTreeView.
#if GTK_CHECK_VERSION(4,0,0)
	GtkWidget *scrolledWindow = gtk_scrolled_window_new();
	gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolledWindow), true);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scrolledWindow);

	// Sort proxy model for the GtkListStore.
	GtkTreeModel *sortProxy = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(listStore));

	// Create the GtkTreeView.
	GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sortProxy));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView),
		(listDataDesc.names != nullptr));
	gtk_widget_show(treeView);

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), treeView);

	// TODO: Set fixed height mode?
	// May require fixed columns...
	// Reference: https://developer.gnome.org/gtk3/stable/GtkTreeView.html#gtk-tree-view-set-fixed-height-mode

#if GTK_CHECK_VERSION(3,0,0)
	// FIXME: Alternating row colors isn't working in GTK+ 3.x...
#else
	// GTK+ 2.x: Use the "rules hint" for alternating row colors.
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeView), true);
#endif

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

		GtkCellRenderer *const renderer = gtk_cell_renderer_text_new();
		if (col0_renderer != nullptr) {
			// Prepend the icon/checkbox renderer.
			gtk_tree_view_column_pack_start(column, col0_renderer, FALSE);
			gtk_tree_view_column_add_attribute(column, col0_renderer, col0_attr_name, 0);
			col0_renderer = nullptr;
		}
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
		switch (col_attrs.sorting & RomFields::COLSORT_MASK) {
			default:
				// Unsupported. We'll use standard sorting.
				assert(!"Unsupported sorting method.");
				// fall-through
			case RomFields::COLSORT_STANDARD:
				// Standard sorting.
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
	g_object_set_data(G_OBJECT(treeView), "RFT_LISTDATA_rows_visible",
		GINT_TO_POINTER(listDataDesc.rows_visible));
	if (listDataDesc.rows_visible > 0) {
		g_signal_connect(treeView, "realize", G_CALLBACK(tree_view_realize_signal_handler), page);
	}

	if (isMulti) {
		page->cxx->vecListDataMulti.emplace_back(
			Data_ListDataMulti_t(listStore, GTK_TREE_VIEW(treeView), &field));
	}

	g_object_set_data(G_OBJECT(scrolledWindow), "RFT_fieldIdx", GINT_TO_POINTER(fieldIdx+1));
	return scrolledWindow;
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
	assert(dateTime != nullptr);
	if (!dateTime) {
		// Unable to convert the timestamp.
		return rom_data_view_init_string(page, field, fieldIdx, C_("RomDataView", "Unknown"));
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
	GtkWidget *const lblStringMulti = rom_data_view_init_string(page, field, fieldIdx, nullptr);
	if (lblStringMulti) {
		page->cxx->vecStringMulti.emplace_back(lblStringMulti, &field);
	}
	return lblStringMulti;
}

/**
 * Update all multi-language fields.
 * @param page		[in] RomDataView object.
 * @param user_lc	[in] User-specified language code.
 */
static void
rom_data_view_update_multi(RomDataView *page, uint32_t user_lc)
{
	_RomDataViewCxx *const cxx = page->cxx;
	set<uint32_t> set_lc;

	// RFT_STRING_MULTI
	for (const Data_StringMulti_t &vsm : cxx->vecStringMulti) {
		GtkWidget *const lblString = vsm.first;
		const RomFields::Field *const pField = vsm.second;
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
			for (const auto &psm : *pStr_multi) {
				set_lc.emplace(psm.first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, cxx->def_lc, user_lc);
		assert(pStr != nullptr);
		gtk_label_set_text(GTK_LABEL(lblString), (pStr ? pStr->c_str() : ""));
	}

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
			const auto &listDataDesc = pField->desc.list_data;

			// If we have checkboxes or icons, start at column 1.
			// Otherwise, start at column 0.
			int listStore_col_start;
			if (listDataDesc.flags & (RomFields::RFT_LISTDATA_CHECKBOXES | RomFields::RFT_LISTDATA_ICONS)) {
				// Checkboxes and/or icons are present.
				listStore_col_start = 1;
			} else {
				listStore_col_start = 0;
			}

			// Update the list.
			GtkTreeIter treeIter;
			GtkTreeModel *const treeModel = GTK_TREE_MODEL(listStore);
			gboolean ok = gtk_tree_model_get_iter_first(treeModel, &treeIter);
			auto iter_listData = pListData->cbegin();
			const auto pListData_cend = pListData->cend();
			while (ok && iter_listData != pListData_cend) {
				// TODO: Verify GtkListStore column count?
				int col = listStore_col_start;
				const auto iter_listData_cend = iter_listData->cend();
				for (auto iter_row = iter_listData->cbegin();
				     iter_row != iter_listData_cend; ++iter_row, col++)
				{
					gtk_list_store_set(listStore, &treeIter, col, iter_row->c_str(), -1);
				}

				// Next row.
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

	if (!page->cboLanguage && set_lc.size() > 1) {
		// Create a VBox for the combobox to reduce its vertical height.
#if GTK_CHECK_VERSION(3,0,0)
		GtkWidget *const vboxCboLanguage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_widget_set_valign(vboxCboLanguage, GTK_ALIGN_START);
		gtk_widget_show(vboxCboLanguage);

#  if GTK_CHECK_VERSION(4,0,0)
		gtk_box_append(GTK_BOX(page->hboxHeaderRow_outer), vboxCboLanguage);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), vboxCboLanguage, false, false, 0);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#else /* !GTK_CHECK_VERSION(3,0,0) */
		GtkWidget *const topAlign = gtk_alignment_new(0.5f, 0.0f, 0.0f, 0.0f);
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), topAlign, false, false, 0);
		gtk_widget_show(topAlign);

		GtkWidget *const vboxCboLanguage = gtk_vbox_new(false, 0);
		gtk_container_add(GTK_CONTAINER(topAlign), vboxCboLanguage);
		gtk_widget_show(vboxCboLanguage);
#endif /* GTK_CHECK_VERSION(3,0,0) */

		// Create the language combobox.
		page->cboLanguage = language_combo_box_new();
		gtk_widget_show(page->cboLanguage);

#if GTK_CHECK_VERSION(4,0,0)
		gtk_box_append(GTK_BOX(vboxCboLanguage), page->cboLanguage);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_box_pack_end(GTK_BOX(vboxCboLanguage), page->cboLanguage, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

		// Set the languages.
		// NOTE: LanguageComboBox uses a 0-terminated array, so we'll
		// need to convert the std::set<> to an std::vector<>.
		vector<uint32_t> vec_lc;
		vec_lc.reserve(set_lc.size() + 1);
		vec_lc.assign(set_lc.cbegin(), set_lc.cend());
		vec_lc.emplace_back(0);
		language_combo_box_set_lcs(LANGUAGE_COMBO_BOX(page->cboLanguage), vec_lc.data());

		// Select the default language.
		uint32_t lc_to_set = 0;
		const auto set_lc_end = set_lc.end();
		if (set_lc.find(cxx->def_lc) != set_lc_end) {
			// def_lc was found.
			lc_to_set = cxx->def_lc;
		} else if (set_lc.find('en') != set_lc_end) {
			// 'en' was found.
			lc_to_set = 'en';
		} else {
			// Unknown. Select the first language.
			if (!set_lc.empty()) {
				lc_to_set = *(set_lc.cbegin());
			}
		}
		language_combo_box_set_selected_lc(LANGUAGE_COMBO_BOX(page->cboLanguage), lc_to_set);

		// Connect the "lc-changed" signal.
		g_signal_connect(page->cboLanguage, "lc-changed", G_CALLBACK(cboLanguage_lc_changed_signal_handler), page);
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
	assert(page != nullptr);
	assert(page->cxx != nullptr);
	_RomDataViewCxx *const cxx = page->cxx;

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

	// Lambda function to check a QObject's RFT_fieldIdx.
	auto checkFieldIdx = [](GtkWidget *widget, int fieldIdx) -> bool {
		// NOTE: RFT_fieldIdx starts at 1 to prevent conflicts with widgets
		// that don't have RFT_fieldIdx, which would return NULL here.
		const gint tmp_fieldIdx = GPOINTER_TO_INT(
			g_object_get_data(G_OBJECT(widget), "RFT_fieldIdx"));
		return (tmp_fieldIdx != 0 && (tmp_fieldIdx - 1) == fieldIdx);
	};

	// Get the GtkWidget*.
	// NOTE: Linear search through all display objects, since
	// this function isn't used that often.
	GtkWidget *widget = nullptr;
	const auto tabs_cend = cxx->tabs.cend();
	for (auto iter = cxx->tabs.cbegin(); iter != tabs_cend && widget == nullptr; ++iter) {
		GtkWidget *const table = iter->table;	// GtkTable (2.x); GtkGrid (3.x)

#if GTK_CHECK_VERSION(4,0,0)
		// Get the first child.
		// NOTE: Widgets are enumerated in forwards order.
		// TODO: Needs testing!
		for (GtkWidget *tmp_widget = gtk_widget_get_first_child(table);
		     tmp_widget != nullptr; tmp_widget = gtk_widget_get_next_sibling(tmp_widget))
		{
			// Check if the field index is correct.
			if (checkFieldIdx(tmp_widget, fieldIdx)) {
				// Found the field.
				widget = tmp_widget;
				break;
			}
		}
#else /* !GTK_CHECK_VERSION(4,0,0) */
		// Get the list of child widgets.
		// NOTE: Widgets are enumerated in forwards order,
		// since the list head is the first item.
		GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(table));
		if (!widgetList)
			continue;

		for (GList *widgetIter = widgetList; widgetIter != nullptr;
		     widgetIter = widgetIter->next)
		{
			GtkWidget *const tmp_widget = GTK_WIDGET(widgetIter->data);
			if (!tmp_widget)
				continue;

			// Check if the field index is correct.
			if (checkFieldIdx(tmp_widget, fieldIdx)) {
				// Found the field.
				widget = tmp_widget;
				break;
			}
		}
		g_list_free(widgetList);
#endif
	}

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

#if GTK_CHECK_VERSION(4,0,0)
			// Get the first child.
			// NOTE: Widgets are enumerated in forwards order.
			// TODO: Needs testing!
			GtkWidget *checkBox = gtk_widget_get_first_child(widget);
			if (!checkBox) {
				ret = 9;
				break;
			}

			// Inhibit the "no-toggle" signal while updating.
			page->inhibit_checkbox_no_toggle = true;

			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend && checkBox != nullptr;
			     ++iter, checkBox = gtk_widget_get_next_sibling(checkBox), bitfield >>= 1)
			{
				const string &name = *iter;
				if (name.empty())
					continue;

				assert(GTK_IS_CHECK_BUTTON(checkBox));
				if (!GTK_IS_CHECK_BUTTON(checkBox))
					break;

				const bool value = (bitfield & 1);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkBox), value);
				g_object_set_data(G_OBJECT(checkBox), "RFT_BITFIELD_value", GUINT_TO_POINTER((guint)value));
			}
#else /* !GTK_CHECK_VERSION(4,0,0) */
			// Get the list of child widgets.
			// NOTE: Widgets are enumerated in reverse order.
			GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(widget));
			if (!widgetList) {
				ret = 9;
				break;
			}
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
#endif /* GTK_CHECK_VERSION(4,0,0) */

			// Done updating.
			page->inhibit_checkbox_no_toggle = false;
			ret = 0;
			break;
		}
	}

	return ret;
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

	// The parent widget can be one of two types:
	// - nautilus-3.x: GtkNotebook
	// - caja-3.x: GtkNotebook
	// - nemo-3.x: GtkStack
#ifndef GTK_IS_STACK
#  define GTK_IS_STACK(x) 1
#endif
	assert(GTK_IS_NOTEBOOK(parent) || GTK_IS_STACK(parent));
	if (!GTK_IS_NOTEBOOK(parent) && !GTK_IS_STACK(parent))
		return;

	// Nemo only: We might have a GtkFrame between the GtkStack and GtkBox.
	GtkWidget *fparent = gtk_widget_get_parent(parent);
	if (GTK_IS_FRAME(fparent))
		parent = fparent;

	// Next: Container widget.
	// - GTK+ 2.x: GtkVBox
	// - GTK+ 3.x: GtkBox
	parent = gtk_widget_get_parent(parent);
#if GTK_CHECK_VERSION(3,0,0)
	assert(GTK_IS_BOX(parent));
	if (!GTK_IS_BOX(parent))
		return;
#else /* !GTK_CHECK_VERSION(3,0,0) */
	assert(GTK_IS_VBOX(parent));
	if (!GTK_IS_VBOX(parent))
		return;
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Next: GtkDialog subclass.
	// - XFCE: ThunarPropertiesDialog
	// - Nautilus: NautilusPropertiesWindow
	//   - NOTE: Nautilus 40 uses HdyWindow, which is a GtkWindow subclass.
	// - Caja: FMPropertiesWindow
	// - Nemo: NemoPropertiesWindow
#if GTK_CHECK_VERSION(3,0,0)
	bool isLibHandy = false;
#endif /* GTK_CHECK_VERSION(3,0,0) */
	parent = gtk_widget_get_parent(parent);
#if GTK_CHECK_VERSION(3,0,0)
	if (!GTK_IS_DIALOG(parent)) {
		// NOTE: As of Nautilus 40, there may be an HdyDeck here.
		// Check if it's HdyDeck using dynamically-loaded function pointers.
		if (pfn_hdy_deck_get_type && G_OBJECT_TYPE(parent) == pfn_hdy_deck_get_type()) {
			// Get the next parent widget.
			isLibHandy = true;
			parent = gtk_widget_get_parent(parent);
		}
	}
	if (isLibHandy) {
		// Main window is based on HdyWindow, which is derived from
		// GtkWindow, not GtkDialog.
		assert(GTK_IS_WINDOW(parent));
		if (!GTK_IS_WINDOW(parent))
			return;
	} else
#endif /* GTK_CHECK_VERSION(3,0,0) */
	{
		// Main window is derived from GtkDialog.
		assert(GTK_IS_DIALOG(parent));
		if (!GTK_IS_DIALOG(parent))
			return;
	}

	// Create the OptionsMenuButton.
	page->btnOptions = options_menu_button_new();
	gtk_widget_hide(page->btnOptions);
	options_menu_button_set_direction(OPTIONS_MENU_BUTTON(page->btnOptions), GTK_ARROW_UP);

#if GTK_CHECK_VERSION(3,0,0)
	if (isLibHandy) {
		// LibHandy version doesn't use GtkDialog.
	} else
#endif /* GTK_CHECK_VERSION(3,0,0) */
	{
		// Not using LibHandy, so add the widget to the GtkDialog.
		gtk_dialog_add_action_widget(GTK_DIALOG(parent), page->btnOptions, GTK_RESPONSE_NONE);

		// Disconnect the "clicked" signal from the default GtkDialog response handler.
		// NOTE: May be "activate" now that OptionsMenuButton no longer derives from
		// GtkMenuButton/GtkButton.
		guint signal_id = 0;
		if (GTK_IS_BUTTON(page->btnOptions)) {
			signal_id = g_signal_lookup("clicked", GTK_TYPE_BUTTON);
		} else {
			signal_id = gtk_widget_class_get_activate_signal(GTK_WIDGET_GET_CLASS(page->btnOptions));
		}
		gulong handler_id = g_signal_handler_find(page->btnOptions, G_SIGNAL_MATCH_ID,
			signal_id, 0, nullptr, 0, 0);
		g_signal_handler_disconnect(page->btnOptions, handler_id);
	}

#if GTK_CHECK_VERSION(3,12,0)
	GtkWidget *headerBar = nullptr;
	if (isLibHandy) {
		// Nautilus 40 uses libhandy, which has a different arrangement of widgets.
		// NautilusPropertiesWindow
		// |- GtkBox
		//    |- HdyHeaderBar
		GtkWidget *const gtkBox = gtk_widget_get_first_child(parent);
		if (gtkBox && GTK_IS_BOX(gtkBox)) {
			GtkWidget *const hdyHeaderBar = gtk_widget_get_first_child(gtkBox);
			if (hdyHeaderBar && G_OBJECT_TYPE(hdyHeaderBar) == pfn_hdy_header_bar_get_type())
			{
				// Found the HdyHeaderBar.
				// Pack the Options button at the end.
				// NOTE: No type checking here...
				pfn_hdy_header_bar_pack_end((HdyHeaderBar*)hdyHeaderBar, page->btnOptions);
				headerBar = hdyHeaderBar;
			}
		}
	} else {
		// Earlier versions use GtkDialog's own header bar functionality.
		headerBar = gtk_dialog_get_header_bar(GTK_DIALOG(parent));
	}
	if (headerBar) {
		// Dialog has a Header Bar instead of an action area.
		// No reordering is necessary.

		// Change the arrow to point down instead of up.
		options_menu_button_set_direction(OPTIONS_MENU_BUTTON(page->btnOptions), GTK_ARROW_DOWN);
	} else
#endif /* GTK_CHECK_VERSION(3,12,0) */
	{
		// Reorder the "Options" button so it's to the right of "Help".
		// NOTE: GTK+ 3.10 introduced the GtkHeaderBar, but
		// gtk_dialog_get_header_bar() was added in GTK+ 3.12.
		// Hence, this might not work properly on GTK+ 3.10.
		// FIXME: GTK4 no longer has GtkButtonBox. Figure out "secondary" there.
#if !GTK_CHECK_VERSION(4,0,0)
		GtkWidget *const btnBox = gtk_widget_get_parent(page->btnOptions);
		//assert(GTK_IS_BUTTON_BOX(btnBox));
		if (GTK_IS_BUTTON_BOX(btnBox))
		{
			gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(btnBox), page->btnOptions, TRUE);
		}
#endif /* !GTK_CHECK_VERSION(4,0,0) */
	}

	// Connect the OptionsMenuButton's triggered(int) signal.
	g_signal_connect(page->btnOptions, "triggered", G_CALLBACK(btnOptions_triggered_signal_handler), page);

	// Initialize the menu options.
	options_menu_button_reinit_menu(OPTIONS_MENU_BUTTON(page->btnOptions), page->romData);
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
	auto &tabs = page->cxx->tabs;
	int tabCount = pFields->tabCount();
	if (tabCount > 1) {
		tabs.resize(tabCount);
		page->tabWidget = gtk_notebook_new();

		// Add spacing between the system info header and the table.
		g_object_set(page, "spacing", 8, nullptr);

		auto tabIter = tabs.begin();
		for (int i = 0; i < tabCount; i++, ++tabIter) {
			// Create a tab.
			const char *name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = *tabIter;

			tab.vbox = RP_gtk_vbox_new(8);
#if USE_GTK_GRID
			tab.table = gtk_grid_new();
			gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
			gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else /* !USE_GTK_GRID */
			// TODO: Adjust the table size?
			tab.table = gtk_table_new(rowCount, 2, false);
			gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
			gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif /* USE_GTK_GRID */

#if GTK_CHECK_VERSION(4,0,0)
			// FIXME: GTK4 equivalent of gtk_container_set_border_width().
			//gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
			gtk_box_append(GTK_BOX(tab.vbox), tab.table);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
			gtk_box_pack_start(GTK_BOX(tab.vbox), tab.table, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */

			gtk_widget_show(tab.table);
			gtk_widget_show(tab.vbox);

			// Add the tab.
			GtkWidget *label = gtk_label_new(name);
			gtk_notebook_append_page(GTK_NOTEBOOK(page->tabWidget), tab.vbox, label);
		}
		gtk_widget_show(page->tabWidget);

#if GTK_CHECK_VERSION(4,0,0)
		gtk_widget_set_halign(page->tabWidget, GTK_ALIGN_FILL);
		gtk_widget_set_valign(page->tabWidget, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(page->tabWidget, true);
		gtk_widget_set_vexpand(page->tabWidget, true);
		gtk_box_append(GTK_BOX(page), page->tabWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_box_pack_start(GTK_BOX(page), page->tabWidget, true, true, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */
	} else {
		// No tabs.
		// Don't create a GtkNotebook, but simulate a single
		// tab in page->tabs[] to make it easier to work with.
		tabCount = 1;
		tabs.resize(1);
		auto &tab = tabs.at(0);
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
		gtk_widget_show(tab.table);

#if GTK_CHECK_VERSION(4,0,0)
		// FIXME: GTK4 equivalent of gtk_container_set_border_width().
		//gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
		gtk_box_append(GTK_BOX(page), tab.table);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_container_set_border_width(GTK_CONTAINER(tab.table), 8);
		gtk_box_pack_start(GTK_BOX(page), tab.table, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */
	}

	// Reserve enough space for vecDescLabels.
	page->cxx->vecDescLabels.reserve(count);
	// Per-tab row counts.
	vector<int> tabRowCount(tabs.size());

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");

	// Create the data widgets.
	int fieldIdx = 0;
	const auto pFields_cend = pFields->cend();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, fieldIdx++) {
		const RomFields::Field &field = *iter;
		if (!field.isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0 && tabIdx < static_cast<int>(tabs.size()));
		if (tabIdx < 0 || tabIdx >= static_cast<int>(tabs.size())) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs.at(tabIdx).table) {
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
			auto &tab = tabs.at(tabIdx);

			// tr: Field description label.
			const string txt = rp_sprintf(desc_label_fmt, field.name.c_str());
			GtkWidget *lblDesc = gtk_label_new(txt.c_str());
			gtk_label_set_use_underline(GTK_LABEL(lblDesc), false);
			gtk_widget_show(lblDesc);
			page->cxx->vecDescLabels.emplace_back(lblDesc);

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
				GTK_LABEL_XALIGN_LEFT(lblDesc);

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
					// Set expand and fill properties.
					gtk_widget_set_vexpand(widget, true);
					gtk_widget_set_valign(widget, GTK_ALIGN_FILL);

					// Set margin, since it's located outside of the GtkTable/GtkGrid.
					// NOTE: Setting top margin to 0 due to spacing from the
					// GtkTable/GtkGrid. (Still has extra spacing that needs to be fixed...)
					g_object_set(widget,
						"margin-left", 8, "margin-right",  8,
						"margin-top",  0, "margin-bottom", 8,
						nullptr);

					GtkWidget *const widget_add = widget;
#else /* !USE_GTK_GRID */
					// Need to use GtkAlignment on GTK+ 2.x.
					GtkWidget *const alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
					gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 8, 8, 8);
					gtk_container_add(GTK_CONTAINER(alignment), widget);
					gtk_widget_show(alignment);

					GtkWidget *const widget_add = alignment;
#endif /* USE_GTK_GRID */

					// Add the widget to the GtkBox.
#if GTK_CHECK_VERSION(4,0,0)
					if (tab.lblCredits) {
						// Need to insert the widget before credits.
						// TODO: Verify this.
						gtk_widget_insert_before(widget_add, tab.vbox, tab.lblCredits);
					} else {
						gtk_box_append(GTK_BOX(tab.vbox), widget_add);
					}
#else /* !GTK_CHECK_VERSION(4,0,0) */
					gtk_box_pack_start(GTK_BOX(tab.vbox), widget_add, true, true, 0);
					if (tab.lblCredits) {
						// Need to move it before credits.
						// TODO: Verify this.
						gtk_box_reorder_child(GTK_BOX(tab.vbox), widget_add, 1);
					}
#endif /* GTK_CHECK_VERSION(4,0,0) */

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
	if (!page->cxx->vecStringMulti.empty() || !page->cxx->vecListDataMulti.empty()) {
		page->cxx->def_lc = pFields->defaultLanguageCode();
		rom_data_view_update_multi(page, 0);
	}
}

static gboolean
rom_data_view_load_rom_data(gpointer data)
{
	RomDataView *const page = ROM_DATA_VIEW(data);
	g_return_val_if_fail(page != nullptr && IS_ROM_DATA_VIEW(page), G_SOURCE_REMOVE);

	if (G_UNLIKELY(page->uri == nullptr && page->romData == nullptr)) {
		// No URI or RomData.
		// TODO: Remove widgets?
		page->changed_idle = 0;
		return G_SOURCE_REMOVE;
	}

	// Do we have a RomData object loaded already?
	if (!page->romData) {
		// No RomData object. Load the specified URI.
		RomData *const romData = rp_gtk_open_uri(page->uri);
		if (romData) {
			// FIXME: If called from rom_data_view_set_property(), this might
			// result in *two* notifications.
			page->romData->unref();
			page->romData = romData;
			page->hasCheckedAchievements = false;
			g_object_notify_by_pspec(G_OBJECT(page), props[PROP_SHOWING_DATA]);
		}
	} else {
		// RomData object is already loaded.
		page->hasCheckedAchievements = false;
		g_object_notify_by_pspec(G_OBJECT(page), props[PROP_SHOWING_DATA]);
	}

	if (page->romData) {
		// Update the display widgets.
		// TODO: If already mapped, check achievements again.
		rom_data_view_update_display(page);

		// Make sure the underlying file handle is closed,
		// since we don't need it once the RomData has been
		// loaded by RomDataView.
		page->romData->close();
	}

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
	assert(page->cxx != nullptr);
	_RomDataViewCxx *const cxx = page->cxx;

	// Clear the tabs.
	if (cxx->tabs.size() == 1) {
		// Single tab. We'll need to remove the table first.
		GtkWidget *const table = cxx->tabs[0].table;
		if (table) {
#if GTK_CHECK_VERSION(4,0,0)
			gtk_box_remove(GTK_BOX(page), table);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gtk_container_remove(GTK_CONTAINER(page), table);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		}
	}
	cxx->tabs.clear();

	if (page->messageWidget) {
		// Delete the message widget.
#if GTK_CHECK_VERSION(4,0,0)
		gtk_box_remove(GTK_BOX(page), page->messageWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_container_remove(GTK_CONTAINER(page), page->messageWidget);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		page->messageWidget = nullptr;
	}

	if (page->tabWidget) {
		// Delete the tab widget.
#if GTK_CHECK_VERSION(4,0,0)
		gtk_box_remove(GTK_BOX(page), page->tabWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_container_remove(GTK_CONTAINER(page), page->tabWidget);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		page->tabWidget = nullptr;
	}

	if (page->cboLanguage) {
		// Delete the language combobox.
		// NOTE: cboLanguage is contained in a GtkBox (GtkVBox).
		// We'll need to get the parent widget first, then remove
		// the parent widget.
		GtkWidget *parent = gtk_widget_get_parent(page->cboLanguage);
#if GTK_CHECK_VERSION(3,0,0)
		assert(GTK_IS_BOX(parent));
#else /* !GTK_CHECK_VERSION(3,0,0) */
		// GTK2: The GtkVBox is contained within a GtkAlignment.
		parent = gtk_widget_get_parent(parent);
		assert(GTK_IS_ALIGNMENT(parent));
#endif /* GTK_CHECK_VERSION(3,0,0) */

#if GTK_CHECK_VERSION(4,0,0)
		gtk_box_remove(GTK_BOX(page->hboxHeaderRow_outer), parent);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_container_remove(GTK_CONTAINER(page->hboxHeaderRow_outer), parent);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		page->cboLanguage = nullptr;
	}

	// Clear the various widget references.
	cxx->vecDescLabels.clear();
	cxx->def_lc = 0;
	cxx->vecStringMulti.clear();
	cxx->vecListDataMulti.clear();
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

	// Check for "viewed" achievements.
	if (!page->hasCheckedAchievements) {
		page->romData->checkViewedAchievements();
		page->hasCheckedAchievements = true;
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
 * The RFT_MULTI_STRING language was changed.
 * @param widget	GtkComboBox
 * @param lc		Language code
 * @param user_data	RomDataView
 */
static void
cboLanguage_lc_changed_signal_handler(GtkComboBox *widget,
				      uint32_t     lc,
				      gpointer     user_data)
{
	RP_UNUSED(widget);
	rom_data_view_update_multi(ROM_DATA_VIEW(user_data), lc);
}

/**
 * An "Options" menu action was triggered.
 * @param menuButton	OptionsMenuButton
 * @param id		Menu options ID
 * @param user_data	RomDataView
 */
static void
btnOptions_triggered_signal_handler(OptionsMenuButton *menuButton,
				    gint id,
				    gpointer user_data)
{
	RP_UNUSED(menuButton);
	RomDataView *const page = ROM_DATA_VIEW(user_data);
	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(page));

	if (id < 0) {
		// Export/copy to text or JSON.
		const char *const rom_filename = page->romData->filename();
		if (!rom_filename)
			return;

		bool toClipboard;
		const char *s_title = nullptr;
		const char *s_default_ext = nullptr;
		const char *s_filter = nullptr;
		switch (id) {
			case OPTION_EXPORT_TEXT:
				toClipboard = false;
				s_title = C_("RomDataView", "Export to Text File");
				s_default_ext = ".txt";
				// tr: Text files filter. (RP format)
				s_filter = C_("RomDataView", "Text Files|*.txt|text/plain|All Files|*.*|-");
				break;
			case OPTION_EXPORT_JSON:
				toClipboard = false;
				s_title = C_("RomDataView", "Export to JSON File");
				s_default_ext = ".json";
				// tr: JSON files filter. (RP format)
				s_filter = C_("RomDataView", "JSON Files|*.json|application/json|All Files|*.*|-");
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
			if (!page->prevExportDir) {
				page->prevExportDir = g_path_get_dirname(rom_filename);
			}

#if GTK_CHECK_VERSION(4,0,0)
			// NOTE: GTK4 has *mandatory* overwrite confirmation.
			// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12
			GtkFileChooserNative *const dialog = gtk_file_chooser_native_new(
				s_title,			// title
				parent,				// parent
				GTK_FILE_CHOOSER_ACTION_SAVE,	// action
				_("_Save"), _("_Cancel"));
			// TODO: URI?
			GFile *const set_file = g_file_new_for_path(page->prevExportDir);
			if (set_file) {
				gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), set_file, nullptr);
				g_object_unref(set_file);
			}
#else /* !GTK_CHECK_VERSION(4,0,0) */
			GtkWidget *const dialog = gtk_file_chooser_dialog_new(
				s_title,			// title
				parent,				// parent
				GTK_FILE_CHOOSER_ACTION_SAVE,	// action
				_("_Cancel"), GTK_RESPONSE_CANCEL,
				_("_Save"), GTK_RESPONSE_ACCEPT,
				nullptr);
			gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), page->prevExportDir);
#endif /* GTK_CHECK_VERSION(4,0,0) */

			// Set the filters.
			rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(dialog), s_filter);

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

			// Prompt for a save file.
#if GTK_CHECK_VERSION(4,0,0)
			// GTK4 no longer supports blocking dialogs.
			// FIXME for GTK4: Rewrite to use gtk_window_set_modal() and handle the "response" signal.
			// This will also work for older GTK+.
			assert(!"gtk_dialog_run() is not available in GTK4; needs a rewrite!");
			GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

			// TODO: URIs?
			gchar *out_filename = (get_file ? g_file_get_path(get_file) : nullptr);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gint res = gtk_dialog_run(GTK_DIALOG(dialog));
			gchar *out_filename = (res == GTK_RESPONSE_ACCEPT
				? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))
				: nullptr);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

			g_object_unref(dialog);
			if (!out_filename) {
				// No filename...
				return;
			}

			// Save the previous export directory.
			g_free(page->prevExportDir);
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
				const uint32_t lc = page->cboLanguage
					? language_combo_box_get_selected_lc(LANGUAGE_COMBO_BOX(page->cboLanguage))
					: 0;
				ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(page->romData, lc);
				ofs << ro;
				break;
			}
			case OPTION_EXPORT_JSON: {
				JSONROMOutput jsro(page->romData);
				ofs << jsro << std::endl;
				break;
			}
			case OPTION_COPY_TEXT: {
				const uint32_t lc = page->cboLanguage
					? language_combo_box_get_selected_lc(LANGUAGE_COMBO_BOX(page->cboLanguage))
					: 0;
				ostringstream oss;
				oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(page->romData, lc);
				oss << ro;

				const string str = oss.str();
				rp_gtk_main_clipboard_set_text(str.c_str());
				break;
			}
			case OPTION_COPY_JSON: {
				ostringstream oss;
				JSONROMOutput jsro(page->romData);
				oss << jsro << std::endl;

				const string str = oss.str();
				rp_gtk_main_clipboard_set_text(str.c_str());
				break;
			}
			default:
				assert(!"Invalid action ID.");
				return;
		}
		return;
	}

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = page->romData->romOps();
	assert(id < (int)ops.size());
	if (id >= (int)ops.size()) {
		// ID is out of range.
		return;
	}

	gchar *save_filename = nullptr;
	RomData::RomOpParams params;
	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
#if GTK_CHECK_VERSION(4,0,0)
		// NOTE: GTK4 has *mandatory* overwrite confirmation.
		// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12
		GtkFileChooserNative *const dialog = gtk_file_chooser_native_new(
			op->sfi.title,			// title
			parent,				// parent
			GTK_FILE_CHOOSER_ACTION_SAVE,	// action
			_("Cancel"), _("Save"));
#else /* !GTK_CHECK_VERSION(4,0,0) */
		GtkWidget *const dialog = gtk_file_chooser_dialog_new(
			op->sfi.title,			// title
			parent,				// parent
			GTK_FILE_CHOOSER_ACTION_SAVE,	// action
			_("Cancel"), GTK_RESPONSE_CANCEL,
			_("Save"), GTK_RESPONSE_ACCEPT,
			nullptr);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

		// Set the filters.
		rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(dialog), op->sfi.filter);

		// Add the "All Files" filter.
		GtkFileFilter *const allFilesFilter = gtk_file_filter_new();
		// tr: "All Files" filter (GTK+ file filter)
		gtk_file_filter_set_name(allFilesFilter, C_("RomData", "All Files"));
		gtk_file_filter_add_pattern(allFilesFilter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), allFilesFilter);

		// Initial file and directory, based on the current file.
		// NOTE: Not checking if it's a file or a directory. Assuming it's a file.
		string initialFile = FileSystem::replace_ext(page->romData->filename(), op->sfi.ext);
		if (!initialFile.empty()) {
			// Split the directory and basename.
			size_t slash_pos = initialFile.rfind(DIR_SEP_CHR);
			if (slash_pos != string::npos) {
				// Full path. Set the directory and filename separately.
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), &initialFile[slash_pos + 1]);
				initialFile.resize(slash_pos);

#if GTK_CHECK_VERSION(4,0,0)
				// TODO: URI?
				GFile *const set_file = g_file_new_for_path(initialFile.c_str());
				if (set_file) {
					gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), set_file, nullptr);
					g_object_unref(set_file);
				}
#else /* !GTK_CHECK_VERSION(4,0,0) */
				// FIXME: Do we need to prepend "file://"?
				gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(dialog), initialFile.c_str());
#endif /* GTK_CHECK_VERSION(4,0,0) */
			} else {
				// Not a full path. We can only set the filename.
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), initialFile.c_str());
			}
		}

		// Prompt for a save file.
#if GTK_CHECK_VERSION(4,0,0)
		// GTK4 no longer supports blocking dialogs.
		// FIXME for GTK4: Rewrite to use gtk_window_set_modal() and handle the "response" signal.
		// This will also work for older GTK+.
		assert(!"gtk_dialog_run() is not available in GTK4; needs a rewrite!");
		GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

		// TODO: URIs?
		save_filename = (get_file ? g_file_get_path(get_file) : nullptr);
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gint res = gtk_dialog_run(GTK_DIALOG(dialog));
		save_filename = (res == GTK_RESPONSE_ACCEPT
			? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog))
			: nullptr);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

		g_object_unref(dialog);
		params.save_filename = save_filename;
	}

	GtkMessageType messageType;
	int ret = page->romData->doRomOp(id, &params);
	g_free(save_filename);
	if (ret == 0) {
		// ROM operation completed.

		// Update fields.
		for (int fieldIdx : params.fieldIdx) {
			rom_data_view_update_field(page, fieldIdx);
		}

		// Update the RomOp menu entry in case it changed.
		// NOTE: Assuming the RomOps vector order hasn't changed.
		ops = page->romData->romOps();
		assert(id < (int)ops.size());
		if (id < (int)ops.size()) {
			options_menu_button_update_op(OPTIONS_MENU_BUTTON(page->btnOptions), id, &ops[id]);
		}

		messageType = GTK_MESSAGE_INFO;
	} else {
		// An error occurred...
		// TODO: Show an error message.
		messageType = GTK_MESSAGE_WARNING;
	}

#ifdef ENABLE_MESSAGESOUND
	MessageSound::play(messageType, params.msg.c_str(), GTK_WIDGET(page));
#endif /* ENABLE_MESSAGESOUND */

	if (!params.msg.empty()) {
		// Show the MessageWidget.
		if (!page->messageWidget) {
			page->messageWidget = message_widget_new();
#if GTK_CHECK_VERSION(4,0,0)
			gtk_box_append(GTK_BOX(page), page->messageWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gtk_box_pack_end(GTK_BOX(page), page->messageWidget, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		}

		MessageWidget *const messageWidget = MESSAGE_WIDGET(page->messageWidget);
		message_widget_set_message_type(messageWidget, messageType);
		message_widget_set_text(messageWidget, params.msg.c_str());
#ifdef AUTO_TIMEOUT_MESSAGEWIDGET
		message_widget_show_with_timeout(messageWidget);
#else /* AUTO_TIMEOUT_MESSAGEWIDGET */
		gtk_widget_show(page->messageWidget);
#endif /* AUTO_TIMEOUT_MESSAGEWIDGET */
	}
}

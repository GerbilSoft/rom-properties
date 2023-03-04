/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget.                                 *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RomDataFormat.hpp"

#include "rp-gtk-enums.h"
#include "sort_funcs.h"
#include "is-supported.hpp"

// Custom widgets
#include "DragImage.hpp"
#include "MessageWidget.h"
#include "LanguageComboBox.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
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

// C++ STL classes
using std::set;
using std::string;
using std::unique_ptr;
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

static GParamSpec *props[PROP_LAST];

static void	rp_rom_data_view_set_property	(GObject	*object,
						 guint		 prop_id,
						 const GValue	*value,
						 GParamSpec	*pspec);
static void	rp_rom_data_view_get_property	(GObject	*object,
						 guint		 prop_id,
						 GValue		*value,
						 GParamSpec	*pspec);
static void	rp_rom_data_view_dispose	(GObject	*object);
static void	rp_rom_data_view_finalize	(GObject	*object);

static void	rp_rom_data_view_desc_format_type_changed(RpRomDataView *page,
						 RpDescFormatType desc_format_type);

static void	rp_rom_data_view_init_header_row(RpRomDataView	*page);
static gboolean	rp_rom_data_view_update_display	(RpRomDataView	*page);
static gboolean	rp_rom_data_view_load_rom_data	(RpRomDataView	*page);
static void	rp_rom_data_view_delete_tabs	(RpRomDataView	*page);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler   (GtkCheckButton	*checkbutton,
						     RpRomDataView	*page);
static void	rp_rom_data_view_map_signal_handler (RpRomDataView	*page,
						     gpointer		 user_data);
static void	rp_rom_data_view_unmap_signal_handler(RpRomDataView	*page,
						     gpointer		 user_data);
static void	tree_view_realize_signal_handler    (GtkTreeView	*treeView,
						     RpRomDataView	*page);
static void	cboLanguage_lc_changed_signal_handler(GtkComboBox	*widget,
						     uint32_t		 lc,
						     RpRomDataView	*page);

#if GTK_CHECK_VERSION(3,0,0)
// libadwaita/libhandy function pointers.
// Only initialized if libadwaita/libhandy is linked into the process.
// NOTE: The function pointers are essentially the same, but
// libhandy was renamed to libadwaita for the GTK4 conversion.
// We'll use libadwaita terminology everywhere.
struct AdwHeaderBar;
typedef GType (*pfnGlibGetType_t)(void);
typedef GType (*pfnAdwHeaderBarPackEnd_t)(AdwHeaderBar *self, GtkWidget *child);
static bool has_checked_adw = false;
static pfnGlibGetType_t pfn_adw_deck_get_type = nullptr;
static pfnGlibGetType_t pfn_adw_header_bar_get_type = nullptr;
static pfnAdwHeaderBarPackEnd_t pfn_adw_header_bar_pack_end = nullptr;
#endif /* GTK_CHECK_VERSION(3,0,0) */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpRomDataView, rp_rom_data_view,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0), {});

static void
rp_rom_data_view_class_init(RpRomDataViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_rom_data_view_set_property;
	gobject_class->get_property = rp_rom_data_view_get_property;
	gobject_class->dispose = rp_rom_data_view_dispose;
	gobject_class->finalize = rp_rom_data_view_finalize;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	RFT_BITFIELD_value_quark = g_quark_from_string("RFT_BITFIELD_value");
	RFT_LISTDATA_rows_visible_quark = g_quark_from_string("RFT_LISTDATA_rows_visible");
	RFT_fieldIdx_quark = g_quark_from_string("RFT_fieldIdx");
	RFT_STRING_warning_quark = g_quark_from_string("RFT_STRING_warning");
	RomDataView_romOp_quark = g_quark_from_string("RomDataView.romOp");

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
	/** libadwaita/libhandy **/

	// Check if libadwaita-1 is loaded in the process.
	// TODO: Verify that it is in fact 1.x if symbol versioning isn't available.
	if (!has_checked_adw) {
		has_checked_adw = true;
#  if GTK_CHECK_VERSION(4,0,0)
		// GTK4: libadwaita
		pfn_adw_deck_get_type = (pfnGlibGetType_t)dlvsym(
			RTLD_DEFAULT, "adw_deck_get_type", "LIBADWAITA_1_0");
		if (pfn_adw_deck_get_type) {
			pfn_adw_header_bar_get_type = (pfnGlibGetType_t)dlvsym(
				RTLD_DEFAULT, "adw_header_bar_get_type", "LIBADWAITA_1_0");
			pfn_adw_header_bar_pack_end = (pfnAdwHeaderBarPackEnd_t)dlvsym(
				RTLD_DEFAULT, "adw_header_bar_pack_end", "LIBADWAITA_1_0");
		}
#  else /* !GTK_CHECK_VERSION(4,0,0) */
		// GTK3: libhandy
		pfn_adw_deck_get_type = (pfnGlibGetType_t)dlvsym(
			RTLD_DEFAULT, "hdy_deck_get_type", "LIBHANDY_1_0");
		if (pfn_adw_deck_get_type) {
			pfn_adw_header_bar_get_type = (pfnGlibGetType_t)dlvsym(
				RTLD_DEFAULT, "hdy_header_bar_get_type", "LIBHANDY_1_0");
			pfn_adw_header_bar_pack_end = (pfnAdwHeaderBarPackEnd_t)dlvsym(
				RTLD_DEFAULT, "hdy_header_bar_pack_end", "LIBHANDY_1_0");
		}
#  endif /* GTK_CHECK_VERSION(4,0,0) */
	}
#endif /* GTK_CHECK_VERSION(3,0,0) */
}

/**
 * Set the label format type.
 * @param page RomDataView
 * @param label GtkLabel
 * @param desc_format_type Format type
 */
static inline void
set_label_format_type(GtkLabel *label, RpDescFormatType desc_format_type)
{
	PangoAttrList *const attr_lst = pango_attr_list_new();

	// Check if this label has the "Warning" flag set.
	const gboolean is_warning = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(label), RFT_STRING_warning_quark));
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
#else /* !GTK_CHECK_VERSION(3,16,0) */
	// NOTE: GtkMisc is deprecated on GTK+ 3.x, but it's
	// needed for proper text alignment when using
	// GtkSizeGroup prior to GTK+ 3.16.
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
#endif /* GTK_CHECK_VERSION(3,16,0) */

	gtk_label_set_attributes(label, attr_lst);
	pango_attr_list_unref(attr_lst);
}

static void
rp_rom_data_view_init(RpRomDataView *page)
{
	// g_object_new() guarantees that all values are initialized to 0.

	// Initialize C++ objects.
	page->cxx = new _RpRomDataViewCxx();

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
	gtk_widget_set_name(page->hboxHeaderRow_outer, "hboxHeaderRow_outer");
	gtk_widget_hide(page->hboxHeaderRow_outer);	// GTK4 shows widgets by default.

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_name(page->hboxHeaderRow, "hboxHeaderRow");
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
	gtk_widget_set_name(page->hboxHeaderRow_outer, "hboxHeaderRow_outer");

	// Center-align the header row.
	GtkWidget *centerAlign = gtk_alignment_new(0.5f, 0.0f, 1.0f, 0.0f);
	gtk_widget_set_name(centerAlign, "centerAlign");
	gtk_widget_show(centerAlign);

	// Header row. (inner box)
	page->hboxHeaderRow = gtk_hbox_new(false, 8);
	gtk_widget_set_name(page->hboxHeaderRow, "hboxHeaderRow");
	gtk_widget_show(page->hboxHeaderRow);

	gtk_box_pack_start(GTK_BOX(page), page->hboxHeaderRow_outer, false, false, 0);
	gtk_box_pack_start(GTK_BOX(page->hboxHeaderRow_outer), centerAlign, true, false, 0);
	gtk_container_add(GTK_CONTAINER(page->hboxHeaderRow_outer), page->hboxHeaderRow);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// System information.
	page->lblSysInfo = gtk_label_new(nullptr);
	gtk_widget_set_name(page->lblSysInfo, "lblSysInfo");
	gtk_label_set_justify(GTK_LABEL(page->lblSysInfo), GTK_JUSTIFY_CENTER);
	gtk_widget_show(page->lblSysInfo);

	// Banner and icon.
	page->imgBanner = rp_drag_image_new();
	gtk_widget_set_name(page->imgBanner, "imgBanner");
	page->imgIcon = rp_drag_image_new();
	gtk_widget_set_name(page->imgIcon, "imgIcon");

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
	PangoAttrList *const attr_lst = pango_attr_list_new();
	pango_attr_list_insert(attr_lst,
		pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	gtk_label_set_attributes(GTK_LABEL(page->lblSysInfo), attr_lst);
	pango_attr_list_unref(attr_lst);

	// Connect the "map" and "unmap" signals.
	// These are needed in order to start and stop the animation.
	g_signal_connect(page, "map",
		G_CALLBACK(rp_rom_data_view_map_signal_handler), nullptr);
	g_signal_connect(page, "unmap",
		G_CALLBACK(rp_rom_data_view_unmap_signal_handler), nullptr);

	// Table layout is created in rp_rom_data_view_update_display().
}

static void
rp_rom_data_view_dispose(GObject *object)
{
	RpRomDataView *const page = RP_ROM_DATA_VIEW(object);

	// Unregister changed_idle.
	g_clear_handle_id(&page->changed_idle, g_source_remove);

	// Delete the icon frames and tabs.
	rp_rom_data_view_delete_tabs(page);

	// Call the superclass dispose() function.
	G_OBJECT_CLASS(rp_rom_data_view_parent_class)->dispose(object);
}

static void
rp_rom_data_view_finalize(GObject *object)
{
	RpRomDataView *const page = RP_ROM_DATA_VIEW(object);

	// Delete the C++ objects.
	delete page->cxx;

	// Free the strings.
	g_free(page->prevExportDir);
	g_free(page->uri);

	// Unreference romData.
	UNREF(page->romData);

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_rom_data_view_parent_class)->finalize(object);
}

GtkWidget*
rp_rom_data_view_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_ROM_DATA_VIEW, nullptr));
}

GtkWidget*
rp_rom_data_view_new_with_uri(const gchar *uri, RpDescFormatType desc_format_type)
{
	return rp_rom_data_view_new_with_romData(uri, nullptr, desc_format_type);
}

GtkWidget*
rp_rom_data_view_new_with_romData(const gchar *uri, RomData *romData, RpDescFormatType desc_format_type)
{
	// At least URI needs to be set.
	assert(uri != nullptr);

	RpRomDataView *const page = static_cast<RpRomDataView*>(g_object_new(RP_TYPE_ROM_DATA_VIEW, nullptr));
	page->desc_format_type = desc_format_type;
	if (uri) {
		page->uri = g_strdup(uri);
		if (romData) {
			page->romData = romData->ref();
		}
	}

	if (G_LIKELY(romData != nullptr)) {
		// NOTE: Don't call rp_rom_data_view_load_rom_data() because that will
		// close and reopen romData, which wastes CPU cycles.
		// Call rp_rom_data_view_update_display() instead.
		page->changed_idle = g_idle_add(G_SOURCE_FUNC(rp_rom_data_view_update_display), page);
		page->hasCheckedAchievements = false;
	} else if (G_LIKELY(uri != nullptr)) {
		// URI is specified, but not RomData.
		// We'll need to create a RomData object.
		page->changed_idle = g_idle_add(G_SOURCE_FUNC(rp_rom_data_view_load_rom_data), page);
	}

	return reinterpret_cast<GtkWidget*>(page);
}

/** Properties **/

static void
rp_rom_data_view_set_property(GObject	*object,
			   guint	 prop_id,
			   const GValue	*value,
			   GParamSpec	*pspec)
{
	RpRomDataView *const page = RP_ROM_DATA_VIEW(object);

	switch (prop_id) {
		case PROP_URI:
			rp_rom_data_view_set_uri(page, g_value_get_string(value));
			break;

		case PROP_DESC_FORMAT_TYPE:
			rp_rom_data_view_set_desc_format_type(page,
				static_cast<RpDescFormatType>(g_value_get_enum(value)));
			break;

		case PROP_SHOWING_DATA:	// TODO: "Non-writable property" warning?
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_rom_data_view_get_property(GObject	*object,
			   guint	 prop_id,
			   GValue	*value,
			   GParamSpec	*pspec)
{
	RpRomDataView *const page = RP_ROM_DATA_VIEW(object);

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

/**
 * rp_rom_data_view_get_uri:
 * @page : a #RomDataView.
 *
 * Returns the current URI for the @page.
 *
 * Return value: the file associated with this property page.
 **/
const gchar*
rp_rom_data_view_get_uri(RpRomDataView *page)
{
	g_return_val_if_fail(RP_IS_ROM_DATA_VIEW(page), nullptr);
	return page->uri;
}

/**
 * rp_rom_data_view_set_uri:
 * @page : a #RomDataView.
 * @uri : a URI.
 *
 * Sets the URI for this @page.
 **/
void
rp_rom_data_view_set_uri(RpRomDataView	*page,
		      const gchar	*uri)
{
	g_return_if_fail(RP_IS_ROM_DATA_VIEW(page));

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
		rp_rom_data_view_delete_tabs(page);
	}

	/* Assign the value */
	page->uri = g_strdup(uri);

	/* Connect to the new file (if any) */
	if (G_LIKELY(page->uri != nullptr)) {
		if (page->changed_idle == 0) {
			page->changed_idle = g_idle_add(G_SOURCE_FUNC(rp_rom_data_view_load_rom_data), page);
		}
	} else {
		// Hide the header row. (outerbox)
		if (page->hboxHeaderRow_outer) {
			gtk_widget_hide(page->hboxHeaderRow_outer);
		}
	}

	// URI has been changed.
	// FIXME: If called from rp_rom_data_view_set_property(), this might
	// result in *two* notifications.
	g_object_notify_by_pspec(G_OBJECT(page), props[PROP_URI]);
}

RpDescFormatType
rp_rom_data_view_get_desc_format_type(RpRomDataView *page)
{
	g_return_val_if_fail(RP_IS_ROM_DATA_VIEW(page), RP_DFT_XFCE);
	return page->desc_format_type;
}

void
rp_rom_data_view_set_desc_format_type(RpRomDataView *page, RpDescFormatType desc_format_type)
{
	g_return_if_fail(RP_IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);
	if (desc_format_type == page->desc_format_type) {
		// Nothing to change.
		// NOTE: g_return_if_fail() prints an assertion warning,
		// so we can't use that for this check.
		return;
	}

	// FIXME: If called from rp_rom_data_view_set_property(), this might
	// result in *two* notifications.
	page->desc_format_type = desc_format_type;
	rp_rom_data_view_desc_format_type_changed(page, desc_format_type);
	g_object_notify_by_pspec(G_OBJECT(page), props[PROP_DESC_FORMAT_TYPE]);
}

static void
rp_rom_data_view_desc_format_type_changed(RpRomDataView	*page,
				       RpDescFormatType	desc_format_type)
{
	g_return_if_fail(RP_IS_ROM_DATA_VIEW(page));
	g_return_if_fail(desc_format_type >= RP_DFT_XFCE && desc_format_type < RP_DFT_LAST);

	for (GtkLabel *label : page->cxx->vecDescLabels) {
		set_label_format_type(label, desc_format_type);
	}
}

gboolean
rp_rom_data_view_is_showing_data(RpRomDataView *page)
{
	// FIXME: This was intended to be used to determine if
	// the RomData was valid, but the RomData isn't loaded
	// until idle is processed...
	g_return_val_if_fail(RP_IS_ROM_DATA_VIEW(page), false);
	return (page->romData != nullptr);
}

static void
rp_rom_data_view_init_header_row(RpRomDataView *page)
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
		bool ok = rp_drag_image_set_rp_image(RP_DRAG_IMAGE(page->imgBanner), romData->image(RomData::IMG_INT_BANNER));
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
			bool ok = rp_drag_image_set_icon_anim_data(RP_DRAG_IMAGE(page->imgIcon), romData->iconAnimData());
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = rp_drag_image_set_rp_image(RP_DRAG_IMAGE(page->imgIcon), icon);
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
 * @param str		[in,opt] String data (If nullptr, field data is used)
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_string(RpRomDataView *page,
	const RomFields::Field &field,
	const char *str = nullptr)
{
	// String type.
	GtkWidget *widget = gtk_label_new(nullptr);
	// NOTE: No name for this GtkWidget.
	gtk_label_set_use_underline(GTK_LABEL(widget), false);
	gtk_widget_show(widget);

	if (!str) {
		str = field.data.str;
	}

	if (field.type == RomFields::RFT_STRING &&
	    (field.flags & RomFields::STRF_CREDITS))
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

	// Check for any formatting options. (RFT_STRING only)
	if (field.type == RomFields::RFT_STRING && field.flags != 0) {
		PangoAttrList *const attr_lst = pango_attr_list_new();

		// Monospace font?
		if (field.flags & RomFields::STRF_MONOSPACE) {
			pango_attr_list_insert(attr_lst,
				pango_attr_family_new("monospace"));
		}

		// "Warning" font?
		if (field.flags & RomFields::STRF_WARNING) {
			pango_attr_list_insert(attr_lst,
				pango_attr_weight_new(PANGO_WEIGHT_BOLD));
			pango_attr_list_insert(attr_lst,
				pango_attr_foreground_new(65535, 0, 0));
		}

		gtk_label_set_attributes(GTK_LABEL(widget), attr_lst);
		pango_attr_list_unref(attr_lst);

		if (field.flags & RomFields::STRF_CREDITS) {
			// Credits row goes at the end.
			// There should be a maximum of one STRF_CREDITS per tab.
			auto &tab = page->cxx->tabs.at(field.tabIdx);
			assert(tab.lblCredits == nullptr);
			tab.lblCredits = widget;

			// Credits row.
#if GTK_CHECK_VERSION(4,0,0)
			gtk_box_append(GTK_BOX(tab.vbox), widget);
			gtk_widget_set_valign(tab.vbox, GTK_ALIGN_END);
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
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_bitfield(RpRomDataView *page,
	const RomFields::Field &field)
{
	// Bitfield type. Create a grid of checkboxes.
	// TODO: Description label needs some padding on the top...
	const auto &bitfieldDesc = field.desc.bitfield;
	assert(bitfieldDesc.names->size() <= 32);

#ifdef USE_GTK_GRID
	GtkWidget *widget = gtk_grid_new();
	// NOTE: No name for this GtkWidget.
	//gtk_grid_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_grid_set_column_spacings(GTK_TABLE(widget), 8);
#else /* !USE_GTK_GRID */
	int count = (int)bitfieldDesc.names->size();
	if (count > 32)
		count = 32;

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
	// NOTE: No name for this GtkWidget.
	//gtk_table_set_row_spacings(GTK_TABLE(widget), 2);
	//gtk_table_set_col_spacings(GTK_TABLE(widget), 8);
#endif /* USE_GTK_GRID */
	gtk_widget_show(widget);

	int row = 0, col = 0;
	uint32_t bitfield = field.data.bitfield;
	for (const string &name : *(bitfieldDesc.names)) {
		if (name.empty()) {
			bitfield >>= 1;
			continue;
		}

		GtkWidget *const checkBox = gtk_check_button_new_with_label(name.c_str());
		// NOTE: No name for this GtkWidget.
		gtk_widget_show(checkBox);
		gboolean value = (bitfield & 1);
		gtk_check_button_set_active(GTK_CHECK_BUTTON(checkBox), value);

		// Save the bitfield checkbox's value in the GObject.
		g_object_set_qdata(G_OBJECT(checkBox), RFT_BITFIELD_value_quark,
			GUINT_TO_POINTER((guint)value));

		// Disable user modifications.
		// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
		// emitted for both user and program modifications, so we have to
		// connect this signal *after* setting the initial value.
		g_signal_connect(checkBox, "toggled",
			G_CALLBACK(checkbox_no_toggle_signal_handler), page);

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

		bitfield >>= 1;
	}

	return widget;
}

/**
 * Initialize a list data field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_listdata(RpRomDataView *page,
	const RomFields::Field &field)
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
	assert(!list_data->empty());
	if (!list_data || list_data->empty()) {
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
			// Checkbox column.
			gtk_list_store_set(listStore, &treeIter, 0, (checkboxes & 1), -1);
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
			for (const string &str : data_row) {
				gtk_list_store_set(listStore, &treeIter, col, str.c_str(), -1);
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
#endif /* GTK_CHECK_VERSION */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scrolledWindow);

	// Sort proxy model for the GtkListStore.
	GtkTreeModel *sortProxy = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(listStore));

	// Create the GtkTreeView.
	GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sortProxy));
	// NOTE: No name for this GtkWidget.
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView),
		(listDataDesc.names != nullptr));
	gtk_widget_show(treeView);

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
	g_object_set_qdata(G_OBJECT(treeView), RFT_LISTDATA_rows_visible_quark,
		GINT_TO_POINTER(listDataDesc.rows_visible));
	if (listDataDesc.rows_visible > 0) {
		g_signal_connect(treeView, "realize", G_CALLBACK(tree_view_realize_signal_handler), page);
	}

	if (isMulti) {
		page->cxx->vecListDataMulti.emplace_back(listStore, GTK_TREE_VIEW(treeView), &field);
	}

	return scrolledWindow;
}

/**
 * Initialize a Date/Time field.
 * @param page	[in] RomDataView object
 * @param field	[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_datetime(RpRomDataView *page,
	const RomFields::Field &field)
{
	GtkWidget *widget;

	gchar *const str = rom_data_format_datetime(field.data.date_time, field.flags);
	if (str) {
		widget = rp_rom_data_view_init_string(page, field, str);
		g_free(str);
	} else {
		// tr: Invalid date/time.
		widget = rp_rom_data_view_init_string(page, field, C_("RomDataView", "Unknown"));
	}

	return widget;
}

/**
 * Initialize an Age Ratings field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_age_ratings(RpRomDataView *page,
	const RomFields::Field &field)
{
	const RomFields::age_ratings_t *const age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// tr: No age ratings data.
		return rp_rom_data_view_init_string(page, field, C_("RomDataView", "ERROR"));
	}

	// Convert the age ratings field to a string.
	string str = RomFields::ageRatingsDecode(age_ratings);
	return rp_rom_data_view_init_string(page, field, str.c_str());
}

/**
 * Initialize a Dimensions field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_dimensions(RpRomDataView *page,
	const RomFields::Field &field)
{
	gchar *const str = rom_data_format_dimensions(field.data.dimensions);
	GtkWidget *const widget = rp_rom_data_view_init_string(page, field, str);
	g_free(str);
	return widget;
}

/**
 * Initialize a multi-language string field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
static GtkWidget*
rp_rom_data_view_init_string_multi(RpRomDataView *page,
	const RomFields::Field &field)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	// NOTE 2: The string must be "", not nullptr. Otherwise, it will
	// attempt to use the field's string data, which is invalid.
	GtkWidget *const lblStringMulti = rp_rom_data_view_init_string(page, field, "");
	if (lblStringMulti) {
		page->cxx->vecStringMulti.emplace_back(GTK_LABEL(lblStringMulti), &field);
	}
	return lblStringMulti;
}

/**
 * Update all multi-language fields.
 * @param page		[in] RomDataView object.
 * @param user_lc	[in] User-specified language code.
 */
static void
rp_rom_data_view_update_multi(RpRomDataView *page, uint32_t user_lc)
{
	_RpRomDataViewCxx *const cxx = page->cxx;
	set<uint32_t> set_lc;

	// RFT_STRING_MULTI
	for (const Data_StringMulti_t &vsm : cxx->vecStringMulti) {
		GtkLabel *const lblString = vsm.first;
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
		gtk_label_set_text(lblString, (pStr ? pStr->c_str() : ""));
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
			// If we have checkboxes or icons, start at column 1.
			// Otherwise, start at column 0.
			int listStore_col_start;
			if (pField->flags & (RomFields::RFT_LISTDATA_CHECKBOXES | RomFields::RFT_LISTDATA_ICONS)) {
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
				for (const string &str : *iter_listData) {
					gtk_list_store_set(listStore, &treeIter, col, str.c_str(), -1);
					col++;
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
		gtk_widget_set_name(vboxCboLanguage, "vboxCboLanguage");
		gtk_widget_set_valign(vboxCboLanguage, GTK_ALIGN_START);
		gtk_widget_show(vboxCboLanguage);

#  if GTK_CHECK_VERSION(4,0,0)
		gtk_box_append(GTK_BOX(page->hboxHeaderRow_outer), vboxCboLanguage);
#  else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), vboxCboLanguage, false, false, 0);
#  endif /* GTK_CHECK_VERSION(4,0,0) */
#else /* !GTK_CHECK_VERSION(3,0,0) */
		GtkWidget *const topAlign = gtk_alignment_new(0.5f, 0.0f, 0.0f, 0.0f);
		gtk_widget_set_name(topAlign, "topAlign");
		gtk_box_pack_end(GTK_BOX(page->hboxHeaderRow_outer), topAlign, false, false, 0);
		gtk_widget_show(topAlign);

		GtkWidget *const vboxCboLanguage = gtk_vbox_new(false, 0);
		gtk_widget_set_name(vboxCboLanguage, "vboxCboLanguage");
		gtk_container_add(GTK_CONTAINER(topAlign), vboxCboLanguage);
		gtk_widget_show(vboxCboLanguage);
#endif /* GTK_CHECK_VERSION(3,0,0) */

		// Create the language combobox.
		page->cboLanguage = rp_language_combo_box_new();
		gtk_widget_set_name(page->cboLanguage, "cboLanguage");
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
		rp_language_combo_box_set_lcs(RP_LANGUAGE_COMBO_BOX(page->cboLanguage), vec_lc.data());

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
		rp_language_combo_box_set_selected_lc(RP_LANGUAGE_COMBO_BOX(page->cboLanguage), lc_to_set);

		// Connect the "lc-changed" signal.
		g_signal_connect(page->cboLanguage, "lc-changed", G_CALLBACK(cboLanguage_lc_changed_signal_handler), page);
	}
}

static void
rp_rom_data_view_create_options_button(RpRomDataView *page)
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
	parent = gtk_widget_get_parent(parent);

#if GTK_CHECK_VERSION(3,0,0)
	bool isLibAdwaita = false;
	if (!GTK_IS_DIALOG(parent)) {
		// NOTE: As of Nautilus 40, there may be an AdwDeck/HdyDeck here.
		// Check if it's AdwDeck/HdyDeck using dynamically-loaded function pointers.
		if (pfn_adw_deck_get_type && G_OBJECT_TYPE(parent) == pfn_adw_deck_get_type()) {
			// Get the next parent widget.
			isLibAdwaita = true;
			parent = gtk_widget_get_parent(parent);
		}
	}
	if (isLibAdwaita) {
		// Main window is based on AdwWindow/HdyWindow, which is derived from
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

	// Create the RpOptionsMenuButton.
	page->btnOptions = rp_options_menu_button_new();
	gtk_widget_set_name(page->btnOptions, "btnOptions");
	gtk_widget_hide(page->btnOptions);
	rp_options_menu_button_set_direction(RP_OPTIONS_MENU_BUTTON(page->btnOptions), GTK_ARROW_UP);

#if GTK_CHECK_VERSION(3,0,0)
	if (isLibAdwaita) {
		// LibAdwaita/LibHandy version doesn't use GtkDialog.
	} else
#endif /* GTK_CHECK_VERSION(3,0,0) */
	{
		// Not using LibAdwaita/LibHandy, so add the widget to the GtkDialog.
		gtk_dialog_add_action_widget(GTK_DIALOG(parent), page->btnOptions, GTK_RESPONSE_NONE);

		// Disconnect the "clicked" signal from the default GtkDialog response handler.
		// NOTE: May be "activate" now that RpOptionsMenuButton no longer derives from
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

#if GTK_CHECK_VERSION(3,11,5)
	GtkWidget *headerBar = nullptr;
	if (isLibAdwaita) {
		// Nautilus 40 uses libadwaita/libhandy, which has a different arrangement of widgets.
		// NautilusPropertiesWindow
		// |- GtkBox
		//    |- AdwHeaderBar/HdyHeaderBar
		GtkWidget *const gtkBox = gtk_widget_get_first_child(parent);
		if (gtkBox && GTK_IS_BOX(gtkBox)) {
			GtkWidget *const adwHeaderBar = gtk_widget_get_first_child(gtkBox);
			if (adwHeaderBar && G_OBJECT_TYPE(adwHeaderBar) == pfn_adw_header_bar_get_type())
			{
				// Found the HdyHeaderBar.
				// Pack the Options button at the end.
				// NOTE: No type checking here...
				pfn_adw_header_bar_pack_end((AdwHeaderBar*)adwHeaderBar, page->btnOptions);
				headerBar = adwHeaderBar;
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
		rp_options_menu_button_set_direction(RP_OPTIONS_MENU_BUTTON(page->btnOptions), GTK_ARROW_DOWN);
	} else
#endif /* GTK_CHECK_VERSION(3,11,5) */
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

	// Connect the RpOptionsMenuButton's triggered(int) signal.
	g_signal_connect(page->btnOptions, "triggered", G_CALLBACK(btnOptions_triggered_signal_handler), page);

	// Initialize the menu options.
	rp_options_menu_button_reinit_menu(RP_OPTIONS_MENU_BUTTON(page->btnOptions), page->romData);
}

/**
 * Update the display widgets.
 * @param page RomDataView
 * @return G_SOURCE_REMOVE
 */
static gboolean
rp_rom_data_view_update_display(RpRomDataView *page)
{
	g_return_val_if_fail(RP_IS_ROM_DATA_VIEW(page), G_SOURCE_REMOVE);

	// Delete the icon frames and tabs.
	rp_rom_data_view_delete_tabs(page);

	// Create the "Options" button.
	if (!page->btnOptions) {
		rp_rom_data_view_create_options_button(page);
	}

	// Initialize the header row.
	rp_rom_data_view_init_header_row(page);

	if (!page->romData) {
		// No ROM data...
		page->changed_idle = 0;
		return G_SOURCE_REMOVE;
	}

	// Get the fields.
	const RomFields *const pFields = page->romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		page->changed_idle = 0;
		return G_SOURCE_REMOVE;
	}
	int fieldCount = pFields->count();

	// Create the GtkNotebook.
	auto &tabs = page->cxx->tabs;
	int tabCount = pFields->tabCount();
	if (tabCount > 1) {
		tabs.resize(tabCount);
		page->tabWidget = gtk_notebook_new();
		gtk_widget_set_name(page->tabWidget, "tabWidget");
#if GTK_CHECK_VERSION(2,91,1)
		gtk_widget_set_halign(page->tabWidget, GTK_ALIGN_FILL);
		gtk_widget_set_valign(page->tabWidget, GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(page->tabWidget, true);
		gtk_widget_set_vexpand(page->tabWidget, true);
#endif /* GTK_CHECK_VERSION(2,91,1) */

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

			tab.vbox = rp_gtk_vbox_new(8);
			char tab_name[32];
			snprintf(tab_name, sizeof(tab_name), "vboxTab%d", i);
			gtk_widget_set_name(tab.vbox, tab_name);
#if USE_GTK_GRID
			tab.table = gtk_grid_new();
			gtk_grid_set_row_spacing(GTK_GRID(tab.table), 2);
			gtk_grid_set_column_spacing(GTK_GRID(tab.table), 8);
#else /* !USE_GTK_GRID */
			// FIXME: Adjust the table size for the number of fields on this tab.
			tab.table = gtk_table_new(fieldCount, 2, false);
			gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
			gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif /* USE_GTK_GRID */
			snprintf(tab_name, sizeof(tab_name), "tableTab%d", i);
			gtk_widget_set_name(tab.table, tab_name);

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
			snprintf(tab_name, sizeof(tab_name), "lblTab%d", i);
			gtk_widget_set_name(label, tab_name);
			gtk_notebook_append_page(GTK_NOTEBOOK(page->tabWidget), tab.vbox, label);
		}
		gtk_widget_show(page->tabWidget);

#if GTK_CHECK_VERSION(4,0,0)
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
		// FIXME: Adjust the table size for the number of fields on this tab.
		tab.table = gtk_table_new(fieldCount, 2, false);
		gtk_table_set_row_spacings(GTK_TABLE(tab.table), 2);
		gtk_table_set_col_spacings(GTK_TABLE(tab.table), 8);
#endif /* USE_GTK_GRID */
		gtk_widget_set_name(tab.table, "tableTab0");
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
	page->cxx->vecDescLabels.reserve(fieldCount);
	// Per-tab row counts.
	unique_ptr<int[]> tabRowCount(new int[tabs.size()]());

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");

	// Create the data widgets.
	int fieldIdx = 0;
	const auto pFields_cend = pFields->cend();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, fieldIdx++) {
		const RomFields::Field &field = *iter;
		assert(field.isValid());
		if (!field.isValid())
			continue;

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0);
		assert(tabIdx < static_cast<int>(tabs.size()));
		if (tabIdx < 0 || tabIdx >= static_cast<int>(tabs.size())) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs[tabIdx].table) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		GtkWidget *widget = nullptr;
		bool separate_rows = false;
		switch (field.type) {
			case RomFields::RFT_INVALID:
				// Should not happen due to the above check...
				assert(!"Field type is RFT_INVALID");
				break;
			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;

			case RomFields::RFT_STRING:
				widget = rp_rom_data_view_init_string(page, field);
				break;
			case RomFields::RFT_BITFIELD:
				widget = rp_rom_data_view_init_bitfield(page, field);
				break;
			case RomFields::RFT_LISTDATA:
				separate_rows = !!(field.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW);
				widget = rp_rom_data_view_init_listdata(page, field);
				break;
			case RomFields::RFT_DATETIME:
				widget = rp_rom_data_view_init_datetime(page, field);
				break;
			case RomFields::RFT_AGE_RATINGS:
				widget = rp_rom_data_view_init_age_ratings(page, field);
				break;
			case RomFields::RFT_DIMENSIONS:
				widget = rp_rom_data_view_init_dimensions(page, field);
				break;
			case RomFields::RFT_STRING_MULTI:
				widget = rp_rom_data_view_init_string_multi(page, field);
				break;
		}

		if (!widget) {
			// No widget. Continue to the next field.
			continue;
		}

		// Set the widget's RFT_fieldIdx property.
		// NOTE: RFT_STRING fields with STRF_CREDITS won't have this set.
		g_object_set_qdata(G_OBJECT(widget), RFT_fieldIdx_quark, GINT_TO_POINTER(fieldIdx+1));

		// Add the widget to the table.
		auto &tab = tabs[tabIdx];

		// tr: Field description label.
		const string txt = rp_sprintf(desc_label_fmt, field.name);
		GtkWidget *const lblDesc = gtk_label_new(txt.c_str());
		// NOTE: No name for this GtkWidget.
		gtk_label_set_use_underline(GTK_LABEL(lblDesc), false);
		set_label_format_type(GTK_LABEL(lblDesc), page->desc_format_type);
		gtk_widget_show(lblDesc);
		page->cxx->vecDescLabels.emplace_back(GTK_LABEL(lblDesc));

		// Check if this is an RFT_STRING with warning set.
		// If it is, set the "RFT_STRING_warning" flag.
		const guint is_warning = ((field.type == RomFields::RFT_STRING) &&
		                          (field.flags & RomFields::STRF_WARNING));
		g_object_set_qdata(G_OBJECT(lblDesc), RFT_STRING_warning_quark,
			GUINT_TO_POINTER((guint)is_warning));

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
				g_object_set_qdata(G_OBJECT(widget), RFT_LISTDATA_rows_visible_quark,
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
				// NOTE: No name for this GtkWidget.
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
				fieldCount++;
				gtk_table_resize(GTK_TABLE(tab.table), fieldCount, 2);
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

	// Initial update of RFT_STRING_MULTI and RFT_LISTDATA_MULTI fields.
	if (!page->cxx->vecStringMulti.empty() || !page->cxx->vecListDataMulti.empty()) {
		page->cxx->def_lc = pFields->defaultLanguageCode();
		rp_rom_data_view_update_multi(page, 0);
	}

	page->changed_idle = 0;
	return G_SOURCE_REMOVE;
}

/**
 * Reload the RomData object from the current URI.
 * Call this function using g_idle_add().
 * @param page RomDataView
 * @return G_SOURCE_REMOVE
 */
static gboolean
rp_rom_data_view_load_rom_data(RpRomDataView *page)
{
	g_return_val_if_fail(RP_IS_ROM_DATA_VIEW(page), G_SOURCE_REMOVE);

	if (G_UNLIKELY(page->uri == nullptr)) {
		// No URI or RomData.
		// TODO: Remove widgets?
		page->changed_idle = 0;
		return G_SOURCE_REMOVE;
	}

	// Do we have a RomData object loaded already?
	if (page->romData) {
		// Unload the existing RomData object.
		UNREF_AND_NULL_NOCHK(page->romData);
		page->hasCheckedAchievements = false;
		g_object_notify_by_pspec(G_OBJECT(page), props[PROP_SHOWING_DATA]);
	}

	// Load the specified URI.
	RomData *const romData = rp_gtk_open_uri(page->uri);
	if (romData) {
		// FIXME: If called from rp_rom_data_view_set_property(), this might
		// result in *two* notifications.
		page->romData = romData;
		page->hasCheckedAchievements = false;
		g_object_notify_by_pspec(G_OBJECT(page), props[PROP_SHOWING_DATA]);
	}

	if (page->romData) {
		// Update the display widgets.
		// TODO: If already mapped, check achievements again.
		// NOTE: This will clear page->changed_idle.
		rp_rom_data_view_update_display(page);

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
 * @param page RomDataView
 */
static void
rp_rom_data_view_delete_tabs(RpRomDataView *page)
{
	assert(page != nullptr);
	assert(page->cxx != nullptr);
	_RpRomDataViewCxx *const cxx = page->cxx;

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
 * @param checkbutton Bitfield checkbox
 * @param page RomDataView
 */
static void
checkbox_no_toggle_signal_handler(GtkCheckButton *checkbutton, RpRomDataView *page)
{
	if (page->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved RFT_BITFIELD value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(checkbutton), RFT_BITFIELD_value_quark));
	if (gtk_check_button_get_active(checkbutton) != value) {
		// Toggle this box.
		gtk_check_button_set_active(checkbutton, value);
	}
}

/**
 * RomDataView is being mapped onto the screen.
 * @param page RomDataView
 * @param user_data User data
 */
static void
rp_rom_data_view_map_signal_handler(RpRomDataView	*page,
				    gpointer	 	 user_data)
{
	RP_UNUSED(user_data);
	rp_drag_image_start_anim_timer(RP_DRAG_IMAGE(page->imgIcon));
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
 * @param user_data User data
 */
static void
rp_rom_data_view_unmap_signal_handler(RpRomDataView	*page,
				   gpointer	 user_data)
{
	RP_UNUSED(user_data);
	rp_drag_image_stop_anim_timer(RP_DRAG_IMAGE(page->imgIcon));
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
 * The RFT_MULTI_STRING language was changed.
 * @param widget	GtkComboBox
 * @param lc		Language code
 * @param page		RomDataView
 */
static void
cboLanguage_lc_changed_signal_handler(GtkComboBox *widget,
				      uint32_t     lc,
				      RpRomDataView *page)
{
	RP_UNUSED(widget);
	rp_rom_data_view_update_multi(page, lc);
}

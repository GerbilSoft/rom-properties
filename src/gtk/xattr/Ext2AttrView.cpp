/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * Ext2AttrView.cpp: Ext2 file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Ext2AttrView.hpp"

// Ext2 flags (also used for Ext3, Ext4, and other Linux file systems)
#include "librpfile/xattr/ext2_flags.h"

// Ext2AttrData
#include "librpfile/xattr/Ext2AttrData.h"

// librpfile
using LibRpFile::XAttrReader;

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_FLAGS,
	PROP_ZALGORITHM,

	PROP_LAST
} RpExt2AttrViewPropID;

static void	rp_ext2_attr_view_set_property(GObject		*object,
					       guint		 prop_id,
					       const GValue	*value,
					       GParamSpec	*pspec);
static void	rp_ext2_attr_view_get_property(GObject		*object,
					       guint		 prop_id,
					       GValue		*value,
					       GParamSpec	*pspec);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
						  RpExt2AttrView	*widget);

/** Update flags display **/
static void	rp_ext2_attr_view_update_flags_string(RpExt2AttrView *widget);
static void	rp_ext2_attr_view_update_flags_checkboxes(RpExt2AttrView *widget);
static void	rp_ext2_attr_view_update_flags_display(RpExt2AttrView *widget);

static GParamSpec *props[PROP_LAST];

static GQuark Ext2AttrView_value_quark;

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

// Ext2AttrView class
struct _RpExt2AttrViewClass {
	superclass __parent__;
};

// Ext2AttrView instance
struct _RpExt2AttrView {
	super __parent__;

	int flags;
	XAttrReader::ZAlgorithm zAlgorithm;

	// Inhibit checkbox toggling while updating.
	gboolean inhibit_checkbox_no_toggle;

	// lsattr-style attributes label
	GtkWidget *lblLsAttr;

	// Compression label
	GtkWidget *lblCompression;

	// See enum CheckboxID and checkboxInfo.
	GtkWidget *checkBoxes[EXT2_ATTR_CHECKBOX_MAX];
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpExt2AttrView, rp_ext2_attr_view,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_ext2_attr_view_class_init(RpExt2AttrViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_ext2_attr_view_set_property;
	gobject_class->get_property = rp_ext2_attr_view_get_property;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	Ext2AttrView_value_quark = g_quark_from_string("Ext2AttrValue.value");

	/** Properties **/

	props[PROP_FLAGS] = g_param_spec_int(
		"flags", "Flags", "Ext2 file system file attributes",
		G_MININT, G_MAXINT, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// TODO: Enum?
	props[PROP_ZALGORITHM] = g_param_spec_uint(
		"zalgorithm", "zAlgorithm", "Compression algorithm",
		0, G_MAXUINT, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_ext2_attr_view_init(RpExt2AttrView *widget)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

	/** lsattr **/
	GtkWidget *const hboxLsAttr = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxLsAttr, "hboxLsAttr");
	GtkWidget *const lblLsAttrDesc = gtk_label_new(C_("Ext2AttrView", "lsattr:"));
	gtk_widget_set_name(lblLsAttrDesc, "lblLsAttrDesc");
	widget->lblLsAttr = gtk_label_new("----------------------");
	gtk_widget_set_name(widget->lblLsAttr, "lblLsAttr");

	// Monospace font for lblLsAttr
	PangoAttrList *const attr_lst = pango_attr_list_new();
	pango_attr_list_insert(attr_lst, pango_attr_family_new("monospace"));
	gtk_label_set_attributes(GTK_LABEL(widget->lblLsAttr), attr_lst);
	pango_attr_list_unref(attr_lst);

	// Compression
	widget->lblCompression = gtk_label_new("Compression:");
	gtk_widget_set_name(widget->lblCompression, "lblCompression");

	// Checkboxes
	int col = 0, row = 0;
	static const int col_count = 4;
#ifdef USE_GTK_GRID
	GtkWidget *const gridCheckboxes = gtk_grid_new();
#else /* !USE_GTK_GRID */
	static const int row_count = (EXT2_ATTR_CHECKBOX_MAX / col_count) +
				    ((EXT2_ATTR_CHECKBOX_MAX % col_count) > 0);
	GtkWidget *const gridCheckboxes = gtk_table_new(row_count, col_count, FALSE);
#endif /* USE_GTK_GRID */
	gtk_widget_set_name(gridCheckboxes, "gridCheckboxes");

	// tr: format string for Ext2 attribute checkbox labels (%c == lsattr character)
	const char *const s_lsattr_fmt = C_("Ext2AttrView", "%c: %s");

	for (size_t i = 0; i < ARRAY_SIZE(widget->checkBoxes); i++) {
		const Ext2AttrCheckboxInfo_t *const p = ext2AttrCheckboxInfo((Ext2AttrCheckboxID)i);

		// Prepend the lsattr character to the checkbox label.
		char buf[256];
		snprintf(buf, sizeof(buf), s_lsattr_fmt, p->lsattr_chr,
			pgettext_expr("Ext2AttrView", p->label));

		GtkWidget *const checkBox = gtk_check_button_new_with_label(buf);
		gtk_widget_set_name(checkBox, p->name);
		gtk_widget_set_tooltip_text(checkBox,
			pgettext_expr("Ext2AttrView", p->tooltip));

		widget->checkBoxes[i] = checkBox;

#ifdef USE_GTK_GRID
		gtk_grid_attach(GTK_GRID(gridCheckboxes), checkBox, col, row, 1, 1);
#else /* !USE_GTK_GRID */
		gtk_table_attach(GTK_TABLE(gridCheckboxes), checkBox, col, col+1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
#endif /* USE_GTK_GRID */

		// Disable user modifications.
		// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
		// emitted for both user and program modifications, so we have to
		// connect this signal *after* setting the initial value.
		g_signal_connect(checkBox, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);

		// Next cell.
		col++;
		if (col == col_count) {
			col = 0;
			row++;
		}
	}

	// TODO: lblCompression should be right-aligned.
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_box_append(GTK_BOX(hboxLsAttr), lblLsAttrDesc);
	gtk_box_append(GTK_BOX(hboxLsAttr), widget->lblLsAttr);
	gtk_box_append(GTK_BOX(hboxLsAttr), widget->lblCompression);
	gtk_box_append(GTK_BOX(widget), hboxLsAttr);
	gtk_box_append(GTK_BOX(widget), gridCheckboxes);

	// Don't show lblCompression by default.
	gtk_widget_set_visible(widget->lblCompression, false);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	gtk_box_pack_start(GTK_BOX(hboxLsAttr), lblLsAttrDesc, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxLsAttr), widget->lblLsAttr, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), hboxLsAttr, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hboxLsAttr), widget->lblCompression, FALSE, FALSE, 0);
	gtk_widget_show_all(hboxLsAttr);

	// Don't show lblCompression by default.
	gtk_widget_hide(widget->lblCompression);

	gtk_box_pack_start(GTK_BOX(widget), gridCheckboxes, FALSE, FALSE, 0);
	gtk_widget_show_all(gridCheckboxes);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */
}

GtkWidget*
rp_ext2_attr_view_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_EXT2_ATTR_VIEW, nullptr));
}

/** Properties **/

static void
rp_ext2_attr_view_set_property(GObject		*object,
				guint		 prop_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	RpExt2AttrView *const widget = RP_EXT2_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_FLAGS:
			rp_ext2_attr_view_set_flags(widget, g_value_get_int(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_ext2_attr_view_get_property(GObject		*object,
				guint		 prop_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	RpExt2AttrView *const widget = RP_EXT2_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_FLAGS:
			g_value_set_int(value, widget->flags);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Update flags display **/

/**
 * Update the flags string display.
 * This uses the same format as e2fsprogs lsattr.
 * @param widget Ext2AttrView
 */
static void
rp_ext2_attr_view_update_flags_string(RpExt2AttrView *widget)
{
	char str[] = "----------------------";
	static_assert(sizeof(str) == 22+1, "str[] is the wrong size");

	// NOTE: This struct uses bit numbers, not masks.
	struct flags_name {
		uint8_t bit;
		char chr;
	};
	static const struct flags_name flags_array[] = {
		{  0, 's' }, {  1, 'u' }, {  3, 'S' }, { 16, 'D' },
		{  4, 'i' }, {  5, 'a' }, {  6, 'd' }, {  7, 'A' },
		{  2, 'c' }, { 11, 'E' }, { 14, 'j' }, { 12, 'I' },
		{ 15, 't' }, { 17, 'T' }, { 19, 'e' }, { 23, 'C' },
		{ 25, 'x' }, { 30, 'F' }, { 28, 'N' }, { 29, 'P' },
		{ 20, 'V' }, { 10, 'm' }
	};

	for (size_t i = 0; i < ARRAY_SIZE(flags_array); i++) {
		if (widget->flags & (1U << flags_array[i].bit)) {
			str[i] = flags_array[i].chr;
		}
	}

	gtk_label_set_text(GTK_LABEL(widget->lblLsAttr), str);
}

/**
 * Update the flags checkboxes.
 * @param widget Ext2AttrView
 */
static void
rp_ext2_attr_view_update_flags_checkboxes(RpExt2AttrView *widget)
{
	static_assert(ARRAY_SIZE(widget->checkBoxes) == EXT2_ATTR_CHECKBOX_MAX,
		"checkBoxes and EXT2_ATTR_CHECKBOX_MAX are out of sync!");

	widget->inhibit_checkbox_no_toggle = TRUE;

	// Flag order, relative to checkboxes
	// NOTE: Uses bit indexes.
	static const uint8_t flag_order[] = {
		 5,  7,  2, 23,  6, 16, 19, 11,
		30,  4, 12, 14, 10, 28, 29,  0,
		 3, 15, 17,  1, 25, 20
	};

	for (size_t i = 0; i < ARRAY_SIZE(widget->checkBoxes); i++) {
		gboolean val = !!(widget->flags & (1U << flag_order[i]));
		gtk_check_button_set_active(GTK_CHECK_BUTTON(widget->checkBoxes[i]), val);
		g_object_set_qdata(G_OBJECT(widget->checkBoxes[i]), Ext2AttrView_value_quark, GUINT_TO_POINTER((guint)val));
	}

	widget->inhibit_checkbox_no_toggle = FALSE;
}

/**
 * Update the flags display.
 * @param widget Ext2AttrView
 */
static void
rp_ext2_attr_view_update_flags_display(RpExt2AttrView *widget)
{
	rp_ext2_attr_view_update_flags_string(widget);
	rp_ext2_attr_view_update_flags_checkboxes(widget);
}

/**
 * Update the compression algorithm label.
 */
static void
rp_ext2_attr_view_update_zAlgorithm_label(RpExt2AttrView *widget)
{
	const char *s_alg;
	switch (widget->zAlgorithm) {
		default:
		case XAttrReader::ZAlgorithm::None:
			// No compression...
			s_alg = nullptr;
			break;
		case XAttrReader::ZAlgorithm::ZLIB:
			s_alg = "zlib";
			break;
		case XAttrReader::ZAlgorithm::LZO:
			s_alg = "lzo";
			break;
		case XAttrReader::ZAlgorithm::ZSTD:
			s_alg = "zstd";
			break;
	}

	if (s_alg) {
		gtk_label_set_text(GTK_LABEL(widget->lblCompression), fmt::format(FSTR("Compression: {:s}"), s_alg).c_str());
		gtk_widget_set_visible(widget->lblCompression, true);
	} else {
		gtk_widget_set_visible(widget->lblCompression, false);
	}
}

/** Property accessors / mutators **/

/**
 * Set the current Ext2 attributes.
 * @param widget Ext2AttrView
 * @param flags Ext2 attributes
 */
void
rp_ext2_attr_view_set_flags(RpExt2AttrView *widget, int flags)
{
	g_return_if_fail(RP_IS_EXT2_ATTR_VIEW(widget));

	if (widget->flags != flags) {
		widget->flags = flags;
		rp_ext2_attr_view_update_flags_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_FLAGS]);
	}
}

/**
 * Get the current Ext2 attributes.
 * @param widget Ext2AttrView
 * @return Ext2 attributes
 */
int
rp_ext2_attr_view_get_flags(RpExt2AttrView *widget)
{
	g_return_val_if_fail(RP_IS_EXT2_ATTR_VIEW(widget), 0);
	return widget->flags;
}

/**
 * Clear the current Ext2 attributes.
 * @param widget Ext2AttrView
 */
void
rp_ext2_attr_view_clear_flags(RpExt2AttrView *widget)
{
	g_return_if_fail(RP_IS_EXT2_ATTR_VIEW(widget));
	if (widget->flags != 0) {
		widget->flags = 0;
		rp_ext2_attr_view_update_flags_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_FLAGS]);
	}
}

/**
 * Set the current compression algorithm.
 * @return Compression algorithm
 */
void
rp_ext2_attr_view_set_zAlgorithm(RpExt2AttrView *widget, XAttrReader::ZAlgorithm zAlgorithm)
{
	if (widget->zAlgorithm != zAlgorithm) {
		widget->zAlgorithm = zAlgorithm;
		rp_ext2_attr_view_update_zAlgorithm_label(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_ZALGORITHM]);
	}
}

/**
 * Set the current compression algorithm.
 * @param zAlgorithm Compression algorithm
 */
XAttrReader::ZAlgorithm
rp_ext2_attr_view_get_zAlgorithm(RpExt2AttrView *widget)
{
	g_return_val_if_fail(RP_IS_EXT2_ATTR_VIEW(widget), XAttrReader::ZAlgorithm::None);
	return widget->zAlgorithm;
}

/**
 * Clear the current compression algorithm.
 */
void
rp_ext2_attr_view_clear_zAlgorithm(RpExt2AttrView *widget)
{
	if (widget->zAlgorithm != XAttrReader::ZAlgorithm::None) {
		widget->zAlgorithm = XAttrReader::ZAlgorithm::None;
		rp_ext2_attr_view_update_zAlgorithm_label(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_ZALGORITHM]);
	}
}

/** Signal handlers **/

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param checkbutton Bitfield checkbox
 * @param page Ext2AttrView
 */
static void
checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
				  RpExt2AttrView	*widget)
{
	if (widget->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved Ext2AttrView value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(checkbutton), Ext2AttrView_value_quark));
	if (gtk_check_button_get_active(checkbutton) != value) {
		// Toggle this box.
		gtk_check_button_set_active(checkbutton, value);
	}
}

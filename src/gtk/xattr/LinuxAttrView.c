/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * LinuxAttrView.c: Linux file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LinuxAttrView.h"

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "librpfile/xattr/ext2_flags.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_FLAGS,

	PROP_LAST
} RpLinuxAttrViewPropID;

static void	rp_linux_attr_view_set_property(GObject		*object,
						guint		 prop_id,
						const GValue	*value,
						GParamSpec	*pspec);
static void	rp_linux_attr_view_get_property(GObject		*object,
						guint		 prop_id,
						GValue		*value,
						GParamSpec	*pspec);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
						  RpLinuxAttrView	*widget);

/** Update flags display **/
static void	rp_linux_attr_view_update_flags_string(RpLinuxAttrView *widget);
static void	rp_linux_attr_view_update_flags_checkboxes(RpLinuxAttrView *widget);
static void	rp_linux_attr_view_update_flags_display(RpLinuxAttrView *widget);

static GParamSpec *props[PROP_LAST];

static GQuark LinuxAttrView_value_quark;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

typedef enum {
	chkAppendOnly,
	chkNoATime,
	chkCompressed,
	chkNoCOW,

	chkNoDump,
	chkDirSync,
	chkExtents,
	chkEncrypted,

	chkCasefold,
	chkImmutable,
	chkIndexed,
	chkJournalled,

	chkNoCompress,
	chkInlineData,
	chkProject,
	chkSecureDelete,

	chkFileSync,
	chkNoTailMerge,
	chkTopDir,
	chkUndelete,

	chkDAX,
	chkVerity,

	CHECKBOX_MAX
} CheckboxID;

// Checkbox info
typedef struct _CheckboxInfo {
	const char *name;	// object name
	const char *label;
	const char *tooltip;
} CheckboxInfo;

static const CheckboxInfo checkboxInfo[CHECKBOX_MAX] = {
	{"chkAppendOnly", NOP_C_("LinuxAttrView", "a: append only"),
	 NOP_C_("LinuxAttrView", "File can only be opened in append mode for writing.")},

	{"chkNoATime", NOP_C_("LinuxAttrView", "A: no atime"),
	 NOP_C_("LinuxAttrView", "Access time record is not modified.")},

	{"chkCompressed", NOP_C_("LinuxAttrView", "c: compressed"),
	 NOP_C_("LinuxAttrView", "File is compressed.")},

	{"chkNoCOW", NOP_C_("LinuxAttrView", "C: no CoW"),
	 NOP_C_("LinuxAttrView", "Not subject to copy-on-write updates.")},

	{"chkNoDump", NOP_C_("LinuxAttrView", "d: no dump"),
	// tr: "dump" is the name of the executable, so it should not be localized.
	 NOP_C_("LinuxAttrView", "This file is not a candidate for dumping with the dump(8) program.")},

	{"chkDirSync", NOP_C_("LinuxAttrView", "D: dir sync"),
	 NOP_C_("LinuxAttrView", "Changes to this directory are written synchronously to the disk.")},

	{"chkExtents", NOP_C_("LinuxAttrView", "e: extents"),
	 NOP_C_("LinuxAttrView", "File is mapped on disk using extents.")},

	{"chkEncrypted", NOP_C_("LinuxAttrView", "E: encrypted"),
	 NOP_C_("LinuxAttrView", "File is encrypted.")},

	{"chkCasefold", NOP_C_("LinuxAttrView", "F: casefold"),
	 NOP_C_("LinuxAttrView", "Files stored in this directory use case-insensitive filenames.")},

	{"chkImmutable", NOP_C_("LinuxAttrView", "i: immutable"),
	 NOP_C_("LinuxAttrView", "File cannot be modified, deleted, or renamed.")},

	{"chkIndexed", NOP_C_("LinuxAttrView", "I: indexed"),
	 NOP_C_("LinuxAttrView", "Directory is indexed using hashed trees.")},

	{"chkJournalled", NOP_C_("LinuxAttrView", "j: journalled"),
	 NOP_C_("LinuxAttrView", "File data is written to the journal before writing to the file itself.")},

	{"chkNoCompress", NOP_C_("LinuxAttrView", "m: no compress"),
	 NOP_C_("LinuxAttrView", "File is excluded from compression.")},

	{"chkInlineData", NOP_C_("LinuxAttrView", "N: inline data"),
	 NOP_C_("LinuxAttrView", "File data is stored inline in the inode.")},

	{"chkProject", NOP_C_("LinuxAttrView", "P: project"),
	 NOP_C_("LinuxAttrView", "Directory will enforce a hierarchical structure for project IDs.")},

	{"chkSecureDelete", NOP_C_("LinuxAttrView", "s: secure del"),
	 NOP_C_("LinuxAttrView", "File's blocks will be zeroed when deleted.")},

	{"chkFileSync", NOP_C_("LinuxAttrView", "S: sync"),
	 NOP_C_("LinuxAttrView", "Changes to this file are written synchronously to the disk.")},

	{"chkNoTailMerge", NOP_C_("LinuxAttrView", "t: no tail merge"),
	 NOP_C_("LinuxAttrView", "If the file system supports tail merging, this file will not have a partial block fragment at the end of the file merged with other files.")},

	{"chkTopDir", NOP_C_("LinuxAttrView", "T: top dir"),
	 NOP_C_("LinuxAttrView", "Directory will be treated like a top-level directory by the ext3/ext4 Orlov block allocator.")},

	{"chkUndelete", NOP_C_("LinuxAttrView", "u: undelete"),
	 NOP_C_("LinuxAttrView", "File's contents will be saved when deleted, potentially allowing for undeletion. This is known to be broken.")},

	{"chkDAX", NOP_C_("LinuxAttrView", "x: DAX"),
	 NOP_C_("LinuxAttrView", "Direct access")},

	{"chkVerity", NOP_C_("LinuxAttrView", "V: fs-verity"),
	 NOP_C_("LinuxAttrView", "File has fs-verity enabled.")},
};

// LinuxAttrView class
struct _RpLinuxAttrViewClass {
	superclass __parent__;
};

// LinuxAttrView instance
struct _RpLinuxAttrView {
	super __parent__;

	int flags;

	// Inhibit checkbox toggling while updating.
	gboolean inhibit_checkbox_no_toggle;

	// lsattr-style attributes label
	GtkWidget *lblLsAttr;

	// See enum CheckboxID and checkboxInfo.
	GtkWidget *checkboxes[CHECKBOX_MAX];
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpLinuxAttrView, rp_linux_attr_view,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_linux_attr_view_class_init(RpLinuxAttrViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_linux_attr_view_set_property;
	gobject_class->get_property = rp_linux_attr_view_get_property;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	LinuxAttrView_value_quark = g_quark_from_string("LinuxAttrValue.value");

	/** Properties **/

	props[PROP_FLAGS] = g_param_spec_int(
		"flags", "Flags", "Linux file system file attributes",
		G_MININT, G_MAXINT, 0,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_linux_attr_view_init(RpLinuxAttrView *widget)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	/** lsattr **/
	GtkWidget *const hboxLsAttr = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxLsAttr, "hboxLsAttr");
	GtkWidget *const lblLsAttrDesc = gtk_label_new(C_("LinuxAttrView", "lsattr:"));
	gtk_widget_set_name(lblLsAttrDesc, "lblLsAttrDesc");
	widget->lblLsAttr = gtk_label_new("----------------------");
	gtk_widget_set_name(widget->lblLsAttr, "lblLsAttr");

	// Monospace font for lblLsAttr
	PangoAttrList *const attr_lst = pango_attr_list_new();
	pango_attr_list_insert(attr_lst, pango_attr_family_new("monospace"));
	gtk_label_set_attributes(GTK_LABEL(widget->lblLsAttr), attr_lst);
	pango_attr_list_unref(attr_lst);

	// Checkboxes
	int col = 0, row = 0;
#ifdef USE_GTK_GRID
	GtkWidget *const gridCheckboxes = gtk_grid_new();
#else /* !USE_GTK_GRID */
	static const int col_count = 4;
	static const int row_count = (ARRAY_SIZE(checkboxInfo) / col_count) +
				    ((ARRAY_SIZE(checkboxInfo) % col_count) > 0);
	GtkWidget *const gridCheckboxes = gtk_table_new(row_count, col_count, FALSE);
#endif /* USE_GTK_GRID */
	gtk_widget_set_name(gridCheckboxes, "gridCheckboxes");
	for (int i = 0; i < CHECKBOX_MAX; i++) {
		const CheckboxInfo *const p = &checkboxInfo[i];

		GtkWidget *const checkBox = gtk_check_button_new_with_label(
			dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", p->label));
		gtk_widget_set_name(checkBox, p->name);
		gtk_widget_set_tooltip_text(checkBox,
			dpgettext_expr(RP_I18N_DOMAIN, "LinuxAttrView", p->tooltip));

		widget->checkboxes[i] = checkBox;

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
		if (col == 4) {
			col = 0;
			row++;
		}
	}

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(hboxLsAttr), lblLsAttrDesc);
	gtk_box_append(GTK_BOX(hboxLsAttr), widget->lblLsAttr);
	gtk_box_append(GTK_BOX(widget), hboxLsAttr);
	gtk_box_append(GTK_BOX(widget), gridCheckboxes);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(hboxLsAttr), lblLsAttrDesc, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxLsAttr), widget->lblLsAttr, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), hboxLsAttr, FALSE, FALSE, 0);
	gtk_widget_show_all(hboxLsAttr);

	gtk_box_pack_start(GTK_BOX(widget), gridCheckboxes, FALSE, FALSE, 0);
	gtk_widget_show_all(gridCheckboxes);
#endif /* GTK_CHECK_VERSION(4,0,0) */
}

GtkWidget*
rp_linux_attr_view_new(void)
{
	return (GtkWidget*)g_object_new(RP_TYPE_LINUX_ATTR_VIEW, NULL);
}

/** Properties **/

static void
rp_linux_attr_view_set_property(GObject		*object,
				guint		 prop_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	RpLinuxAttrView *const widget = RP_LINUX_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_FLAGS: {
			const int flags = g_value_get_int(value);
			if (widget->flags != flags) {
				widget->flags = flags;
				rp_linux_attr_view_update_flags_display(widget);
			}
			break;
		}

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_linux_attr_view_get_property(GObject		*object,
				guint		 prop_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	RpLinuxAttrView *const widget = RP_LINUX_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_FLAGS:
			g_value_set_uint(value, widget->flags);
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
 * @param widget LinuxAttrView
 */
static void
rp_linux_attr_view_update_flags_string(RpLinuxAttrView *widget)
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

	for (int i = 0; i < ARRAY_SIZE_I(flags_array); i++) {
		if (widget->flags & (1 << flags_array[i].bit)) {
			str[i] = flags_array[i].chr;
		}
	}

	gtk_label_set_text(GTK_LABEL(widget->lblLsAttr), str);
}

/**
 * Update the flags checkboxes.
 * @param widget LinuxAttrView
 */
static void
rp_linux_attr_view_update_flags_checkboxes(RpLinuxAttrView *widget)
{
	widget->inhibit_checkbox_no_toggle = TRUE;

	// Flag order, relative to checkboxes
	// NOTE: Uses bit indexes.
	static const uint8_t flag_order[] = {
		 5,  7,  2, 23,  6, 16, 19, 11,
		30,  4, 12, 14, 10, 28, 29,  0,
		 3, 15, 17,  1, 25, 20
	};

	for (int i = 0; i < CHECKBOX_MAX; i++) {
		gboolean val = !!(widget->flags & (1U << flag_order[i]));
		gtk_check_button_set_active(GTK_CHECK_BUTTON(widget->checkboxes[i]), val);
		g_object_set_qdata(G_OBJECT(widget->checkboxes[i]), LinuxAttrView_value_quark, GUINT_TO_POINTER((guint)val));
	}

	widget->inhibit_checkbox_no_toggle = FALSE;
}

/**
 * Update the flags display.
 * @param widget LinuxAttrView
 */
static void
rp_linux_attr_view_update_flags_display(RpLinuxAttrView *widget)
{
	rp_linux_attr_view_update_flags_string(widget);
	rp_linux_attr_view_update_flags_checkboxes(widget);
}

/** Property accessors / mutators **/

/**
 * Set the current Linux attributes.
 * @param widget LinuxAttrView
 * @param flags Linux attributes
 */
void
rp_linux_attr_view_set_flags(RpLinuxAttrView *widget, int flags)
{
	g_return_if_fail(RP_IS_LINUX_ATTR_VIEW(widget));
	if (widget->flags != flags) {
		widget->flags = flags;
		rp_linux_attr_view_update_flags_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_FLAGS]);
	}
}

/**
 * Get the current Linux attributes.
 * @param widget LinuxAttrView
 * @return Linux attributes
 */
int
rp_linux_attr_view_get_flags(RpLinuxAttrView *widget)
{
	g_return_val_if_fail(RP_IS_LINUX_ATTR_VIEW(widget), 0);
	return widget->flags;
}

/**
 * Clear the current Linux attributes.
 * @param widget LinuxAttrView
 */
void
rp_linux_attr_view_clear_flags(RpLinuxAttrView *widget)
{
	g_return_if_fail(RP_IS_LINUX_ATTR_VIEW(widget));
	if (widget->flags != 0) {
		widget->flags = 0;
		rp_linux_attr_view_update_flags_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_FLAGS]);
	}
}

/** Signal handlers **/

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param checkbutton Bitfield checkbox
 * @param page LinuxAttrView
 */
static void
checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
				  RpLinuxAttrView	*widget)
{
	if (widget->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved LinuxAttrView value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(checkbutton), LinuxAttrView_value_quark));
	if (gtk_check_button_get_active(checkbutton) != value) {
		// Toggle this box.
		gtk_check_button_set_active(checkbutton, value);
	}
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * ImageTypesTab.cpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageTypesTab.hpp"
#include "RpConfigTab.h"

#include "RpGtk.hpp"
#include "gtk-compat.h"

// librpbase
using namespace LibRpBase;

// TImageTypesConfig is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/config/ImageTypesConfig.hpp"
#include "libromdata/config/TImageTypesConfig.cpp"
using namespace LibRomData;

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

class RpImageTypesTabPrivate final : public TImageTypesConfig<OurComboBox*>
{
	public:
		explicit RpImageTypesTabPrivate(RpImageTypesTab *q);
		~RpImageTypesTabPrivate() final;

	private:
		RpImageTypesTab *const q;
		RP_DISABLE_COPY(RpImageTypesTabPrivate)

	protected:
		/** TImageTypesConfig functions (protected) **/

		/**
		 * Create the labels in the grid.
		 */
		void createGridLabels(void) final;

		/**
		 * Create a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 */
		void createComboBox(unsigned int cbid) final;

		/**
		 * Add strings to a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 * @param max_prio Maximum priority value. (minimum is 1)
		 */
		void addComboBoxStrings(unsigned int cbid, int max_prio) final;

		/**
		 * Finish adding the ComboBoxes.
		 */
		void finishComboBoxes(void) final;

		/**
		 * Write an ImageType configuration entry.
		 * @param sysName System name.
		 * @param imageTypeList Image type list, comma-separated.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveWriteEntry(const char *sysName, const char *imageTypeList) final;

	protected:
		/** TImageTypesConfig functions (public) **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) final;

	public:
		/** Other RpImageTypesTabPrivate functions **/

		/**
		 * Initialize strings.
		 */
		void initStrings(void);

	public:
		// Last ComboBox added.
		// Needed in order to set the correct
		// tab order for the credits label.
		OurComboBox *cboImageType_lastAdded;

		// Temporary GKeyFile object.
		// Set and cleared by ImageTypesTab::save();
		GKeyFile *keyFile;
};

// ImageTypesTab class
struct _RpImageTypesTabClass {
	superclass __parent__;
};

// ImageTypesTab instance
struct _RpImageTypesTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

	RpImageTypesTabPrivate *d;

	GtkWidget *table;	// GtkGrid/GtkTable
	GtkWidget *lblCredits;
};

static GQuark rp_config_cbid_quark;

static void	rp_image_types_tab_finalize			(GObject		*object);

// Interface initialization
static void	rp_image_types_tab_rp_config_tab_interface_init	(RpConfigTabInterface	*iface);
static gboolean	rp_image_types_tab_has_defaults			(RpImageTypesTab	*tab);
static void	rp_image_types_tab_reset			(RpImageTypesTab	*tab);
static void	rp_image_types_tab_load_defaults		(RpImageTypesTab	*tab);
static void	rp_image_types_tab_save				(RpImageTypesTab	*tab,
								 GKeyFile       	*keyFile);

// "modified" signal handler for UI widgets
#ifdef USE_GTK_DROP_DOWN
static void	rp_image_types_tab_notify_selected_handler	(GtkDropDown		*cbo,
								 GParamSpec		*pspec,
								 RpImageTypesTab	*tab);
#else /* !USE_GTK_DROP_DOWN */
static void	rp_image_types_tab_modified_handler		(GtkComboBox		*cbo,
								 RpImageTypesTab	*tab);
#endif /* USE_GTK_DROP_DOWN */

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpImageTypesTab, rp_image_types_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_image_types_tab_rp_config_tab_interface_init));

/** RpImageTypesTabPrivate **/

RpImageTypesTabPrivate::RpImageTypesTabPrivate(RpImageTypesTab* q)
	: q(q)
	, cboImageType_lastAdded(nullptr)
	, keyFile(nullptr)
{ }

RpImageTypesTabPrivate::~RpImageTypesTabPrivate()
{
	// cboImageType_lastAdded should be nullptr.
	// (Cleared by finishComboBoxes().)
	assert(cboImageType_lastAdded == nullptr);

	// keyFile should be nullptr,
	// since it's only used when saving.
	assert(keyFile == nullptr);
}

/** TImageTypesConfig functions (protected) **/

/**
 * Create the labels in the grid.
 */
void RpImageTypesTabPrivate::createGridLabels(void)
{
	// TODO: Make sure that all columns except 0 have equal sizes.

	// Create the image type labels.
	const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();
	for (unsigned int i = 0; i < imageTypeCount; i++) {
		// TODO: Decrement the column number for >IMG_INT_MEDIA?
		if (i == RomData::IMG_INT_MEDIA) {
			// No INT MEDIA boxes, so eliminate the column.
			continue;
		}

		GtkWidget *const lblImageType = gtk_label_new(imageTypeName(i));
		char lbl_name[32];
		snprintf(lbl_name, sizeof(lbl_name), "lblImageType%u", i);
		gtk_widget_set_name(lblImageType, lbl_name);

#if !GTK_CHECK_VERSION(4,0,0)
		gtk_widget_show(lblImageType);
#endif /* !GTK_CHECK_VERSION(4,0,0) */
		GTK_LABEL_XALIGN_CENTER(lblImageType);
		gtk_label_set_justify(GTK_LABEL(lblImageType), GTK_JUSTIFY_CENTER);
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_set_margin_start(lblImageType, 3);
		gtk_widget_set_margin_end(lblImageType, 3);
		gtk_widget_set_margin_bottom(lblImageType, 4);
#else /* !GTK_CHECK_VERSION(3,0,0) */
		g_object_set(G_OBJECT(lblImageType), "xpad", 3, nullptr);
		g_object_set(G_OBJECT(lblImageType), "ypad", 4, nullptr);
#endif /* GTK_CHECK_VERSION(3,0,0) */

#ifdef USE_GTK_GRID
		gtk_grid_attach(GTK_GRID(q->table), lblImageType, i+1, 0, 1, 1);
#else /* !USE_GTK_GRID */
		gtk_table_attach(GTK_TABLE(q->table), lblImageType, i+1, i+2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
#endif /* USE_GTK_GRID */
	}

	// Create the system name labels.
	const unsigned int sysCount = ImageTypesConfig::sysCount();
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		GtkWidget *const lblSysName = gtk_label_new(sysName(sys));
		char lbl_name[32];
		snprintf(lbl_name, sizeof(lbl_name), "lblSysName%u", sys);
		gtk_widget_set_name(lblSysName, lbl_name);

#if !GTK_CHECK_VERSION(4,0,0)
		gtk_widget_show(lblSysName);
#endif /* !GTK_CHECK_VERSION(4,0,0) */
		GTK_LABEL_XALIGN_LEFT(lblSysName);
#if GTK_CHECK_VERSION(3,0,0)
		gtk_widget_set_margin_end(lblSysName, 6);
#else /* !GTK_CHECK_VERSION(3,0,0) */
		g_object_set(G_OBJECT(lblSysName), "xpad", 6, nullptr);
#endif /* GTK_CHECK_VERSION(3,0,0) */

#ifdef USE_GTK_GRID
		gtk_grid_attach(GTK_GRID(q->table), lblSysName, 0, sys+1, 1, 1);
#else /* !USE_GTK_GRID */
		gtk_table_attach(GTK_TABLE(q->table), lblSysName, 0, 1, sys+1, sys+2, GTK_FILL, GTK_SHRINK, 0, 0);
#endif /* USE_GTK_GRID */
	}
}

/**
 * Create a ComboBox in the grid.
 * @param cbid ComboBox ID.
 */
void RpImageTypesTabPrivate::createComboBox(unsigned int cbid)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	// TODO: Decrement the column number for >IMG_INT_MEDIA?
	if (imageType == RomData::IMG_INT_MEDIA) {
		// No INT MEDIA boxes, so eliminate the column.
		return;
	}

	SysData_t &sysData = v_sysData[sys];

	// Create the ComboBox.
#ifdef USE_GTK_DROP_DOWN
	GtkWidget *const cbo = gtk_drop_down_new(nullptr, nullptr);
#else /* !USE_GTK_DROP_DOWN */
	GtkWidget *const cbo = gtk_combo_box_new();
#endif /* USE_GTK_DROP_DOWN */

	char cbo_name[32];
	snprintf(cbo_name, sizeof(cbo_name), "cbo%04X", cbid);
	gtk_widget_set_name(cbo, cbo_name);

#if !GTK_CHECK_VERSION(4,0,0)
	gtk_widget_show(cbo);
#endif /* !GTK_CHECK_VERSION */
#ifdef USE_GTK_GRID
	gtk_grid_attach(GTK_GRID(q->table), cbo, imageType+1, sys+1, 1, 1);
#else /* !USE_GTK_GRID */
	gtk_table_attach(GTK_TABLE(q->table), cbo, imageType+1, imageType+2, sys+1, sys+2, GTK_SHRINK, GTK_SHRINK, 0, 0);
#endif /* USE_GTK_GRID */
	sysData.cboImageType[imageType] = OUR_COMBO_BOX(cbo);

	// Set the cbid as GObject data.
	g_object_set_qdata(G_OBJECT(cbo), rp_config_cbid_quark, GUINT_TO_POINTER(cbid));

	// Connect the signal handlers for the comboboxes.
	// NOTE: Signal handlers are triggered if the value is
	// programmatically edited, unlike Qt, so we'll need to
	// inhibit handling when loading settings.
#ifdef USE_GTK_DROP_DOWN
	// GtkDropDown doesn't have a "changed" signal, and its
	// GtkSelectionModel object isn't accessible.
	// Listen for GObject::notify for the "selected" property.
	g_signal_connect(cbo, "notify::selected", G_CALLBACK(rp_image_types_tab_notify_selected_handler), q);
#else /* !USE_GTK_DROP_DOWN */
	g_signal_connect(cbo, "changed", G_CALLBACK(rp_image_types_tab_modified_handler), q);
#endif /* USE_GTK_DROP_DOWN */

	// Adjust the tab order. [TODO]
#if 0
	if (cboImageType_lastAdded) {
		q->setTabOrder(cboImageType_lastAdded, cbo);
	}
	cboImageType_lastAdded = cbo;
#endif /* 0 */
}

/**
 * Add strings to a ComboBox in the grid.
 * @param cbid ComboBox ID.
 * @param max_prio Maximum priority value. (minimum is 1)
 */
void RpImageTypesTabPrivate::addComboBoxStrings(unsigned int cbid, int max_prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;
	const SysData_t &sysData = v_sysData[sys];

	OurComboBox *const cbo = sysData.cboImageType[imageType];
	assert(cbo != nullptr);
	if (!cbo)
		return;

	const bool prev_inhibit = q->inhibit;
	q->inhibit = true;

	// NOTE: Need to add one more than the total number,
	// since "No" counts as an entry.
#ifdef USE_GTK_DROP_DOWN
	GtkStringList *const list = gtk_string_list_new(nullptr);
	// tr: Don't use this image type for this particular system.
	gtk_string_list_append(list, C_("ImageTypesTab|Values", "No"));
	for (int i = 1; i <= max_prio; i++) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", i);
		gtk_string_list_append(list, buf);
	}

	gtk_drop_down_set_model(cbo, G_LIST_MODEL(list));
	g_object_unref(list);	// FIXME: may be incorrect
#else /* !USE_GTK_DROP_DOWN */
	assert(max_prio <= static_cast<int>(ImageTypesConfig::imageTypeCount()));
	GtkListStore *const lstCbo = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstCbo, nullptr, 0, 0, C_("ImageTypesTab|Values", "No"), -1);
	for (int i = 1; i <= max_prio; i++) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", i);
		gtk_list_store_insert_with_values(lstCbo, nullptr, i, 0, buf, -1);
	}

	gtk_combo_box_set_model(cbo, GTK_TREE_MODEL(lstCbo));
	g_object_unref(lstCbo);	// TODO: Is this correct?

	// Create the cell renderer.
	GtkCellRenderer *column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbo), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cbo),
		column, "text", 0, nullptr);
#endif /* USE_GTK_DROP_DOWN */

	SET_CBO(cbo, 0);
	q->inhibit = prev_inhibit;
}

/**
 * Finish adding the ComboBoxes.
 */
void RpImageTypesTabPrivate::finishComboBoxes(void)
{
	if (!cboImageType_lastAdded) {
		// Nothing to do here.
		return;
	}

	/* TODO
	// Set the tab order for the credits label.
	Q_Q(RpImageTypesTab);
	q->setTabOrder(cboImageType_lastAdded, ui.lblCredits);
	cboImageType_lastAdded = nullptr;
	*/
}

/**
 * Write an ImageType configuration entry.
 * @param sysName System name.
 * @param imageTypeList Image type list, comma-separated.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpImageTypesTabPrivate::saveWriteEntry(const char *sysName, const char *imageTypeList)
{
	// NOTE: GKeyFile does *not* store comma-separated strings with
	// double-quotes, whereas QSettings does.
	// Config will simply ignore the double-quotes if it's present.
	assert(keyFile != nullptr);
	if (!keyFile) {
		return -ENOENT;
	}

	g_key_file_set_string(keyFile, "ImageTypes", sysName, imageTypeList);
	return 0;
}

/** TImageTypesConfig functions (public) **/

/**
 * Set a ComboBox's current index.
 * This will not trigger cboImageType_priorityValueChanged().
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
void RpImageTypesTabPrivate::cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;
	const SysData_t &sysData = v_sysData[sys];

	OurComboBox *const cbo = sysData.cboImageType[imageType];
	assert(cbo != nullptr);
	if (cbo) {
		const bool prev_inhibit = q->inhibit;
		q->inhibit = true;
		SET_CBO(cbo, prio < ImageTypesConfig::imageTypeCount() ? prio+1 : 0);
		q->inhibit = prev_inhibit;
	}
}

/** ImageTypesTab **/

static void
rp_image_types_tab_class_init(RpImageTypesTabClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = rp_image_types_tab_finalize;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	rp_config_cbid_quark = g_quark_from_string("rp-config.cbid");
}

static void
rp_image_types_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_image_types_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_image_types_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))rp_image_types_tab_load_defaults;
	iface->save = (__typeof__(iface->save))rp_image_types_tab_save;
}

static void
rp_image_types_tab_init(RpImageTypesTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Create the base widgets for the Image Types tab.
	GtkWidget *const lblImageTypes = gtk_label_new(C_("ImageTypesTab",
		"Select the image types you would like to use for each system as its thumbnail image.\n"
		"Internal images are contained within the ROM file.\n"
		"External images are downloaded from an external image database.\n\n"
		"1 = highest priority; 2 = second highest priority; No = ignore"));
	gtk_widget_set_name(lblImageTypes, "lblImageTypes");
	GTK_LABEL_XALIGN_LEFT(lblImageTypes);

	// Credits label
	// TODO: Runtime language retranslation?
	// tr: External image credits.
	tab->lblCredits = gtk_label_new(nullptr);
	gtk_widget_set_name(tab->lblCredits, "lblCredits");
	gtk_label_set_markup(GTK_LABEL(tab->lblCredits), C_("ImageTypesTab",
		"GameCube, Wii, Wii U, Nintendo DS, and Nintendo 3DS external images\n"
		"are provided by <a href=\"https://www.gametdb.com/\">GameTDB</a>.\n"
		"amiibo images are provided by <a href=\"https://amiibo.life/\">amiibo.life</a>,"
		" the Unofficial amiibo Database."));
	GTK_LABEL_XALIGN_LEFT(tab->lblCredits);

	// Create the GtkGrid/GtkTable.
#ifdef USE_GTK_GRID
	tab->table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(tab->table), 2);
	gtk_grid_set_column_spacing(GTK_GRID(tab->table), 2);
#else /* !USE_GTK_GRID */
	tab->table = gtk_table_new(ImageTypesConfig::sysCount()+1,
				   ImageTypesConfig::imageTypeCount()+1, false);
	gtk_table_set_row_spacings(GTK_TABLE(tab->table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(tab->table), 2);
#endif /* USE_GTK_GRID */
	gtk_widget_set_name(tab->table, "table");

	// Create the control grid.
	tab->d = new RpImageTypesTabPrivate(tab);
	tab->d->createGrid();

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), lblImageTypes);
	gtk_box_append(GTK_BOX(tab), tab->table);

	// TODO: Spacer and/or alignment?
	gtk_box_append(GTK_BOX(tab), tab->lblCredits);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(tab), lblImageTypes, false, false, 0);
	gtk_box_pack_start(GTK_BOX(tab), tab->table, false, false, 0);

	// TODO: Spacer and/or alignment?
	gtk_box_pack_end(GTK_BOX(tab), tab->lblCredits, false, false, 0);

	gtk_widget_show(lblImageTypes);
	gtk_widget_show(tab->table);
	gtk_widget_show(tab->lblCredits);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	rp_image_types_tab_reset(tab);
}

static void
rp_image_types_tab_finalize(GObject *object)
{
	RpImageTypesTab *const tab = RP_IMAGE_TYPES_TAB(object);

	// Delete the private class.
	delete tab->d;

	// Call the superclass finalize() function.
	G_OBJECT_CLASS(rp_image_types_tab_parent_class)->finalize(object);
}

GtkWidget*
rp_image_types_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_IMAGE_TYPES_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_image_types_tab_has_defaults(RpImageTypesTab *tab)
{
	g_return_val_if_fail(RP_IS_IMAGE_TYPES_TAB(tab), FALSE);
	return TRUE;
}

static void
rp_image_types_tab_reset(RpImageTypesTab *tab)
{
	g_return_if_fail(RP_IS_IMAGE_TYPES_TAB(tab));

	tab->inhibit = true;
	tab->d->reset();
	tab->changed = false;
	tab->inhibit = false;
}

static void
rp_image_types_tab_load_defaults(RpImageTypesTab *tab)
{
	g_return_if_fail(RP_IS_IMAGE_TYPES_TAB(tab));
	tab->inhibit = true;

	if (tab->d->loadDefaults()) {
		// Configuration has been changed.
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", nullptr);
	}

	tab->inhibit = false;
}

static void
rp_image_types_tab_save(RpImageTypesTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_IMAGE_TYPES_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.
	tab->d->keyFile = keyFile;
	tab->d->save();
	tab->d->keyFile = nullptr;

	// Configuration saved.
	tab->changed = false;
}

/** Signal handlers **/

#ifdef USE_GTK_DROP_DOWN
/**
 * Internal GObject "notify" signal handler for GtkDropDown's "selected" property.
 * @param cbo GtkDropDown sending the signal
 * @param pspec Property specification
 * @param widget RpLanguageComboBox
 */
static void
rp_image_types_tab_notify_selected_handler(GtkDropDown		*cbo,
					   GParamSpec		*pspec,
					   RpImageTypesTab	*tab)
{
	RP_UNUSED(pspec);

	assert(GTK_IS_DROP_DOWN(cbo));
	g_return_if_fail(GTK_IS_DROP_DOWN(cbo));

	if (tab->inhibit)
		return;

	const unsigned int cbid = GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cbo), rp_config_cbid_quark));
	RpImageTypesTabPrivate *const d = tab->d;

	const guint idx = gtk_drop_down_get_selected(cbo);
	const unsigned int prio = ((idx == 0 || idx == GTK_INVALID_LIST_POSITION) ? 0xFFU : idx-1);
	if (d->cboImageType_priorityValueChanged(cbid, prio)) {
		// Configuration has been changed.
		// Forward the "modified" signal.
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", nullptr);
	}
}
#else /* !USE_GTK_DROP_DOWN */
/**
 * Internal "modified" signal handler for GtkComboBox.
 * @param cbo GtkComboBox sending the signal
 * @param tab ImageTypesTab
 */
static void
rp_image_types_tab_modified_handler(GtkComboBox *cbo, RpImageTypesTab *tab)
{
	assert(GTK_IS_COMBO_BOX(cbo));
	g_return_if_fail(GTK_IS_COMBO_BOX(cbo));

	if (tab->inhibit)
		return;

	const unsigned int cbid = GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cbo), rp_config_cbid_quark));
	RpImageTypesTabPrivate *const d = tab->d;

	const int idx = gtk_combo_box_get_active(cbo);
	const unsigned int prio = (unsigned int)(idx <= 0 ? 0xFFU : idx-1);
	if (d->cboImageType_priorityValueChanged(cbid, prio)) {
		// Configuration has been changed.
		// Forward the "modified" signal.
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", nullptr);
	}
}
#endif /* !USE_GTK_DROP_DOWN */

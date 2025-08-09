/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget. (Private functions)             *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>

#include "OptionsMenuButton.hpp"

// librpbase
#include "librpbase/RomData.hpp"
namespace LibRpBase {
	class RomFields;
}

// C++ includes
#include <vector>

#if GTK_CHECK_VERSION(3, 0, 0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#  define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

// GTK+ property page class
struct _RpRomDataViewClass {
	superclass __parent__;
};

// Multi-language stuff
typedef std::pair<GtkLabel*, const LibRpBase::RomFields::Field*> Data_StringMulti_t;

#if GTK_CHECK_VERSION(4, 0, 0)
struct Data_ListDataMulti_t {
	GListStore *listStore;
	GtkColumnView *columnView;
	const LibRpBase::RomFields::Field *field;

	Data_ListDataMulti_t(
		GListStore *listStore,
		GtkColumnView *columnView,
		const LibRpBase::RomFields::Field *field)
		: listStore(listStore)
		, columnView(columnView)
		, field(field) { }
};
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
struct Data_ListDataMulti_t {
	GtkListStore *listStore;
	GtkTreeView *treeView;
	const LibRpBase::RomFields::Field *field;

	Data_ListDataMulti_t(
		GtkListStore *listStore,
		GtkTreeView *treeView,
		const LibRpBase::RomFields::Field *field)
		: listStore(listStore)
		, treeView(treeView)
		, field(field) { }
};
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

// C++ objects
struct _RpRomDataViewCxx {
	LibRpBase::RomDataPtr	romData;	// RomData

	struct tab {
		GtkWidget	*vbox;		// Either parent page or a GtkVBox/GtkBox.
		GtkWidget	*table;		// GtkTable (2.x); GtkGrid (3.x)
		GtkWidget	*lblCredits;

		tab() : vbox(nullptr), table(nullptr), lblCredits(nullptr) { }
	};
	std::vector<tab> tabs;

	// RFT_STRING_MULTI value labels
	std::vector<Data_StringMulti_t> vecStringMulti;

	// RFT_LISTDATA_MULTI value GtkListStores
	std::vector<Data_ListDataMulti_t> vecListDataMulti;

	// Default language code for multi-language.
	uint32_t def_lc;
};

// GTK+ property page instance
struct _RpRomDataView {
	super __parent__;

	_RpRomDataViewCxx	*cxx;		// C++ objects
	gchar			*uri;		// URI (GVfs)

	// Header row.
	GtkWidget	*hboxHeaderRow_outer;
	GtkWidget	*hboxHeaderRow;
	GtkWidget	*lblSysInfo;
	GtkWidget	*imgIcon;
	GtkWidget	*imgBanner;

	// Tab layout.
	GtkWidget	*tabWidget;
	// Tabs moved to: cxx->tabs

	/** ROM operations **/

	// "Options" button. (RpOptionsMenuButton)
	GtkWidget	*btnOptions;
	gchar		*prevExportDir;

	// MessageWidget for ROM operation notifications.
	GtkWidget	*messageWidget;

	/** Multi-language **/

	// Multi-language combo box.
	GtkWidget	*cboLanguage;

	/* Timeouts */
	guint		changed_idle;

	/** Other **/

	// Description label format type
	RpDescFormatType	desc_format_type;
	PangoAttrList		*pango_attr_desc_label;
	PangoAttrList		*pango_attr_warning;

	// Inhibit checkbox toggling for RFT_BITFIELD while updating.
	bool inhibit_checkbox_no_toggle;
	// Have we checked for achievements?
	bool hasCheckedAchievements;
};

G_BEGIN_DECLS

extern GQuark RFT_BITFIELD_value_quark;
extern GQuark RFT_LISTDATA_rows_visible_quark;
extern GQuark RFT_fieldIdx_quark;
extern GQuark RFT_STRING_warning_quark;

int	rp_rom_data_view_update_field		(RpRomDataView		*page,
						 int			 fieldIdx);

void	checkbox_no_toggle_signal_handler	(GtkCheckButton		*checkbutton,
						 RpRomDataView		*page);

void	btnOptions_triggered_signal_handler	(RpOptionsMenuButton	*menuButton,
						 gint		 	 id,
						 RpRomDataView		*page);

/**
 * Initialize a list data field.
 * @param page		[in] RomDataView object
 * @param field		[in] RomFields::Field
 * @return Display widget, or nullptr on error.
 */
GtkWidget*
rp_rom_data_view_init_listdata(RpRomDataView *page, const LibRpBase::RomFields::Field &field);

G_END_DECLS

#ifdef __cplusplus
/**
 * Update RFT_LISTDATA_MULTI fields.
 * Called from rp_rom_data_view_update_multi.
 * @param page		[in] RomDataView object.
 * @param user_lc	[in] User-specified language code.
 * @param set_lc	[in/out] Set of LCs
 */
void
rp_rom_data_view_update_multi_RFT_LISTDATA_MULTI(RpRomDataView *page, uint32_t user_lc, std::set<uint32_t> &set_lc);
#endif /* __cplusplus */

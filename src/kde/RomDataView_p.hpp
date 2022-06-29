/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.hpp: RomData viewer. (Private class)                        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_ROMDATAVIEW_P_HPP__
#define __ROMPROPERTIES_KDE_ROMDATAVIEW_P_HPP__

#include "config.kde.h"

// KDE4/KF5 includes
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <KAcceleratorManager>
#  include <KWidgetsAddons/kpagewidget.h>
#  define HAVE_KMESSAGEWIDGET 1
#  define HAVE_KMESSAGEWIDGET_SETICON 1
#  include <KWidgetsAddons/kmessagewidget.h>
#else /* !QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
#  include <kacceleratormanager.h>
#  include <kpagewidget.h>
#  include <kdeversion.h>
#  if (KDE_VERSION_MAJOR > 4) || (KDE_VERSION_MAJOR == 4 && KDE_VERSION_MINOR >= 7)
#    define HAVE_KMESSAGEWIDGET 1
#    include <kmessagewidget.h>
#    if (KDE_VERSION_MAJOR > 4) || (KDE_VERSION_MAJOR == 4 && KDE_VERSION_MINOR >= 11)
#      define HAVE_KMESSAGEWIDGET_SETICON 1
#    endif
#  endif
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

// librpbase
namespace LibRpBase {
	class RomData;
	class RomFields;
}

// C++ includes
#include <vector>

// Custom widgets
class LanguageComboBox;
class OptionsMenuButton;

// Data models
class ListDataModel;

// Uncomment to enable the automatic timeout for the ROM Operations KMessageWidget.
//#define AUTO_TIMEOUT_MESSAGEWIDGET 1

#include "ui_RomDataView.h"
class RomDataViewPrivate
{
	public:
		RomDataViewPrivate(RomDataView *q, LibRpBase::RomData *romData);
		~RomDataViewPrivate();

	private:
		RomDataView *const q_ptr;
		Q_DECLARE_PUBLIC(RomDataView)
	private:
		Q_DISABLE_COPY(RomDataViewPrivate)

	public:
		Ui::RomDataView ui;

		// RomData object.
		LibRpBase::RomData *romData;

		// "Options" button.
		OptionsMenuButton *btnOptions;
		QString prevExportDir;

		// Tab contents.
		struct tab {
			QVBoxLayout *vbox;
			QFormLayout *form;
			QLabel *lblCredits;

			tab() : vbox(nullptr), form(nullptr), lblCredits(nullptr) { }
		};
		std::vector<tab> tabs;

#ifdef HAVE_KMESSAGEWIDGET
		// KMessageWidget for ROM operation notifications.
		KMessageWidget *messageWidget;
#  ifdef AUTO_TIMEOUT_MESSAGEWIDGET
		QTimer *tmrMessageWidget;
#  endif /* AUTO_TIMEOUT_MESSAGEWIDGET */
#endif /* HAVE_KMESSAGEWIDGET */

		// Multi-language functionality.
		LanguageComboBox *cboLanguage;

		// RFT_STRING_MULTI value labels.
		typedef std::pair<QLabel*, const LibRpBase::RomFields::Field*> Data_StringMulti_t;
		std::vector<Data_StringMulti_t> vecStringMulti;

		// RFT_LISTDATA_MULTI value QTreeViews.
		// NOTE: The ListDataModel* is kept here too because the QTreeView
		// might have a QSortFilterProxyModel.
		typedef std::pair<QTreeView*, ListDataModel*> Data_ListDataMulti_t;
		std::vector<Data_ListDataMulti_t> vecListDataMulti;

		uint32_t def_lc;	// Default language code for multi-language.
		bool hasCheckedAchievements;

		/**
		 * Create the "Options" button in the parent window.
		 */
		void createOptionsButton(void);

		/**
		 * Initialize the header row widgets.
		 * The widgets must have already been created by ui.setupUi().
		 */
		void initHeaderRow(void);

		/**
		 * Clear a QLayout.
		 * @param layout QLayout.
		 */
		static void clearLayout(QLayout *layout);

		/**
		 * Initialize a string field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @param str		[in,opt] String data (If nullptr, field data is used)
		 * @return QLabel*, or nullptr on error.
		 */
		QLabel *initString(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx,
			const QString *str = nullptr);

		/**
		 * Initialize a string field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @param str		[in] String data
		 */
		inline QLabel *initString(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx,
			const QString &str)
		{
			return initString(lblDesc, field, fieldIdx, &str);
		}

		/**
		 * Initialize a bitfield.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initBitfield(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a list data field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initListData(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Adjust an RFT_LISTDATA field if it's the last field in a tab.
		 * @param tabIdx Tab index.
		 */
		void adjustListData(int tabIdx);

		/**
		 * Initialize a Date/Time field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initDateTime(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize an Age Ratings field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initAgeRatings(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a Dimensions field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initDimensions(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a multi-language string field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initStringMulti(QLabel *lblDesc,
			const LibRpBase::RomFields::Field &field, int fieldIdx);

		/**
		 * Update all multi-language fields.
		 * @param user_lc User-specified language code.
		 */
		void updateMulti(uint32_t user_lc);

		/**
		 * Update a field's value.
		 * This is called after running a ROM operation.
		 * @param fieldIdx Field index.
		 * @return 0 on success; non-zero on error.
		 */
		int updateField(int fieldIdx);

		/**
		 * Initialize the display widgets.
		 * If the widgets already exist, they will
		 * be deleted and recreated.
		 */
		void initDisplayWidgets(void);

	public:
		/**
		 * ROM operation: Standard Operations
		 * Dispatched by RomDataView::btnOptions_triggered().
		 * @param id Standard operation ID
		 */
		void doRomOp_stdop(int id);
};


#endif /* __ROMPROPERTIES_KDE_ROMDATAVIEW_P_HPP__ */

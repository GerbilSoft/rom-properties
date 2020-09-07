/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.cpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RomDataView.hpp"
#include "RpQImageBackend.hpp"
#include "MessageSound.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;
using LibRpTexture::rp_image;

// libi18n
#include "libi18n/i18n.h"

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

// Qt includes.
#include <QtGui/QClipboard>

// Custom Qt widgets.
#include "DragImageTreeWidget.hpp"

// KDE4/KF5 includes.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <KAcceleratorManager>
#  include <KWidgetsAddons/kpagewidget.h>
#  define HAVE_KMESSAGEWIDGET 1
#  include <KWidgetsAddons/kmessagewidget.h>
#else /* !QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
#  include <kacceleratormanager.h>
#  include <kpagewidget.h>
#  include <kdeversion.h>
#  if (KDE_VERSION_MAJOR > 4) || (KDE_VERSION_MAJOR == 4 && KDE_VERSION_MINOR >= 7)
#    define HAVE_KMESSAGEWIDGET 1
#    include <kmessagewidget.h>
#  endif
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#include "ui_RomDataView.h"
class RomDataViewPrivate
{
	public:
		RomDataViewPrivate(RomDataView *q, RomData *romData);
		~RomDataViewPrivate();

	private:
		RomDataView *const q_ptr;
		Q_DECLARE_PUBLIC(RomDataView)
	private:
		Q_DISABLE_COPY(RomDataViewPrivate)

	public:
		Ui::RomDataView ui;

		// "Options" button.
		QPushButton *btnOptions;
		QMenu *menuOptions;
		int romOps_firstActionIndex;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
		QSignalMapper *mapperOptionsMenu;
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
		QString prevExportDir;

		enum StandardOptionID {
			OPTION_EXPORT_TEXT = -1,
			OPTION_EXPORT_JSON = -2,
			OPTION_COPY_TEXT = -3,
			OPTION_COPY_JSON = -4,
		};

		// Tab contents.
		struct tab {
			QVBoxLayout *vbox;
			QFormLayout *form;
			QLabel *lblCredits;

			tab() : vbox(nullptr), form(nullptr), lblCredits(nullptr) { }
		};
		vector<tab> tabs;

		// Mapping of field index to QObject*.
		// For updateField().
		unordered_map<int, QObject*> map_fieldIdx;

#ifdef HAVE_KMESSAGEWIDGET
		// KMessageWidget for ROM operation notifications.
		KMessageWidget *messageWidget;
		QTimer *tmrMessageWidget;
#endif /* HAVE_KMESSAGEWIDGET */

		// Multi-language functionality.
		uint32_t def_lc;
		QComboBox *cboLanguage;

		/**
		 * Get the selected language code.
		 * @return Selected language code, or 0 for none (default).
		 */
		inline uint32_t sel_lc(void) const;

		// RFT_STRING_MULTI value labels.
		typedef std::pair<QLabel*, const RomFields::Field*> Data_StringMulti_t;
		vector<Data_StringMulti_t> vecStringMulti;

		// RFT_LISTDATA_MULTI value QTreeWidgets.
		typedef std::pair<QTreeWidget*, const RomFields::Field*> Data_ListDataMulti_t;
		vector<Data_ListDataMulti_t> vecListDataMulti;

		// RomData object.
		RomData *romData;

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
			const RomFields::Field &field, int fieldIdx,
			const QString *str = nullptr);

		/**
		 * Initialize a string field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @param str		[in] String data
		 */
		inline QLabel *initString(QLabel *lblDesc,
			const RomFields::Field &field, int fieldIdx,
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
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a list data field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initListData(QLabel *lblDesc,
			const RomFields::Field &field, int fieldIdx);

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
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize an Age Ratings field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initAgeRatings(QLabel *lblDesc,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a Dimensions field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initDimensions(QLabel *lblDesc,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a multi-language string field.
		 * @param lblDesc	[in] Description label
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 */
		void initStringMulti(QLabel *lblDesc,
			const RomFields::Field &field, int fieldIdx);

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
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
	, btnOptions(nullptr)
	, menuOptions(nullptr)
	, romOps_firstActionIndex(-1)
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	, mapperOptionsMenu(nullptr)
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
#ifdef HAVE_KMESSAGEWIDGET
	, messageWidget(nullptr)
	, tmrMessageWidget(nullptr)
#endif /* HAVE_KMESSAGEWIDGET */
	, def_lc(0)
	, cboLanguage(nullptr)
	, romData(romData->ref())
{
	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
}

RomDataViewPrivate::~RomDataViewPrivate()
{
	ui.lblIcon->clearRp();
	ui.lblBanner->clearRp();
	UNREF(romData);
}

/**
 * Get the selected language code.
 * @return Selected language code, or 0 for none (default).
 */
inline uint32_t RomDataViewPrivate::sel_lc(void) const
{
	if (!cboLanguage) {
		// No language dropdown...
		return 0;
	}

	return cboLanguage->itemData(cboLanguage->currentIndex()).value<uint32_t>();
}

/**
 * Find direct child widgets only.
 * @param T Type.
 */
template<typename T>
static inline T findDirectChild(QObject *obj, const QString &aName = QString())
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	return obj->findChild<T>(aName, Qt::FindDirectChildrenOnly);
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	foreach(QObject *child, obj->children()) {
		T qchild = qobject_cast<T>(child);
		if (qchild != nullptr) {
			if (aName.isEmpty() || qchild->objectName() == aName) {
				return qchild;
			}
		}
	}
	return nullptr;
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
}

/**
 * Create the "Options" button in the parent window.
 */
void RomDataViewPrivate::createOptionsButton(void)
{
	assert(btnOptions == nullptr);
	if (btnOptions)
		return;

	Q_Q(RomDataView);

	// Parent should be a KPropertiesDialog.
	QObject *const parent = q->parent();
	assert(parent != nullptr);
	if (!parent)
		return;

	// Parent should contain a KPageWidget.
	// NOTE: Kubuntu 16.04 (Dolphin 15.12.3, KWidgetsAddons 5.18.0) has
	// the QDialogButtonBox in the KPropertiesDialog, not the KPageWidget.
	// NOTE 2: Newer frameworks with QDialogButtonBox in the KPageWidget
	// also give it the object name "buttonbox". We'll leave out the name
	// for compatibility purposes.
	KPageWidget *const pageWidget = findDirectChild<KPageWidget*>(parent);

	// Check for the QDialogButtonBox in the KPageWidget first.
	QDialogButtonBox *btnBox = findDirectChild<QDialogButtonBox*>(pageWidget);
	if (!btnBox) {
		// Check in the KPropertiesDialog.
		btnBox = findDirectChild<QDialogButtonBox*>(parent);
	}
	assert(btnBox != nullptr);
	if (!btnBox)
		return;

	// Create the "Options" button.
	// tr: "Options" button.
	const QString s_options = U82Q(C_("RomDataView", "Op&tions"));
	btnOptions = btnBox->addButton(s_options, QDialogButtonBox::ActionRole);
	btnOptions->hide();

	// Add a spacer to the QDialogButtonBox.
	// This will ensure that the "Options" button is left-aligned.
	QBoxLayout *const boxLayout = findDirectChild<QBoxLayout*>(btnBox);
	if (boxLayout) {
		// Find the index of the "Options" button.
		const int count = boxLayout->count();
		int idx = -1;
		for (int i = 0; i < count; i++) {
			if (boxLayout->itemAt(i)->widget() == btnOptions) {
				idx = i;
				break;
			}
		}

		if (idx >= 0) {
			boxLayout->insertStretch(idx+1, 1);
		}
	}

	// Create the menu and (for Qt4) signal mapper.
	// TODO: Signals.
	menuOptions = new QMenu(s_options, q);
	btnOptions->setMenu(menuOptions);

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	mapperOptionsMenu = new QSignalMapper(q);
	QObject::connect(mapperOptionsMenu, SIGNAL(mapped(int)),
		q, SLOT(menuOptions_action_triggered(int)));
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */

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
		QAction *const action = menuOptions->addAction(
			U82Q(dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p->desc)));
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QObject::connect(action, &QAction::triggered,
			[q, p] { q->menuOptions_action_triggered(p->id); });
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
		QObject::connect(action, SIGNAL(triggered()),
			mapperOptionsMenu, SLOT(map()));
		mapperOptionsMenu->setMapping(action, p->id);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
	}

	/** ROM operations. **/
	const vector<RomData::RomOps> ops = romData->romOps();
	if (!ops.empty()) {
		menuOptions->addSeparator();
		romOps_firstActionIndex = menuOptions->children().count();

		int i = 0;
		const auto ops_end = ops.cend();
		for (auto iter = ops.cbegin(); iter != ops_end; ++iter, i++) {
			QAction *const action = menuOptions->addAction(U82Q(iter->desc));
			action->setEnabled(!!(iter->flags & RomData::RomOps::ROF_ENABLED));
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QObject::connect(action, &QAction::triggered,
				[q, i] { q->menuOptions_action_triggered(i); });
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
			QObject::connect(action, SIGNAL(triggered()),
				mapperOptionsMenu, SLOT(map()));
			mapperOptionsMenu->setMapping(action, i);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
		}
	}
}

/**
 * Initialize the header row widgets.
 * The widgets must have already been created by ui.setupUi().
 */
void RomDataViewPrivate::initHeaderRow(void)
{
	if (!romData) {
		// No ROM data.
		ui.lblSysInfo->hide();
		ui.lblBanner->hide();
		ui.lblIcon->hide();
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

	QString sysInfo = U82Q(rp_sprintf_p(
		// tr: %1$s == system name, %2$s == file type
		C_("RomDataView", "%1$s\n%2$s"), systemName, fileType));
	ui.lblSysInfo->setText(sysInfo);
	ui.lblSysInfo->show();

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		bool ok = ui.lblBanner->setRpImage(romData->image(RomData::IMG_INT_BANNER));
		ui.lblBanner->setVisible(ok);
	} else {
		// No banner.
		ui.lblBanner->hide();
	}

	// Icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image *const icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Is this an animated icon?
			bool ok = ui.lblIcon->setIconAnimData(romData->iconAnimData());
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = ui.lblIcon->setRpImage(icon);
			}
			ui.lblIcon->setVisible(ok);
		} else {
			// No icon.
			ui.lblIcon->hide();
		}
	} else {
		// No icon.
		ui.lblIcon->hide();
	}
}

/**
 * Clear a QLayout.
 * @param layout QLayout.
 */
void RomDataViewPrivate::clearLayout(QLayout *layout)
{
	// References:
	// - http://doc.qt.io/qt-4.8/qlayout.html#takeAt
	// - http://stackoverflow.com/questions/4857188/clearing-a-layout-in-qt

	if (!layout)
		return;
	
	while (!layout->isEmpty()) {
		QLayoutItem *item = layout->takeAt(0);
		if (item->layout()) {
			// This also handles QSpacerItem.
			// NOTE: If this is a layout, item->layout() returns 'this'.
			// We only want to delete the sub-item if it's a widget.
			clearLayout(item->layout());
		} else if (item->widget()) {
			// Delete the widget.
			delete item->widget();
		}
		delete item;
	}
}

/**
 * Initialize a string field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @param str		[in,opt] String data (If nullptr, field data is used)
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initString(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx,
	const QString *str)
{
	// String type.
	Q_Q(RomDataView);
	QLabel *lblString = new QLabel(q);
	if (field.desc.flags & RomFields::STRF_CREDITS) {
		// Credits text. Enable formatting and center text.
		lblString->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		lblString->setTextFormat(Qt::RichText);
		lblString->setOpenExternalLinks(true);
		lblString->setTextInteractionFlags(
			Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);

		// Replace newlines with "<br/>".
		QString text;
		if (str) {
			text = *str;
		} else if (field.data.str) {
			text = U82Q(*(field.data.str));
		}
		text.replace(QChar(L'\n'), QLatin1String("<br/>"));
		lblString->setText(text);
	} else {
		// tr: Standard text with no formatting.
		lblString->setTextInteractionFlags(
			Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
		lblString->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblString->setTextFormat(Qt::PlainText);
		if (str) {
			lblString->setText(*str);
		} else if (field.data.str) {
			lblString->setText(U82Q(*(field.data.str)));
		}
	}

	// Enable strong focus so we can tab into the label.
	lblString->setFocusPolicy(Qt::StrongFocus);

	// Check for any formatting options. (RFT_STRING only)
	if (field.type == RomFields::RFT_STRING) {
		// Monospace font?
		if (field.desc.flags & RomFields::STRF_MONOSPACE) {
			QFont font(QLatin1String("Monospace"));
			font.setStyleHint(QFont::TypeWriter);
			lblString->setFont(font);
			lblString->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		}

		// "Warning" font?
		if (field.desc.flags & RomFields::STRF_WARNING) {
			// Only expecting a maximum of one "Warning" per ROM,
			// so we're initializing this here.
			const QString css = QLatin1String("color: #F00; font-weight: bold;");
			lblDesc->setStyleSheet(css);
			lblString->setStyleSheet(css);
		}
	}

	// Credits?
	auto &tab = tabs[field.tabIdx];
	if (field.type == RomFields::RFT_STRING &&
	    (field.desc.flags & RomFields::STRF_CREDITS))
	{
		// Credits row goes at the end.
		// There should be a maximum of one STRF_CREDITS per tab.
		assert(tab.lblCredits == nullptr);
		if (!tab.lblCredits) {
			// Save this as the credits label.
			tab.lblCredits = lblString;
			// Add the credits label to the end of the QVBoxLayout.
			tab.vbox->addWidget(lblString, 0, Qt::AlignHCenter | Qt::AlignBottom);

			// Set the bottom margin to match the QFormLayout.
			// TODO: Use a QHBoxLayout whose margins match the QFormLayout?
			// TODO: Verify this.
			QMargins margins = tab.form->contentsMargins();
			tab.vbox->setContentsMargins(margins);
		} else {
			// Duplicate credits label.
			delete lblString;
			lblString = nullptr;
		}

		// No description field.
		delete lblDesc;
	} else {
		// Standard string row.
		tab.form->addRow(lblDesc, lblString);
	}

	map_fieldIdx.insert(std::make_pair(fieldIdx, lblString));
	return lblString;
}

/**
 * Initialize a bitfield.
 * @param lblDesc Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initBitfield(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx)
{
	// Bitfield type. Create a grid of checkboxes.
	Q_Q(RomDataView);
	const auto &bitfieldDesc = field.desc.bitfield;
	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32)
		count = 32;

	QGridLayout *gridLayout = new QGridLayout();
	int row = 0, col = 0;
	uint32_t bitfield = field.data.bitfield;
	const auto names_cend = bitfieldDesc.names->cend();
	for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter, bitfield >>= 1) {
		const string &name = *iter;
		if (name.empty())
			continue;

		QCheckBox *const checkBox = new QCheckBox(q);

		// Disable automatic mnemonics.
		KAcceleratorManager::setNoAccel(checkBox);

		// Set the name and value.
		const bool value = (bitfield & 1);
		checkBox->setText(U82Q(name));
		checkBox->setChecked(value);

		// Save the bitfield checkbox's value in the QObject.
		checkBox->setProperty("RFT_BITFIELD_value", value);

		// Disable user modifications.
		// TODO: Prevent the initial mousebutton down from working;
		// otherwise, it shows a partial check mark.
		QObject::connect(checkBox, SIGNAL(clicked(bool)),
				 q, SLOT(bitfield_clicked_slot(bool)));

		gridLayout->addWidget(checkBox, row, col, 1, 1);
		col++;
		if (col == bitfieldDesc.elemsPerRow) {
			row++;
			col = 0;
		}
	}

	tabs[field.tabIdx].form->addRow(lblDesc, gridLayout);
	map_fieldIdx.insert(std::make_pair(fieldIdx, gridLayout));
}

/**
 * Initialize a list data field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initListData(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx)
{
	// ListData type. Create a QTreeWidget.
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
			delete lblDesc;
			return;
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
		delete lblDesc;
		return;
	}

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		delete lblDesc;
		return;
	}

	if (hasIcons) {
		assert(field.data.list_data.mxd.icons != nullptr);
		if (!field.data.list_data.mxd.icons) {
			// No icons vector...
			delete lblDesc;
			return;
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

	Q_Q(RomDataView);
	QTreeWidget *treeWidget;
	Qt::ItemFlags itemFlags;
	if (hasIcons) {
		treeWidget = new DragImageTreeWidget(q);
		treeWidget->setDragEnabled(true);
		treeWidget->setDefaultDropAction(Qt::CopyAction);
		treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
		itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
		// TODO: Get multi-image drag & drop working.
		//treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
		treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	} else {
		treeWidget = new QTreeWidget(q);
		itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	}
	treeWidget->setRootIsDecorated(false);
	treeWidget->setAlternatingRowColors(true);

	// DISABLED uniform row heights.
	// Some Xbox 360 achievements take up two lines,
	// while others might take up three or more.
	treeWidget->setUniformRowHeights(false);

	// Format table.
	// All values are known to fit in uint8_t.
	// NOTE: Need to include AlignVCenter.
	static const uint8_t align_tbl[4] = {
		// Order: TXA_D, TXA_L, TXA_C, TXA_R
		Qt::AlignLeft | Qt::AlignVCenter,
		Qt::AlignLeft | Qt::AlignVCenter,
		Qt::AlignCenter,
		Qt::AlignRight | Qt::AlignVCenter,
	};

	// Set up the column names.
	treeWidget->setColumnCount(colCount);
	if (listDataDesc.names) {
		QStringList columnNames;
		columnNames.reserve(colCount);
		QTreeWidgetItem *const header = treeWidget->headerItem();
		uint32_t align = listDataDesc.alignment.headers;
		auto iter = listDataDesc.names->cbegin();
		for (int col = 0; col < colCount; col++, ++iter, align >>= 2) {
			header->setTextAlignment(col, align_tbl[align & 3]);

			const string &name = *iter;
			if (!name.empty()) {
				columnNames.append(U82Q(name));
			} else {
				// Don't show this column.
				columnNames.append(QString());
				treeWidget->setColumnHidden(col, true);
			}
		}
		treeWidget->setHeaderLabels(columnNames);
	} else {
		// Hide the header.
		treeWidget->header()->hide();
	}

	if (hasIcons) {
		// TODO: Ideal icon size?
		// Using 32x32 for now.
		treeWidget->setIconSize(QSize(32, 32));
	}

	// Add the row data.
	// NOTE: For RFT_STRING_MULTI, we're only adding placeholder rows.
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

		QTreeWidgetItem *const treeWidgetItem = new QTreeWidgetItem(treeWidget);
		if (hasCheckboxes) {
			// The checkbox will only show up if setCheckState()
			// is called at least once, regardless of value.
			treeWidgetItem->setCheckState(0, (checkboxes & 1) ? Qt::Checked : Qt::Unchecked);
			checkboxes >>= 1;
		} else if (hasIcons) {
			const rp_image *const icon = field.data.list_data.mxd.icons->at(row);
			if (icon) {
				treeWidgetItem->setIcon(0, QIcon(
					QPixmap::fromImage(rpToQImage(icon))));
				treeWidgetItem->setData(0, DragImageTreeWidget::RpImageRole,
					QVariant::fromValue((void*)icon));
			}
		}

		// Set item flags.
		treeWidgetItem->setFlags(itemFlags);

		int col = 0;
		uint32_t align = listDataDesc.alignment.data;
		const auto data_row_cend = data_row.cend();
		for (auto iter = data_row.cbegin(); iter != data_row_cend; ++iter) {
			if (!isMulti) {
				treeWidgetItem->setData(col, Qt::DisplayRole, U82Q(*iter));
			}
			treeWidgetItem->setTextAlignment(col, align_tbl[align & 3]);
			col++;
			align >>= 2;
		}
	}

	if (!isMulti) {
		// Resize the columns to fit the contents.
		for (int i = 0; i < colCount; i++) {
			treeWidget->resizeColumnToContents(i);
		}
		treeWidget->resizeColumnToContents(colCount);
	}

	if (listDataDesc.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
		// Separate rows.
		tabs[field.tabIdx].form->addRow(lblDesc);
		tabs[field.tabIdx].form->addRow(treeWidget);
	} else {
		// Single row.
		tabs[field.tabIdx].form->addRow(lblDesc, treeWidget);
	}
	map_fieldIdx.insert(std::make_pair(fieldIdx, treeWidget));

	// Row height is recalculated when the window is first visible
	// and/or the system theme is changed.
	// TODO: Set an actual default number of rows, or let Qt handle it?
	// (Windows uses 5.)
	treeWidget->setProperty("RFT_LISTDATA_rows_visible", listDataDesc.rows_visible);

	// Install the event filter.
	treeWidget->installEventFilter(q);

	if (isMulti) {
		vecListDataMulti.emplace_back(std::make_pair(treeWidget, &field));
	}
}

/**
 * Adjust an RFT_LISTDATA field if it's the last field in a tab.
 * @param tabIdx Tab index.
 */
void RomDataViewPrivate::adjustListData(int tabIdx)
{
	auto &tab = tabs[tabIdx];
	assert(tab.form != nullptr);
	if (!tab.form)
		return;
	int row = tab.form->rowCount();
	if (row <= 0)
		return;
	row--;

	QLayoutItem *const liLabel = tab.form->itemAt(row, QFormLayout::LabelRole);
	QLayoutItem *const liField = tab.form->itemAt(row, QFormLayout::FieldRole);
	if (liLabel || !liField) {
		// Either we have a label, or we don't have a field.
		// This is not RFT_LISTDATA_SEPARATE_ROW.
		return;
	}

	QTreeWidget *const treeWidget = qobject_cast<QTreeWidget*>(liField->widget());
	if (treeWidget) {
		// Move the treeWidget to the QVBoxLayout.
		int newRow = tab.vbox->count();
		if (tab.lblCredits) {
			newRow--;
		}
		assert(newRow >= 0);
		tab.form->removeItem(liField);
		tab.vbox->insertWidget(newRow, treeWidget, 999, Qt::Alignment());
		delete liField;

		// Unset this property to prevent the event filter from
		// setting a fixed height.
		treeWidget->setProperty("RFT_LISTDATA_rows_visible", 0);
	}
}

/**
 * Initialize a Date/Time field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initDateTime(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx)
{
	// Date/Time.
	if (field.data.date_time == -1) {
		// tr: Invalid date/time.
		initString(lblDesc, field, fieldIdx, U82Q(C_("RomDataView", "Unknown")));
		return;
	}

	QDateTime dateTime;
	dateTime.setTimeSpec(
		(field.desc.flags & RomFields::RFT_DATETIME_IS_UTC)
			? Qt::UTC : Qt::LocalTime);
	dateTime.setMSecsSinceEpoch(field.data.date_time * 1000);

	QString str;
	switch (field.desc.flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK) {
		case RomFields::RFT_DATETIME_HAS_DATE:
			// Date only.
			str = dateTime.date().toString(Qt::DefaultLocaleShortDate);
			break;

		case RomFields::RFT_DATETIME_HAS_TIME:
		case RomFields::RFT_DATETIME_HAS_TIME |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Time only.
			str = dateTime.time().toString(Qt::DefaultLocaleShortDate);
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_HAS_TIME:
			// Date and time.
			str = dateTime.toString(Qt::DefaultLocaleShortDate);
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Date only. (No year)
			// TODO: Localize this.
			str = dateTime.date().toString(QLatin1String("MMM d"));
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
		     RomFields::RFT_DATETIME_HAS_TIME |
		     RomFields::RFT_DATETIME_NO_YEAR:
			// Date and time. (No year)
			// TODO: Localize this.
			str = dateTime.date().toString(QLatin1String("MMM d")) + QChar(L' ') +
			      dateTime.time().toString();
			break;

		default:
			// Invalid combination.
			assert(!"Invalid Date/Time formatting.");
			break;
	}

	if (!str.isEmpty()) {
		initString(lblDesc, field, fieldIdx, str);
	} else {
		// Invalid date/time.
		delete lblDesc;
	}
}

/**
 * Initialize an Age Ratings field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initAgeRatings(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx)
{
	// Age ratings.
	const RomFields::age_ratings_t *age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// tr: No age ratings data.
		initString(lblDesc, field, fieldIdx, U82Q(C_("RomDataView", "ERROR")));
		return;
	}

	// Convert the age ratings field to a string.
	const QString str = U82Q(RomFields::ageRatingsDecode(age_ratings));
	initString(lblDesc, field, fieldIdx, str);
}

/**
 * Initialize a Dimensions field.
 * @param lblDesc Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initDimensions(QLabel *lblDesc,
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

	initString(lblDesc, field, fieldIdx, QString::fromLatin1(buf));
}

/**
 * Initialize a multi-language string field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 */
void RomDataViewPrivate::initStringMulti(QLabel *lblDesc,
	const RomFields::Field &field, int fieldIdx)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	QString qs_empty;
	QLabel *const lblStringMulti = initString(lblDesc, field, fieldIdx, &qs_empty);
	if (lblStringMulti) {
		vecStringMulti.emplace_back(std::make_pair(lblStringMulti, &field));
	}
}

/**
 * Update all multi-language fields.
 * @param user_lc User-specified language code.
 */
void RomDataViewPrivate::updateMulti(uint32_t user_lc)
{
	// Set of supported language codes.
	// NOTE: Using std::set instead of QSet for sorting.
	set<uint32_t> set_lc;

	// RFT_STRING_MULTI
	const auto vecStringMulti_cend = vecStringMulti.cend();
	for (auto iter = vecStringMulti.cbegin(); iter != vecStringMulti_cend; ++iter) {
		QLabel *const lblString = iter->first;
		const RomFields::Field *const pField = iter->second;
		const auto *const pStr_multi = pField->data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (!pStr_multi || pStr_multi->empty()) {
			// Invalid multi-string...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			const auto pStr_multi_cend = pStr_multi->cend();
			for (auto iter_sm = pStr_multi->cbegin();
			     iter_sm != pStr_multi_cend; ++iter_sm)
			{
				set_lc.insert(iter_sm->first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, def_lc, user_lc);
		assert(pStr != nullptr);
		if (pStr) {
			lblString->setText(U82Q(*pStr));
		} else {
			lblString->setText(QString());
		}
	}

	// RFT_LISTDATA_MULTI
	const auto vecListDataMulti_cend = vecListDataMulti.cend();
	for (auto iter = vecListDataMulti.cbegin(); iter != vecListDataMulti_cend; ++iter) {
		QTreeWidget *const treeWidget = iter->first;
		const RomFields::Field *const pField = iter->second;
		const auto *const pListData_multi = pField->data.list_data.data.multi;
		assert(pListData_multi != nullptr);
		assert(!pListData_multi->empty());
		if (!pListData_multi || pListData_multi->empty()) {
			// Invalid RFT_LISTDATA_MULTI...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			const auto pListData_multi_cend = pListData_multi->cend();
			for (auto iter_sm = pListData_multi->cbegin();
			     iter_sm != pListData_multi_cend; ++iter_sm)
			{
				set_lc.insert(iter_sm->first);
			}
		}

		// Get the ListData_t.
		const auto *const pListData = RomFields::getFromListDataMulti(pListData_multi, def_lc, user_lc);
		assert(pListData != nullptr);
		if (pListData != nullptr) {
			// Update the list.
			const int rowCount = treeWidget->topLevelItemCount();
			auto iter_listData = pListData->cbegin();
			const auto pListData_cend = pListData->cend();
			for (int row = 0; row < rowCount && iter_listData != pListData_cend; row++, ++iter_listData) {
				QTreeWidgetItem *const treeWidgetItem = treeWidget->topLevelItem(row);

				int col = 0;
				const auto iter_listData_cend = iter_listData->cend();
				for (auto iter_row = iter_listData->cbegin();
				     iter_row != iter_listData_cend; ++iter_row, col++)
				{
					treeWidgetItem->setData(col, Qt::DisplayRole, U82Q(*iter_row));
				}
			}

			// Resize the columns to fit the contents.
			// NOTE: Only done on first load.
			if (!cboLanguage) {
				const int colCount = treeWidget->columnCount();
				for (int i = 0; i < colCount; i++) {
					treeWidget->resizeColumnToContents(i);
				}
				treeWidget->resizeColumnToContents(colCount);
			}
		}
	}

	if (!cboLanguage && set_lc.size() > 1) {
		// Create the language combobox.
		Q_Q(RomDataView);
		cboLanguage = new QComboBox(q);
		cboLanguage->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
		ui.hboxHeaderRow->addWidget(cboLanguage);

		// Sprite sheets. (32x32, 24x24, 16x16)
		const QPixmap spriteSheets[3] = {
			QPixmap(QLatin1String(":/flags/flags-32x32.png")),
			QPixmap(QLatin1String(":/flags/flags-24x24.png")),
			QPixmap(QLatin1String(":/flags/flags-16x16.png")),
		};

		int sel_idx = -1;
		const auto set_lc_cend = set_lc.cend();
		for (auto iter = set_lc.cbegin(); iter != set_lc_cend; ++iter) {
			const uint32_t lc = *iter;
			const char *const name = SystemRegion::getLocalizedLanguageName(lc);
			if (name) {
				cboLanguage->addItem(U82Q(name), lc);
			} else {
				QString s_lc;
				s_lc.reserve(4);
				for (uint32_t tmp_lc = lc; tmp_lc != 0; tmp_lc <<= 8) {
					ushort chr = (ushort)(tmp_lc >> 24);
					if (chr != 0) {
						s_lc += QChar(chr);
					}
				}
				cboLanguage->addItem(s_lc, lc);
			}
			int cur_idx = cboLanguage->count()-1;

			// Flag icon.
			int col, row;
			if (!SystemRegion::getFlagPosition(lc, &col, &row)) {
				// Found a matching icon.
				QIcon flag_icon;
				flag_icon.addPixmap(spriteSheets[0].copy(col*32, row*32, 32, 32));
				flag_icon.addPixmap(spriteSheets[1].copy(col*24, row*24, 24, 24));
				flag_icon.addPixmap(spriteSheets[2].copy(col*16, row*16, 16, 16));
				cboLanguage->setItemIcon(cur_idx, flag_icon);
			}

			// Save the default index:
			// - ROM-default language code.
			// - English if it's not available.
			if (lc == def_lc) {
				// Select this item.
				sel_idx = cur_idx;
			} else if (lc == 'en') {
				// English. Select this item if def_lc hasn't been found yet.
				if (sel_idx < 0) {
					sel_idx = cur_idx;
				}
			}
		}

		// Set the current index.
		cboLanguage->setCurrentIndex(sel_idx);

		// Connect the signal after everything's been initialized.
		QObject::connect(cboLanguage, SIGNAL(currentIndexChanged(int)),
		                 q, SLOT(cboLanguage_currentIndexChanged_slot(int)));
	}
}

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param fieldIdx Field index.
 * @return 0 on success; non-zero on error.
 */
int RomDataViewPrivate::updateField(int fieldIdx)
{
	const RomFields *const pFields = romData->fields();
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

	// Get the QObject*.
	auto iter = map_fieldIdx.find(fieldIdx);
	if (iter == map_fieldIdx.end()) {
		// Not found.
		return 4;
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
			// QObject is a QLabel.
			QLabel *const label = qobject_cast<QLabel*>(iter->second);
			assert(label != nullptr);
			if (!label) {
				ret = 7;
				break;
			}

			label->setText(field->data.str
				? U82Q(*(field->data.str))
				: QString());
			ret = 0;
			break;
		}

		case RomFields::RFT_BITFIELD: {
			// QObject is a QGridLayout with QCheckBox widgets.
			QGridLayout *const layout = qobject_cast<QGridLayout*>(iter->second);
			assert(layout != nullptr);
			if (!layout) {
				ret = 8;
				break;
			}

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;
			int count = (int)bitfieldDesc.names->size();
			assert(count <= 32);
			if (count > 32)
				count = 32;

			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			int layoutIdx = 0;
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter, bitfield >>= 1) {
				const string &name = *iter;
				if (name.empty())
					continue;

				// Get the widget.
				QLayoutItem *const subLayoutItem = layout->itemAt(layoutIdx);
				assert(subLayoutItem != nullptr);
				if (!subLayoutItem)
					break;

				QCheckBox *const checkBox = qobject_cast<QCheckBox*>(subLayoutItem->widget());
				assert(checkBox != nullptr);
				if (!checkBox)
					break;

				const bool value = (bitfield & 1);
				checkBox->setChecked(value);
				checkBox->setProperty("RFT_BITFIELD_value", value);

				// Next layout item.
				layoutIdx++;
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * Initialize the display widgets.
 * If the widgets already exist, they will
 * be deleted and recreated.
 */
void RomDataViewPrivate::initDisplayWidgets(void)
{
	// Clear the tabs.
	std::for_each(tabs.begin(), tabs.end(),
		[this](RomDataViewPrivate::tab &tab) {
			// Delete the credits label if it's present.
			delete tab.lblCredits;
			// Delete the QFormLayout if it's present.
			if (tab.form) {
				clearLayout(tab.form);
				delete tab.form;
			}
			// Delete the QVBoxLayout.
			if (tab.vbox != ui.vboxLayout) {
				delete tab.vbox;
			}
		}
	);
	tabs.clear();
	map_fieldIdx.clear();
	ui.tabWidget->clear();
	ui.tabWidget->hide();

	// Initialize the header row.
	initHeaderRow();

	if (!romData) {
		// No ROM data to display.
		return;
	}

	// Get the fields.
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return;
	}

	// Create the QTabWidget.
	Q_Q(RomDataView);
	const int tabCount = pFields->tabCount();
	if (tabCount > 1) {
		tabs.resize(tabCount);
		ui.tabWidget->show();
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];
			QWidget *widget = new QWidget(q);

			// Layouts.
			// NOTE: We shouldn't zero out the QVBoxLayout margins here.
			// Otherwise, we end up with no margins.
			tab.vbox = new QVBoxLayout(widget);
			tab.form = new QFormLayout();
			tab.vbox->addLayout(tab.form, 1);

			// Add the tab.
			ui.tabWidget->addTab(widget, (name ? U82Q(name) : QString()));
		}
	} else {
		// No tabs.
		// Don't create a QTabWidget, but simulate a single
		// tab in tabs[] to make it easier to work with.
		tabs.resize(1);
		auto &tab = tabs[0];

		// QVBoxLayout
		// NOTE: Using ui.vboxLayout. We must ensure that
		// this isn't deleted.
		tab.vbox = ui.vboxLayout;

		// QFormLayout
		tab.form = new QFormLayout();
		tab.vbox->addLayout(tab.form, 1);
	}

	// TODO: Ensure the description column has the
	// same width on all tabs.

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");

	// Create the data widgets.
	int prevTabIdx = 0;
	const auto pFields_cend = pFields->cend();
	int fieldIdx = 0;
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, fieldIdx++) {
		const RomFields::Field &field = *iter;
		if (!field.isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)tabs.size());
		if (tabIdx < 0 || tabIdx >= (int)tabs.size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs[tabIdx].form) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		// Did the tab index change?
		if (prevTabIdx != tabIdx) {
			// Check if the last field in the previous tab
			// was RFT_LISTDATA. If it is, expand it vertically.
			// NOTE: Only for RFT_LISTDATA_SEPARATE_ROW.
			adjustListData(prevTabIdx);
			prevTabIdx = tabIdx;
		}

		// tr: Field description label.
		string txt = rp_sprintf(desc_label_fmt, field.name.c_str());
		QLabel *lblDesc = new QLabel(U82Q(txt), q);
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);

		switch (field.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				delete lblDesc;
				break;
			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				delete lblDesc;
				break;

			case RomFields::RFT_STRING:
				initString(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_BITFIELD:
				initBitfield(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_LISTDATA:
				initListData(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_DATETIME:
				initDateTime(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_AGE_RATINGS:
				initAgeRatings(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_DIMENSIONS:
				initDimensions(lblDesc, field, fieldIdx);
				break;
			case RomFields::RFT_STRING_MULTI:
				initStringMulti(lblDesc, field, fieldIdx);
				break;
		}
	}

	// Initial update of RFT_STRING_MULTI and RFT_LISTDATA_MULTI fields.
	if (!vecStringMulti.empty() || !vecListDataMulti.empty()) {
		def_lc = pFields->defaultLanguageCode();
		updateMulti(0);
	}

	// Check if the last field in the last tab
	// was RFT_LISTDATA. If it is, expand it vertically.
	// NOTE: Only for RFT_LISTDATA_SEPARATE_ROW.
	if (!tabs.empty()) {
		adjustListData(static_cast<int>(tabs.size()-1));
	}

	// Close the file.
	// Keeping the file open may prevent the user from
	// changing the file.
	romData->close();
}

/** RomDataView **/

RomDataView::RomDataView(QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, nullptr))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Create the "Options" button in the parent window.
	d->createOptionsButton();
}

RomDataView::RomDataView(RomData *romData, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, romData))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Create the "Options" button in the parent window.
	d->createOptionsButton();

	// Initialize the display widgets.
	d->initDisplayWidgets();
}

RomDataView::~RomDataView()
{
	delete d_ptr;
}

/** QWidget overridden functions. **/

/**
 * Window has been hidden.
 * This means that this tab has been selected.
 * @param event QShowEvent.
 */
void RomDataView::showEvent(QShowEvent *event)
{
	// Start the icon animation.
	Q_D(RomDataView);
	d->ui.lblIcon->startAnimTimer();

	// Show the "Options" button.
	if (d->btnOptions) {
		d->btnOptions->show();
	}

	// Pass the event to the superclass.
	super::showEvent(event);
}

/**
 * Window has been hidden.
 * This means that a different tab has been selected.
 * @param event QHideEvent.
 */
void RomDataView::hideEvent(QHideEvent *event)
{
	// Stop the icon animation.
	Q_D(RomDataView);
	d->ui.lblIcon->stopAnimTimer();

	// Hide the "Options" button.
	if (d->btnOptions) {
		d->btnOptions->hide();
	}

	// Pass the event to the superclass.
	super::hideEvent(event);
}

/**
 * Event filter for recalculating RFT_LISTDATA row heights.
 * @param object QObject.
 * @param event Event.
 * @return True to filter the event; false to pass it through.
 */
bool RomDataView::eventFilter(QObject *object, QEvent *event)
{
	// Check the event type.
	switch (event->type()) {
		case QEvent::LayoutRequest:	// Main event we want to handle.
		case QEvent::FontChange:
		case QEvent::StyleChange:
			// FIXME: Adjustments in response to QEvent::StyleChange
			// don't seem to work on Kubuntu 16.10...
			break;

		default:
			// We don't care about this event.
			return false;
	}

	// Make sure this is a QTreeWidget.
	QTreeWidget *const treeWidget = qobject_cast<QTreeWidget*>(object);
	if (!treeWidget) {
		// Not a QTreeWidget.
		return false;
	}

	// Get the requested minimum number of rows.
	// Recalculate the row heights for this QTreeWidget.
	const int rows_visible = treeWidget->property("RFT_LISTDATA_rows_visible").toInt();
	if (rows_visible <= 0) {
		// This QTreeWidget doesn't have a fixed number of rows.
		// Let Qt decide how to manage its layout.
		return false;
	}

	// Get the height of the first item.
	QTreeWidgetItem *const treeWidgetItem = treeWidget->topLevelItem(0);
	assert(treeWidgetItem != nullptr);
	if (!treeWidgetItem) {
		// No items...
		return false;
	}

	QRect rect = treeWidget->visualItemRect(treeWidgetItem);
	if (rect.height() <= 0) {
		// Item has no height?!
		return false;
	}

	// Multiply the height by the requested number of visible rows.
	int height = rect.height() * rows_visible;
	// Add the header.
	if (treeWidget->header()->isVisibleTo(treeWidget)) {
		height += treeWidget->header()->height();
	}
	// Add QTreeWidget borders.
	height += (treeWidget->frameWidth() * 2);

	// Set the QTreeWidget height.
	treeWidget->setMinimumHeight(height);
	treeWidget->setMaximumHeight(height);

	// Allow the event to propagate.
	return false;
}

/** Widget slots. **/

/**
 * Disable user modification of RFT_BITFIELD checkboxes.
 */
void RomDataView::bitfield_clicked_slot(bool checked)
{
	QAbstractButton *sender = qobject_cast<QAbstractButton*>(QObject::sender());
	if (!sender)
		return;

	// Get the saved RFT_BITFIELD value.
	const bool value = sender->property("RFT_BITFIELD_value").toBool();
	if (checked != value) {
		// Toggle this box.
		sender->setChecked(value);
	}
}

/**
 * The RFT_MULTI_STRING language was changed.
 * @param lc Language code. (Cast to uint32_t)
 */
void RomDataView::cboLanguage_currentIndexChanged_slot(int index)
{
	Q_D(RomDataView);
	if (index < 0) {
		// Invalid index...
		return;
	}

	d->updateMulti(d->sel_lc());
}

/** Properties. **/

/**
 * Get the current RomData object.
 * @return RomData object.
 */
RomData *RomDataView::romData(void) const
{
	Q_D(const RomDataView);
	return d->romData;
}

/**
 * Set the current RomData object.
 *
 * If a RomData object is already set, it is unref()'d.
 * The new RomData object is ref()'d when set.
 */
void RomDataView::setRomData(RomData *romData)
{
	Q_D(RomDataView);
	if (d->romData == romData)
		return;

	bool prevAnimTimerRunning = d->ui.lblIcon->isAnimTimerRunning();
	if (prevAnimTimerRunning) {
		// Animation is running.
		// Stop it temporarily and reset the frame number.
		d->ui.lblIcon->stopAnimTimer();
		d->ui.lblIcon->resetAnimFrame();
	}

	UNREF(d->romData);
	d->romData = (romData ? romData->ref() : nullptr);
	d->initDisplayWidgets();

	if (romData != nullptr && prevAnimTimerRunning) {
		// Restart the animation timer.
		// FIXME: Ensure frame 0 is drawn?
		d->ui.lblIcon->startAnimTimer();
	}

	emit romDataChanged(romData);
}

/**
 * An "Options" menu action was triggered.
 * @param id Options ID.
 */
void RomDataView::menuOptions_action_triggered(int id)
{
	// IDs below 0 are for built-in actions.
	// IDs >= 0 are for RomData-specific actions.
	Q_D(RomDataView);

	if (id < 0) {
		// Export/copy to text or JSON.
		const char *const rom_filename = d->romData->filename();
		if (!rom_filename)
			return;
		QFileInfo fi(U82Q(rom_filename));

		bool toClipboard;
		QString s_title, s_filter;
		QString s_default_ext;
		switch (id) {
			case RomDataViewPrivate::OPTION_EXPORT_TEXT:
				toClipboard = false;
				s_title = U82Q(C_("RomDataView", "Export to Text File"));
				// tr: Text files filter. (Qt)
				s_filter = U82Q(C_("RomDataView", "Text Files (*.txt);;All Files (*.*)"));
				s_default_ext = QLatin1String(".txt");
				break;
			case RomDataViewPrivate::OPTION_EXPORT_JSON:
				toClipboard = false;
				s_title = U82Q(C_("RomDataView", "Export to JSON File"));
				// tr: JSON files filter. (Qt)
				s_filter = U82Q(C_("RomDataView", "JSON Files (*.json);;All Files (*.*)"));
				s_default_ext = QLatin1String(".json");
				break;
			case RomDataViewPrivate::OPTION_COPY_TEXT:
			case RomDataViewPrivate::OPTION_COPY_JSON:
				toClipboard = true;
				break;
			default:
				assert(!"Invalid action ID.");
				return;
		}

		// TODO: QTextStream wrapper for ostream.
		// For now, we'll use ofstream.
		ofstream ofs;

		if (!toClipboard) {
			if (d->prevExportDir.isEmpty()) {
				d->prevExportDir = fi.path();
			}

			QString defaultFileName = d->prevExportDir + QChar(L'/') + fi.completeBaseName() + s_default_ext;
			QString out_filename = QFileDialog::getSaveFileName(this,
				s_title, defaultFileName, s_filter);
			if (out_filename.isEmpty())
				return;

			// Save the previous export directory.
			QFileInfo fi2(out_filename);
			d->prevExportDir = fi2.path();

			ofs.open(out_filename.toUtf8().constData(), ofstream::out);
			if (ofs.fail())
				return;
		}

		// TODO: Optimize this such that we can pass ofstream or ostringstream
		// to a factored-out function.

		switch (id) {
			case RomDataViewPrivate::OPTION_EXPORT_TEXT: {
				ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(d->romData, d->sel_lc());
				ofs << ro;
				break;
			}
			case RomDataViewPrivate::OPTION_EXPORT_JSON: {
				JSONROMOutput jsro(d->romData);
				ofs << jsro << std::endl;
				break;
			}
			case RomDataViewPrivate::OPTION_COPY_TEXT: {
				ostringstream oss;
				oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
				ROMOutput ro(d->romData, d->sel_lc());
				oss << ro;
				QApplication::clipboard()->setText(U82Q(oss.str()));
				break;
			}
			case RomDataViewPrivate::OPTION_COPY_JSON: {
				ostringstream oss;
				JSONROMOutput jsro(d->romData);
				oss << jsro << std::endl;
				QApplication::clipboard()->setText(U82Q(oss.str()));
				break;
			}
			default:
				assert(!"Invalid action ID.");
				return;
		}
	} else if (d->romOps_firstActionIndex >= 0) {
		// Run a ROM operation.
		RomData::RomOpResult result;
		int ret = d->romData->doRomOp(id, &result);
		const QString qs_msg = U82Q(result.msg);
		if (ret == 0) {
			// ROM operation completed.

			// Update fields.
			std::for_each(result.fieldIdx.cbegin(), result.fieldIdx.cend(),
				[d](int fieldIdx) {
					d->updateField(fieldIdx);
				}
			);

			// Update the RomOp menu entry in case it changed.
			// NOTE: Assuming the RomOps vector order hasn't changed.
			// TODO: Have RomData store the RomOps vector instead of
			// rebuilding it here?
			const vector<RomData::RomOps> ops = d->romData->romOps();
			assert(id < (int)ops.size());
			if (id < (int)ops.size()) {
				const QObjectList &objList = d->menuOptions->children();
				int actionIndex = d->romOps_firstActionIndex + id;
				assert(actionIndex < objList.size());
				if (actionIndex < objList.size()) {
					QAction *const action = qobject_cast<QAction*>(objList.at(actionIndex));
					if (action) {
						const RomData::RomOps &op = ops[id];
						action->setText(U82Q(op.desc));
						action->setEnabled(!!(op.flags & RomData::RomOps::ROF_ENABLED));
					}
				}
			}

			// Show the message and play the sound.
			const QString qs_msg = QString::fromUtf8("This is a test of the GerbilSoft notification system. Had this been an actual notification, ducks would be quacking. 🦆");//U82Q(result.msg);
			MessageSound::play(QMessageBox::Information, qs_msg, this);
		} else {
			// An error occurred...
			MessageSound::play(QMessageBox::Warning, qs_msg, this);
		}

#ifdef HAVE_KMESSAGEWIDGET
		if (!qs_msg.isEmpty()) {
			if (!d->messageWidget) {
				d->messageWidget = new KMessageWidget(this);
				d->messageWidget->setCloseButtonVisible(true);
				d->messageWidget->setWordWrap(true);
				d->ui.vboxLayout->addWidget(d->messageWidget);

				d->tmrMessageWidget = new QTimer(this);
				d->tmrMessageWidget->setSingleShot(true);
				d->tmrMessageWidget->setInterval(10*1000);
				connect(d->tmrMessageWidget, SIGNAL(timeout()),
				        d->messageWidget, SLOT(animatedHide()));
#  if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
				// KMessageWidget::hideAnimationFinished was added in KF5.
				// FIXME: This is after the animation *finished*, not when
				// the Close button was clicked.
				connect(d->messageWidget, &KMessageWidget::showAnimationFinished,
				        d->tmrMessageWidget, static_cast<void (QTimer::*)()>(&QTimer::start));
				connect(d->messageWidget, &KMessageWidget::hideAnimationFinished,
				        d->tmrMessageWidget, &QTimer::stop);
#  endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */
			}

			d->messageWidget->setMessageType(ret == 0 ? KMessageWidget::Information : KMessageWidget::Warning);
			d->messageWidget->setText(qs_msg);
			d->messageWidget->animatedShow();
//#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
			// KDE4's KMessageWidget doesn't have the "animation finished"
			// signals, so we'll have to start the timer manually.
			d->tmrMessageWidget->start();
//#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
		}
#endif /* HAVE_KMESSAGEWIDGET */
	}
}

/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * RomDataView.cpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RomDataFormat.hpp"

#include "AchQtDBus.hpp"
#include "RpQImageBackend.hpp"

// Other rom-properties libraries
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// libi18n
#include "libi18n/i18n.h"

// C++ STL classes
using std::set;
using std::string;

// Custom widgets
#include "DragImageTreeView.hpp"
#include "LanguageComboBox.hpp"
#include "OptionsMenuButton.hpp"

// Data models
#include "ListDataModel.hpp"
#include "ListDataSortProxyModel.hpp"

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, const RomDataPtr &romData)
	: q_ptr(q)
	, romData(romData)
	, btnOptions(nullptr)
#ifdef HAVE_KMESSAGEWIDGET
	, messageWidget(nullptr)
#endif /* HAVE_KMESSAGEWIDGET */
	, cboLanguage(nullptr)
	, def_lc(0)
	, hasCheckedAchievements(false)
{}

RomDataViewPrivate::~RomDataViewPrivate()
{
	ui.lblIcon->clearRp();
	ui.lblBanner->clearRp();
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
	QDialogButtonBox *buttonBox = nullptr;
	if (pageWidget) {
		buttonBox = findDirectChild<QDialogButtonBox*>(pageWidget);
	}
	if (!buttonBox) {
		// Check in the KPropertiesDialog.
		buttonBox = findDirectChild<QDialogButtonBox*>(parent);
	}
	assert(buttonBox != nullptr);
	if (!buttonBox)
		return;

	// Create the "Options" button.
	btnOptions = new OptionsMenuButton();
	btnOptions->setObjectName(QLatin1String("btnOptions"));
	// NOTE: Using HelpRole to force the button to the left side of the dialog.
	// The previous method added a stretch layout item to the QDialogButtonBox's
	// layout directly, but that doesn't appear to work on Qt6.
	// FIXME: Generally works on KF5/Qt5, but not on Ubuntu 18.04?
	buttonBox->addButton(btnOptions, QDialogButtonBox::HelpRole);
	btnOptions->hide();

	// Connect the OptionsMenuButton's triggered(int) signal.
	QObject::connect(btnOptions, SIGNAL(triggered(int)),
			 q, SLOT(btnOptions_triggered(int)));

	// Initialize the menu options.
	btnOptions->reinitMenu(romData.get());
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

	// System name and file type
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

	const QString sysInfo = U82Q(fmt::format(
		// tr: {0:s} == system name, {1:s} == file type
		FRUN(C_("RomDataView", "{0:s}\n{1:s}")), systemName, fileType));
	ui.lblSysInfo->setText(sysInfo);
	ui.lblSysInfo->show();

	// Supported image types
	const uint32_t imgbf = romData->supportedImageTypes();
	// FIXME: Store the standard image height somewhere else.
	static constexpr int imgStdHeight = 32;
	bool ok = false;

	// Banner
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image_const_ptr img = romData->image(RomData::IMG_INT_BANNER);
		if (img) {
			ok = ui.lblBanner->setRpImage(img);
			if (ok) {
				// Adjust the banner size.
				const QSize bannerSize(img->width(), img->height());
				if (bannerSize.height() != imgStdHeight) {
					// Need to scale the banner label to match the aspect ratio.
					const QSize bannerScaledSize(rintf(
						static_cast<float>(imgStdHeight) * (static_cast<float>(bannerSize.width()) / static_cast<float>(bannerSize.height()))),
						imgStdHeight);
					ui.lblBanner->setMinimumSize(bannerScaledSize);
					ui.lblBanner->setMaximumSize(bannerScaledSize);
					ui.lblBanner->setScaledContents(true);
				} else {
					// Use the original banner size.
					ui.lblBanner->setMinimumSize(bannerSize);
					ui.lblBanner->setMaximumSize(bannerSize);
					ui.lblBanner->setScaledContents(false);
				}
			}
		}
	}
	ui.lblBanner->setVisible(ok);

	// Icon
	ok = false;
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image_const_ptr icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			QSize iconSize;

			// Is this an animated icon?
			const IconAnimDataConstPtr iconAnimData = romData->iconAnimData();
			if (iconAnimData) {
				ok = ui.lblIcon->setIconAnimData(romData->iconAnimData());
				if (ok) {
					// Get the size of the first animated icon frame.
					const int frame = iconAnimData->seq_index[0];
					const rp_image_ptr &img = iconAnimData->frames[frame];
					assert((bool)img);
					if (img) {
						iconSize = QSize(img->width(), img->height());
					} else {
						// Invalid icon frame?
						ui.lblIcon->setIconAnimData(nullptr);
						ok = false;
					}
				}
			}
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = ui.lblIcon->setRpImage(icon);
				if (ok) {
					iconSize = QSize(icon->width(), icon->height());
				}
			}

			if (ok) {
				if (iconSize.height() != imgStdHeight) {
					// Need to scale the icon label to match the aspect ratio.
					const QSize iconScaledSize(rintf(
						static_cast<float>(imgStdHeight) * (static_cast<float>(iconSize.width()) / static_cast<float>(iconSize.height()))),
						imgStdHeight);
					ui.lblIcon->setMinimumSize(iconScaledSize);
					ui.lblIcon->setMaximumSize(iconScaledSize);
					ui.lblIcon->setScaledContents(true);
				} else {
					// Use the original icon size.
					ui.lblIcon->setMinimumSize(iconSize);
					ui.lblIcon->setMaximumSize(iconSize);
					ui.lblIcon->setScaledContents(false);
				}
			}
		}
	}
	ui.lblIcon->setVisible(ok);

	const bool ecksBawks = (romData->fileType() == RomData::FileType::DiscImage &&
	                        systemName && strstr(systemName, "Xbox") != nullptr);
	ui.lblIcon->setEcksBawks(ecksBawks);
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
 * @param str		[in,opt] String data (If nullptr, field data is used)
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initString(QLabel *lblDesc,
	const RomFields::Field &field,
	const QString *str)
{
	// String type.
	Q_Q(RomDataView);
	QLabel *lblString = new QLabel(q);
	// NOTE: No name for this QObject.
	if (field.flags & RomFields::STRF_CREDITS) {
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
			text = U82Q(field.data.str);
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
			lblString->setText(U82Q(field.data.str));
		}
	}

	// Enable strong focus so we can tab into the label.
	lblString->setFocusPolicy(Qt::StrongFocus);

	// Allow the label to be shrunken horizontally.
	// TODO: Scrolling.
	lblString->setMinimumWidth(1);

	// Check for any formatting options. (RFT_STRING only)
	if (field.type == RomFields::RFT_STRING) {
		// Monospace font?
		if (field.flags & RomFields::STRF_MONOSPACE) {
			lblString->setFont(getSystemMonospaceFont());
			lblString->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			vecMonoWidgets.push_back(lblString);
		}

		// "Warning" font?
		if (field.flags & RomFields::STRF_WARNING) {
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
	    (field.flags & RomFields::STRF_CREDITS))
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
			tab.vbox->setContentsMargins(tab.form->contentsMargins());
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

	return lblString;
}

/**
 * Initialize a bitfield.
 * @param lblDesc Description label
 * @param field		[in] RomFields::Field
 * @return QGridLayout*, or nullptr on error.
 */
QGridLayout *RomDataViewPrivate::initBitfield(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// Bitfield type. Create a grid of checkboxes.
	Q_Q(RomDataView);
	const auto &bitfieldDesc = field.desc.bitfield;
	assert(bitfieldDesc.names->size() <= 32);

	QGridLayout *const gridLayout = new QGridLayout();
	// NOTE: No name for this QObject.
	int row = 0, col = 0;
	uint32_t bitfield = field.data.bitfield;
	for (const string &name : *(bitfieldDesc.names)) {
		if (name.empty()) {
			bitfield >>= 1;
			continue;
		}

		QCheckBox *const checkBox = new QCheckBox(q);
		// NOTE: No name for this QObject.

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

		bitfield >>= 1;
	}

	tabs[field.tabIdx].form->addRow(lblDesc, gridLayout);
	return gridLayout;
}

/**
 * Initialize a list data field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @return QTreeView*, or nullptr on error.
 */
QTreeView *RomDataViewPrivate::initListData(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// ListData type. Create a QTreeView.
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
			delete lblDesc;
			return nullptr;
		}

		list_data = &multi->cbegin()->second;
	} else {
		// Single language.
		list_data = field.data.list_data.data.single;
	}

	assert(list_data != nullptr);
	if (!list_data) {
		// No data...
		delete lblDesc;
		return nullptr;
	}

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(field.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(field.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		delete lblDesc;
		return nullptr;
	}

	if (hasIcons) {
		assert(field.data.list_data.mxd.icons != nullptr);
		if (!field.data.list_data.mxd.icons) {
			// No icons vector...
			delete lblDesc;
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
		delete lblDesc;
		return nullptr;
	}

	Q_Q(RomDataView);
	QTreeView *treeView;
	if (hasIcons) {
		treeView = new DragImageTreeView(q);
		// NOTE: No name for this QObject.
		treeView->setDragEnabled(true);
		treeView->setDefaultDropAction(Qt::CopyAction);
		treeView->setDragDropMode(QAbstractItemView::InternalMove);
		// TODO: Get multi-image drag & drop working.
		//treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
		treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	} else {
		treeView = new QTreeView(q);
		// NOTE: No name for this QObject.
		treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	}
	treeView->setRootIsDecorated(false);
	treeView->setAlternatingRowColors(true);

	// DISABLED uniform row heights.
	// Some Xbox 360 achievements take up two lines,
	// while others might take up three or more.
	treeView->setUniformRowHeights(false);

	// Item models
	ListDataModel *const listModel = new ListDataModel(q);
	// NOTE: No name for this QObject.
	ListDataSortProxyModel *const proxyModel = new ListDataSortProxyModel(q);
	proxyModel->setSortingMethods(listDataDesc.col_attrs.sorting);
	proxyModel->setSourceModel(listModel);
	treeView->setModel(proxyModel);

	if (hasIcons) {
		// TODO: Ideal icon size? Using 32x32 for now.
		// NOTE: QTreeView's iconSize only applies to QIcon, not QPixmap.
		const QSize iconSize(32, 32);
		treeView->setIconSize(iconSize);
		listModel->setIconSize(iconSize);
	}

	// Add the field data to the ListDataModel.
	listModel->setField(&field);

	// FIXME: Qt6 is defaulting to sorting by column 0, descending.
	// Qt5 didn't have this issue...
	treeView->header()->setSortIndicator(-1, Qt::AscendingOrder);

	// Set up column and header visibility.
	if (listDataDesc.names) {
		int col = 0;
		for (const string &str : *(listDataDesc.names)) {
			if (col >= colCount)
				break;

			if (str.empty()) {
				// Don't show this column.
				treeView->setColumnHidden(col, true);
			}
			col++;
		}
	} else {
		// Hide the header.
		treeView->header()->hide();
	}

	// Set up column sizing.
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	if (listDataDesc.col_attrs.sizing != 0) {
		// Explicit column sizing was specified.
		// NOTE: RomFields' COLSZ_* enums match QHeaderView::ResizeMode.
		QHeaderView *const pHeader = treeView->header();
		assert(pHeader != nullptr);
		if (pHeader) {
			pHeader->setStretchLastSection(false);
			unsigned int sizing = listDataDesc.col_attrs.sizing;
			for (int i = 0; i < colCount; i++, sizing >>= RomFields::COLSZ_BITS) {
				pHeader->setSectionResizeMode(i, static_cast<QHeaderView::ResizeMode>(sizing & RomFields::COLSZ_MASK));
			}
		}
	} else
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
	{
		// No explicit column sizing.
		// Use default column sizing, but resize columns to contents initially.
		if (!isMulti) {
			// Resize the columns to fit the contents.
			for (int i = 0; i < colCount; i++) {
				treeView->resizeColumnToContents(i);
			}
			treeView->resizeColumnToContents(colCount);
		}
	}

	// Enable sorting.
	// NOTE: sort_dir maps directly to Qt::SortOrder.
	treeView->setSortingEnabled(true);
	if (listDataDesc.col_attrs.sort_col >= 0) {
		treeView->sortByColumn(listDataDesc.col_attrs.sort_col,
			static_cast<Qt::SortOrder>(listDataDesc.col_attrs.sort_dir));
	}

	if (field.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
		// Separate rows.
		tabs[field.tabIdx].form->addRow(lblDesc);
		tabs[field.tabIdx].form->addRow(treeView);
	} else {
		// Single row.
		tabs[field.tabIdx].form->addRow(lblDesc, treeView);
	}

	// Row height is recalculated when the window is first visible
	// and/or the system theme is changed.
	// TODO: Set an actual default number of rows, or let Qt handle it?
	// (Windows uses 5.)
	treeView->setProperty("RFT_LISTDATA_rows_visible", listDataDesc.rows_visible);

	// Install the event filter.
	treeView->installEventFilter(q);

	if (isMulti) {
		vecListDataMulti.emplace_back(treeView, listModel);
	}

	return treeView;
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

	QTreeView *const treeView = qobject_cast<QTreeView*>(liField->widget());
	if (!treeView) {
		// Not a QTreeView.
		return;
	}

	// Move the treeView to the QVBoxLayout.
	int newRow = tab.vbox->count();
	if (tab.lblCredits) {
		newRow--;
	}
	assert(newRow >= 0);
	tab.form->removeItem(liField);
	tab.vbox->insertWidget(newRow, treeView, 999, Qt::Alignment());
	delete liField;

	// Unset this property to prevent the event filter from
	// setting a fixed height.
	treeView->setProperty("RFT_LISTDATA_rows_visible", QVariant());
}

/**
 * Initialize a Date/Time field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initDateTime(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// Date/Time
	if (field.data.date_time == -1) {
		// tr: Invalid date/time
		return initString(lblDesc, field, QC_("RomDataView", "Unknown"));
	}

	QString str = formatDateTime(field.data.date_time, field.flags);
	if (!str.isEmpty()) {
		return initString(lblDesc, field, str);
	}

	// Invalid date/time.
	delete lblDesc;
	return nullptr;
}

/**
 * Initialize an Age Ratings field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initAgeRatings(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// Age ratings.
	const RomFields::age_ratings_t *age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);

	// Convert the age ratings field to a string.
	const QString str = (age_ratings
		? U82Q(RomFields::ageRatingsDecode(age_ratings))
		: QC_("RomDataView", "ERROR"));
	return initString(lblDesc, field, str);
}

/**
 * Initialize a Dimensions field.
 * @param lblDesc Description label
 * @param field		[in] RomFields::Field
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initDimensions(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// Dimensions
	const int *const dimensions = field.data.dimensions;
	return initString(lblDesc, field, formatDimensions(dimensions));
}

/**
 * Initialize a multi-language string field.
 * @param lblDesc	[in] Description label
 * @param field		[in] RomFields::Field
 * @return QLabel*, or nullptr on error.
 */
QLabel *RomDataViewPrivate::initStringMulti(QLabel *lblDesc,
	const RomFields::Field &field)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	// NOTE 2: The string must be an empty QString, not nullptr. Otherwise, it will
	// attempt to use the field's string data, which is invalid.
	const QString qs_empty;
	QLabel *const lblStringMulti = initString(lblDesc, field, &qs_empty);
	if (lblStringMulti) {
		vecStringMulti.emplace_back(lblStringMulti, &field);
	}
	return lblStringMulti;
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
	for (const auto &vsm : vecStringMulti) {
		QLabel *const lblString = vsm.first;
		const RomFields::Field *const pField = vsm.second;
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
			for (const auto &psm : *pStr_multi) {
				set_lc.insert(psm.first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, def_lc, user_lc);
		assert(pStr != nullptr);
		if (pStr) {
			lblString->setText(U82Q(*pStr));
		} else {
			lblString->clear();
		}
	}

	// RFT_LISTDATA_MULTI
	for (const auto &vldm : vecListDataMulti) {
		QTreeView *const treeView = vldm.first;
		ListDataModel *const listModel = vldm.second;

		if (listModel != nullptr) {
			if (!cboLanguage) {
				// Need to add all supported languages.
				// TODO: Do we need to do this for all of them, or just one?
				const set<uint32_t> list_set_lc = listModel->getLCs();
				set_lc.insert(list_set_lc.cbegin(), list_set_lc.cend());
			}

			// Set the language code.
			listModel->setLC(def_lc, user_lc);
		}

		// Resize the columns to fit the contents.
		// NOTE: Only done on first load.
		if (!cboLanguage) {
			const int colCount = treeView->model()->columnCount();
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
			// Check if explicit column sizing was used.
			// If so, only resize columns marked as "interactive".
			QHeaderView *const pHeader = treeView->header();
			assert(pHeader != nullptr);
			if (pHeader && !pHeader->stretchLastSection()) {
				for (int i = 0; i < colCount; i++) {
					if (pHeader->sectionResizeMode(i) == QHeaderView::Interactive) {
						treeView->resizeColumnToContents(i);
					}
				}
			} else
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
			{
				for (int i = 0; i < colCount; i++) {
					treeView->resizeColumnToContents(i);
				}
			}
			// TODO: Not sure if this should be done for explicit column sizing.
			treeView->resizeColumnToContents(colCount);
		}
	}

	if (!cboLanguage && set_lc.size() > 1) {
		// Create the language combobox.
		Q_Q(RomDataView);
		cboLanguage = new LanguageComboBox(q);
		cboLanguage->setObjectName(QLatin1String("cboLanguage"));
		cboLanguage->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
		ui.hboxHeaderRow->addWidget(cboLanguage);

		// Set the languages.
		cboLanguage->setForcePAL(romData->isPAL());
		cboLanguage->setLCs(set_lc);

		// Select the default language.
		uint32_t lc_to_set = 0;
		const auto set_lc_end = set_lc.end();
		if (set_lc.find(def_lc) != set_lc_end) {
			// def_lc was found.
			lc_to_set = def_lc;
		} else if (set_lc.find('en') != set_lc_end) {
			// 'en' was found.
			lc_to_set = 'en';
		} else {
			// Unknown. Select the first language.
			if (!set_lc.empty()) {
				lc_to_set = *(set_lc.cbegin());
			}
		}
		cboLanguage->setSelectedLC(lc_to_set);

		// Connect the signal after everything's been initialized.
		QObject::connect(cboLanguage, SIGNAL(lcChanged(uint32_t)),
		                 q, SLOT(cboLanguage_lcChanged_slot(uint32_t)));
	}
}

/**
 * Initialize the display widgets.
 * If the widgets already exist, they will
 * be deleted and recreated.
 */
void RomDataViewPrivate::initDisplayWidgets(void)
{
	// Clear the tabs.
	for (const tab &tab : tabs) {
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
	tabs.clear();
	ui.tabWidget->clear();
	ui.tabWidget->hide();

	// Clear multi-language stuff.
	def_lc = 0;
	vecStringMulti.clear();
	vecListDataMulti.clear();
	delete cboLanguage;
	cboLanguage = nullptr;

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

	// Initialize the QTabWidget.
	Q_Q(RomDataView);
	const int tabCount = pFields->tabCount();
	if (tabCount > 1) {
		tabs.resize(tabCount);
		ui.tabWidget->show();
		string tab_name;
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *const name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];
			QWidget *const widget = new QWidget(q);
			tab_name = fmt::format(FSTR("tab{:d}"), i);
			widget->setObjectName(QLatin1String(tab_name.c_str()));

			// Layouts.
			// NOTE: We shouldn't zero out the QVBoxLayout margins here.
			// Otherwise, we end up with no margins.
			tab.vbox = new QVBoxLayout(widget);
			tab_name = fmt::format(FSTR("vboxTab{:d}"), i);
			tab.vbox->setObjectName(QLatin1String(tab_name.c_str()));
			tab.form = new QFormLayout();
			tab_name = fmt::format(FSTR("formTab{:d}"), i);
			tab.form->setObjectName(QLatin1String(tab_name.c_str()));
			tab.vbox->addLayout(tab.form, 1);

			// Add the tab.
			ui.tabWidget->addTab(widget, U82Q(name));
		}
	} else {
		// No tabs.
		// Don't initialize the QTabWidget, but simulate a single
		// tab in tabs[] to make it easier to work with.
		tabs.resize(1);
		auto &tab = tabs[0];

		// QVBoxLayout
		// NOTE: Using ui.vboxLayout. We must ensure that
		// this isn't deleted.
		tab.vbox = ui.vboxLayout;
		tab.vbox->setObjectName(QLatin1String("vboxTab0"));

		// QFormLayout
		tab.form = new QFormLayout();
		tab.form->setObjectName(QLatin1String("formTab0"));
		tab.vbox->addLayout(tab.form, 1);
	}

	// TODO: Ensure the description column has the
	// same width on all tabs.

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "{:s}:");

	// Create the data widgets.
	int prevTabIdx = 0;
	int fieldIdx = 0;
	for (const RomFields::Field &field : *pFields) {
		assert(field.isValid());
		if (!field.isValid()) {
			fieldIdx++;
			continue;
		}

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
		assert(tabIdx >= 0 && tabIdx < static_cast<int>(tabs.size()));
		if (tabIdx < 0 || tabIdx >= static_cast<int>(tabs.size())) {
			// Tab index is out of bounds.
			fieldIdx++;
			continue;
		} else if (!tabs[tabIdx].form) {
			// Tab name is empty. Tab is hidden.
			fieldIdx++;
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
		const string txt = fmt::format(FRUN(desc_label_fmt), field.name);
		QLabel *const lblDesc = new QLabel(U82Q(txt), q);
		// NOTE: No name for this QObject.
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);

		QObject *obj;
		switch (field.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				assert(!"Field type is RFT_INVALID");
				obj = nullptr;
				delete lblDesc;
				break;
			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				obj = nullptr;
				delete lblDesc;
				break;

			case RomFields::RFT_STRING:
				obj = initString(lblDesc, field);
				break;
			case RomFields::RFT_BITFIELD:
				obj = initBitfield(lblDesc, field);
				break;
			case RomFields::RFT_LISTDATA:
				obj = initListData(lblDesc, field);
				break;
			case RomFields::RFT_DATETIME:
				obj = initDateTime(lblDesc, field);
				break;
			case RomFields::RFT_AGE_RATINGS:
				obj = initAgeRatings(lblDesc, field);
				break;
			case RomFields::RFT_DIMENSIONS:
				obj = initDimensions(lblDesc, field);
				break;
			case RomFields::RFT_STRING_MULTI:
				obj = initStringMulti(lblDesc, field);
				break;
		}

		if (obj) {
			// Set RFT_fieldIdx for ROM operations.
			obj->setProperty("RFT_fieldIdx", fieldIdx);
		}

		fieldIdx++;
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

	// Add vertical spacers to each QFormLayout.
	// This is mostly needed for e.g. DSi and 3DS permissions.
	for (const tab &tab : tabs) {
		tab.form->addItem(new QSpacerItem(0, 0));
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

	// Add an event filter for the top-level window so we can
	// handle QEvent::StyleChange.
	installEventFilterInTopLevelWidget(this);

	// Create the "Options" button in the parent window.
	d->createOptionsButton();
}

RomDataView::RomDataView(const RomDataPtr &romData, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, romData))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Add an event filter for the top-level window so we can
	// handle QEvent::StyleChange.
	installEventFilterInTopLevelWidget(this);

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
 * Window is now visible.
 * This means that this tab has been selected.
 * @param event QShowEvent
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
 * @param event QHideEvent
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
 * Paint event.
 *
 * The window is technically "shown" and hidden at least once
 * before the tab is selected, which causes the achievement
 * notification to be triggered too early. Wait for an actual
 * paint event before checking for achievements instead.
 *
 * @param event QPaintEvent
 */
void RomDataView::paintEvent(QPaintEvent *event)
{
	// Check for "viewed" achievements.
	Q_D(RomDataView);
	if (!d->hasCheckedAchievements && d->romData) {
		d->romData->checkViewedAchievements();
		d->hasCheckedAchievements = true;
	}

	// Pass the event to the superclass.
	super::paintEvent(event);
}

/**
 * Event filter for QTreeView and top-level windows.
 * @param object QObject
 * @param event Event
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

	// Make sure this is a QTreeView.
	QTreeView *const treeView = qobject_cast<QTreeView*>(object);
	if (!treeView) {
		// Not a QTreeView.
		// Assuming this is a top-level window.
		if (event->type() == QEvent::StyleChange) {
			// Update monospace fonts.
			Q_D(RomDataView);
			for (QWidget *widget : d->vecMonoWidgets) {
				widget->setFont(getSystemMonospaceFont());
			}
		}
		return false;
	}

	// Get the requested minimum number of rows.
	// Recalculate the row heights for this QTreeView.
	const int rows_visible = treeView->property("RFT_LISTDATA_rows_visible").toInt();
	if (rows_visible <= 0) {
		// This QTreeView doesn't have a fixed number of rows.
		// Let Qt decide how to manage its layout.
		return false;
	}

	// Get the height of the first item.
	QAbstractItemModel *const model = treeView->model();
	const QRect rect = treeView->visualRect(model->index(0, 0));
	if (rect.height() <= 0) {
		// Item has no height?
		return false;
	}

	// Multiply the height by the requested number of visible rows.
	int height = rect.height() * rows_visible;
	// Add the header.
	QHeaderView *const header = treeView->header();
	if (header && header->isVisibleTo(treeView)) {
		height += header->height();
	}
	// Add QTreeView borders.
	height += (treeView->frameWidth() * 2);

	// Set the QTreeView height.
	treeView->setMinimumHeight(height);
	treeView->setMaximumHeight(height);

	// Allow the event to propagate.
	return false;
}

/** Widget slots **/

/**
 * Disable user modification of RFT_BITFIELD checkboxes.
 */
void RomDataView::bitfield_clicked_slot(bool checked)
{
	QAbstractButton *const sender = qobject_cast<QAbstractButton*>(QObject::sender());
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
 * @param lc Language code.
 */
void RomDataView::cboLanguage_lcChanged_slot(uint32_t lc)
{
	Q_D(RomDataView);
	d->updateMulti(lc);
}

/** Properties **/

/**
 * Get the current RomData object.
 * @return RomData object
 */
RomDataPtr RomDataView::romData(void) const
{
	Q_D(const RomDataView);
	return d->romData;
}

/**
 * Set the current RomData object.
 *
 * If a RomData object is already set, it is unref()'d.
 * The new RomData object is ref()'d when set.
 *
 * @param romData New RomData object
 */
void RomDataView::setRomData(const RomDataPtr &romData)
{
	Q_D(RomDataView);
	if (d->romData == romData)
		return;

	const bool prevAnimTimerRunning = d->ui.lblIcon->isAnimTimerRunning();
	if (prevAnimTimerRunning) {
		// Animation is running.
		// Stop it temporarily and reset the frame number.
		d->ui.lblIcon->stopAnimTimer();
		d->ui.lblIcon->resetAnimFrame();
	}

	d->romData = romData;
	d->initDisplayWidgets();

	if (romData && prevAnimTimerRunning) {
		// Restart the animation timer.
		// FIXME: Ensure frame 0 is drawn?
		d->ui.lblIcon->startAnimTimer();
	}

	// FIXME: Not compatible with std::shared_ptr<>.
	//emit romDataChanged(romData);
}

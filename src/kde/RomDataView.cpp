/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomDataView.cpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <array>
#include <string>
#include <vector>
using std::array;
using std::string;
using std::vector;

#include <QtCore/QDateTime>
#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QLabel>
#include <QCheckBox>
#include <QHeaderView>
#include <QSpacerItem>
#include <QTreeWidget>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>

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

		// Tab contents.
		struct tab {
			QVBoxLayout *vboxLayout;
			QFormLayout *formLayout;
			QLabel *lblCredits;
		};
		vector<tab> tabs;

		// RomData object.
		RomData *romData;

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
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 * @param str		[in,opt] String data. (If nullptr, field data is used.)
		 */
		void initString(QLabel *lblDesc, const RomFields::Field *field, const QString *str = nullptr);

		/**
		 * Initialize a string field.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 * @param str		[in,opt] String data. (If nullptr, field data is used.)
		 */
		inline void initString(QLabel *lblDesc, const RomFields::Field *field, const QString &str)
		{
			return initString(lblDesc, field, &str);
		}

		/**
		 * Initialize a bitfield.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 */
		void initBitfield(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize a list data field.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 */
		void initListData(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Adjust an RFT_LISTDATA field if it's the last field in a tab.
		 * @param tabIdx Tab index.
		 */
		void adjustListData(int tabIdx);

		/**
		 * Initialize a Date/Time field.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 */
		void initDateTime(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize an Age Ratings field.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 */
		void initAgeRatings(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize a Dimensions field.
		 * @param lblDesc	[in] Description label.
		 * @param field		[in] RomFields::Field
		 */
		void initDimensions(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize the display widgets.
		 * If the widgets already exist, they will
		 * be deleted and recreated.
		 */
		void initDisplayWidgets(void);

		/**
		 * Convert a QImage to QPixmap.
		 * Automatically resizes the QImage if it's smaller
		 * than the minimum size.
		 * @param img QImage.
		 * @return QPixmap.
		 */
		QPixmap imgToPixmap(const QImage &img);

		/**
		 * Set the label's pixmap using an rp_image source.
		 * @param label QLabel.
		 * @param img rp_image. (If nullptr, returns an error; use clear() to clear it.)
		 * @return True on success; false on error.
		 */
		bool setPixmapFromRpImage(QLabel *label, const rp_image *img);
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
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
	if (romData) {
		romData->unref();
	}
}

/**
 * Convert a QImage to QPixmap.
 * Automatically resizes the QImage if it's smaller
 * than the minimum size.
 * @param img QImage.
 * @return QPixmap.
 */
QPixmap RomDataViewPrivate::imgToPixmap(const QImage &img)
{
	// Minimum image size.
	// If images are smaller, they will be resized.
	// TODO: Adjust minimum size for DPI.
	const QSize min_img_size(32, 32);

	if (img.width() >= min_img_size.width() &&
	    img.height() >= min_img_size.height())
	{
		// No resize necessary.
		return QPixmap::fromImage(img);
	}

	// Resize the image.
	QSize img_size = img.size();
	do {
		// Increase by integer multiples until
		// the icon is at least 32x32.
		// TODO: Constrain to 32x32?
		img_size.setWidth(img_size.width() + img.width());
		img_size.setHeight(img_size.height() + img.height());
	} while (img_size.width() < min_img_size.width() &&
		 img_size.height() < min_img_size.height());

	return QPixmap::fromImage(img.scaled(img_size, Qt::KeepAspectRatio, Qt::FastTransformation));
}

/**
 * Set the label's pixmap using an rp_image source.
 * @param label QLabel.
 * @param img rp_image. (If nullptr, returns an error; use clear() to clear it.)
 * @return True on success; false on error.
 */
bool RomDataViewPrivate::setPixmapFromRpImage(QLabel *label, const rp_image *img)
{
	assert(img != nullptr);
	if (!img || !img->isValid()) {
		// No image, or image is not valid.
		return false;
	}

	// Convert the rp_image to a QImage.
	QImage qImg = rpToQImage(img);
	if (qImg.isNull()) {
		// Unable to convert the image.
		return false;
	}

	// Image converted successfully.
	label->setPixmap(imgToPixmap(qImg));
	return true;
}

/**
 * Initialize the header row widgets.
 * The widgets must have already been created by ui.setupUi().
 */
void RomDataViewPrivate::initHeaderRow(void)
{
	Q_Q(RomDataView);
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
 * @param lblDesc	[in] Description label.
 * @param field		[in] RomFields::Field
 * @param str		[in,opt] String data. (If nullptr, field data is used.)
 */
void RomDataViewPrivate::initString(QLabel *lblDesc, const RomFields::Field *field, const QString *str)
{
	// String type.
	Q_Q(RomDataView);
	QLabel *lblString = new QLabel(q);
	if (field->desc.flags & RomFields::STRF_CREDITS) {
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
		} else if (field->data.str) {
			text = U82Q(*(field->data.str));
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
		} else if (field->data.str) {
			lblString->setText(U82Q(*(field->data.str)));
		}
	}

	// Enable strong focus so we can tab into the label.
	lblString->setFocusPolicy(Qt::StrongFocus);

	// Check for any formatting options. (RFT_STRING only)
	if (field->type == RomFields::RFT_STRING) {
		// Monospace font?
		if (field->desc.flags & RomFields::STRF_MONOSPACE) {
			QFont font(QLatin1String("Monospace"));
			font.setStyleHint(QFont::TypeWriter);
			lblString->setFont(font);
			lblString->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		}

		// "Warning" font?
		if (field->desc.flags & RomFields::STRF_WARNING) {
			// Only expecting a maximum of one "Warning" per ROM,
			// so we're initializing this here.
			const QString css = QLatin1String("color: #F00; font-weight: bold;");
			lblDesc->setStyleSheet(css);
			lblString->setStyleSheet(css);
		}
	}

	// Credits?
	auto &tab = tabs[field->tabIdx];
	if (field->type == RomFields::RFT_STRING &&
	    (field->desc.flags & RomFields::STRF_CREDITS))
	{
		// Credits row goes at the end.
		// There should be a maximum of one STRF_CREDITS per tab.
		assert(tab.lblCredits == nullptr);
		if (!tab.lblCredits) {
			// Save this as the credits label.
			tab.lblCredits = lblString;
			// Add the credits label to the end of the QVBoxLayout.
			tab.vboxLayout->addWidget(lblString, 0, Qt::AlignHCenter | Qt::AlignBottom);

			// Set the bottom margin to match the QFormLayout.
			// TODO: Use a QHBoxLayout whose margins match the QFormLayout?
			// TODO: Verify this.
			QMargins margins = tab.formLayout->contentsMargins();
			tab.vboxLayout->setContentsMargins(margins);
		} else {
			// Duplicate credits label.
			delete lblString;
		}

		// No description field.
		delete lblDesc;
	} else {
		// Standard string row.
		tab.formLayout->addRow(lblDesc, lblString);
	}
}

/**
 * Initialize a bitfield.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initBitfield(QLabel *lblDesc, const RomFields::Field *field)
{
	// Bitfield type. Create a grid of checkboxes.
	Q_Q(RomDataView);
	const auto &bitfieldDesc = field->desc.bitfield;
	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32)
		count = 32;

	QGridLayout *gridLayout = new QGridLayout();
	int row = 0, col = 0;
	auto iter = bitfieldDesc.names->cbegin();
	for (int bit = 0; bit < count; bit++, ++iter) {
		const string &name = *iter;
		if (name.empty())
			continue;

		// TODO: Disable KDE's automatic mnemonic.
		QCheckBox *checkBox = new QCheckBox(q);
		checkBox->setText(U82Q(name));
		bool value = !!(field->data.bitfield & (1 << bit));
		checkBox->setChecked(value);

		// Save the bitfield checkbox's value in the QObject.
		checkBox->setProperty("RFT_BITFIELD_value", value);

		// Disable user modifications.
		// TODO: Prevent the initial mousebutton down from working;
		// otherwise, it shows a partial check mark.
		QObject::connect(checkBox, SIGNAL(toggled(bool)),
				 q, SLOT(bitfield_toggled_slot(bool)));

		gridLayout->addWidget(checkBox, row, col, 1, 1);
		col++;
		if (col == bitfieldDesc.elemsPerRow) {
			row++;
			col = 0;
		}
	}
	tabs[field->tabIdx].formLayout->addRow(lblDesc, gridLayout);
}

/**
 * Initialize a list data field.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initListData(QLabel *lblDesc, const RomFields::Field *field)
{
	// ListData type. Create a QTreeWidget.
	const auto &listDataDesc = field->desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	const auto list_data = field->data.list_data.data;
	assert(list_data != nullptr);

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		return;
	}

	if (hasIcons) {
		assert(field->data.list_data.mxd.icons != nullptr);
		if (!field->data.list_data.mxd.icons) {
			// No icons vector...
			return;
		}
	}

	unsigned int col_count = 1;
	if (listDataDesc.names) {
		col_count = static_cast<unsigned int>(listDataDesc.names->size());
	} else {
		// No column headers.
		// Use the first row.
		if (list_data && !list_data->empty()) {
			col_count = static_cast<unsigned int>(list_data->at(0).size());
		}
	}

	Q_Q(RomDataView);
	QTreeWidget *const treeWidget = new QTreeWidget(q);
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
	treeWidget->setColumnCount(col_count);
	if (listDataDesc.names) {
		QStringList columnNames;
		columnNames.reserve(col_count);
		QTreeWidgetItem *const header = treeWidget->headerItem();
		uint32_t align = listDataDesc.alignment.headers;
		auto iter = listDataDesc.names->cbegin();
		for (int col = 0; col < (int)col_count; col++, ++iter, align >>= 2) {
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
	if (list_data) {
		uint32_t checkboxes = 0;
		if (hasCheckboxes) {
			checkboxes = field->data.list_data.mxd.checkboxes;
		}

		unsigned int row = 0;	// for icons [TODO: Use iterator?]
		for (auto iter = list_data->cbegin(); iter != list_data->cend(); ++iter, row++) {
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
				const rp_image *const icon = field->data.list_data.mxd.icons->at(row);
				if (icon) {
					treeWidgetItem->setIcon(0, QIcon(
						QPixmap::fromImage(rpToQImage(icon))));
				}
			}

			// Disable user checkability.
			treeWidgetItem->setFlags(treeWidgetItem->flags() & ~Qt::ItemIsUserCheckable);

			int col = 0;
			uint32_t align = listDataDesc.alignment.data;
			std::for_each(data_row.cbegin(), data_row.cend(),
				[treeWidgetItem, &col, &align](const string &str) {
					treeWidgetItem->setData(col, Qt::DisplayRole, U82Q(str));
					treeWidgetItem->setTextAlignment(col, align_tbl[align & 3]);
					col++;
					align >>= 2;
				}
			);
		}
	}

	// Resize the columns to fit the contents.
	for (unsigned int i = 0; i < col_count; i++) {
		treeWidget->resizeColumnToContents(static_cast<int>(i));
	}
	treeWidget->resizeColumnToContents(col_count);

	if (listDataDesc.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
		// Separate rows.
		tabs[field->tabIdx].formLayout->addRow(lblDesc);
		tabs[field->tabIdx].formLayout->addRow(treeWidget);
	} else {
		// Single row.
		tabs[field->tabIdx].formLayout->addRow(lblDesc, treeWidget);
	}

	// Row height is recalculated when the window is first visible
	// and/or the system theme is changed.
	// TODO: Set an actual default number of rows, or let Qt handle it?
	// (Windows uses 5.)
	treeWidget->setProperty("RFT_LISTDATA_rows_visible", listDataDesc.rows_visible);

	// Install the event filter.
	treeWidget->installEventFilter(q);
}

/**
 * Adjust an RFT_LISTDATA field if it's the last field in a tab.
 * @param tabIdx Tab index.
 */
void RomDataViewPrivate::adjustListData(int tabIdx)
{
	auto &tab = tabs[tabIdx];
	assert(tab.formLayout != nullptr);
	if (!tab.formLayout)
		return;
	int row = tab.formLayout->rowCount();
	if (row <= 0)
		return;
	row--;

	QLayoutItem *const liLabel = tab.formLayout->itemAt(row, QFormLayout::LabelRole);
	QLayoutItem *const liField = tab.formLayout->itemAt(row, QFormLayout::FieldRole);
	if (liLabel || !liField) {
		// Either we have a label, or we don't have a field.
		// This is not RFT_LISTDATA_SEPARATE_ROW.
		return;
	}

	QTreeWidget *const treeWidget = qobject_cast<QTreeWidget*>(liField->widget());
	if (treeWidget) {
		// Move the treeWidget to the QVBoxLayout.
		int newRow = tab.vboxLayout->count();
		if (tab.lblCredits) {
			newRow--;
		}
		assert(newRow >= 0);
		tab.formLayout->removeItem(liField);
		tab.vboxLayout->insertWidget(newRow, treeWidget, 999, 0);
		delete liField;

		// Unset this property to prevent the event filter from
		// setting a fixed height.
		treeWidget->setProperty("RFT_LISTDATA_rows_visible", 0);
	}
}

/**
 * Initialize a Date/Time field.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initDateTime(QLabel *lblDesc, const RomFields::Field *field)
{
	// Date/Time.
	if (field->data.date_time == -1) {
		// tr: Invalid date/time.
		initString(lblDesc, field, U82Q(C_("RomDataView", "Unknown")));
		return;
	}

	QDateTime dateTime;
	dateTime.setTimeSpec(
		(field->desc.flags & RomFields::RFT_DATETIME_IS_UTC)
			? Qt::UTC : Qt::LocalTime);
	dateTime.setMSecsSinceEpoch(field->data.date_time * 1000);

	QString str;
	switch (field->desc.flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK) {
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
		initString(lblDesc, field, str);
	} else {
		// Invalid date/time.
		delete lblDesc;
	}
}

/**
 * Initialize an Age Ratings field.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initAgeRatings(QLabel *lblDesc, const RomFields::Field *field)
{
	// Age ratings.
	Q_Q(RomDataView);
	QLabel *lblAgeRatings = new QLabel(q);
	lblAgeRatings->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	lblAgeRatings->setTextFormat(Qt::PlainText);
	lblAgeRatings->setTextInteractionFlags(
		Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	lblAgeRatings->setFocusPolicy(Qt::StrongFocus);

	const RomFields::age_ratings_t *age_ratings = field->data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// tr: No age ratings data.
		lblAgeRatings->setText(U82Q(C_("RomDataView", "ERROR")));
		tabs[field->tabIdx].formLayout->addRow(lblDesc, lblAgeRatings);
		return;
	}

	// Convert the age ratings field to a string.
	QString str = U82Q(RomFields::ageRatingsDecode(age_ratings));
	lblAgeRatings->setText(str);
	tabs[field->tabIdx].formLayout->addRow(lblDesc, lblAgeRatings);
}

/**
 * Initialize a Dimensions field.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initDimensions(QLabel *lblDesc, const RomFields::Field *field)
{
	// Dimensions.
	Q_Q(RomDataView);
	QLabel *lblDimensions = new QLabel(q);
	lblDimensions->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	lblDimensions->setTextFormat(Qt::PlainText);
	lblDimensions->setTextInteractionFlags(
		Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	lblDimensions->setFocusPolicy(Qt::StrongFocus);

	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field->data.dimensions;
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

	lblDimensions->setText(QLatin1String(buf));
	tabs[field->tabIdx].formLayout->addRow(lblDesc, lblDimensions);
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
			if (tab.formLayout) {
				clearLayout(tab.formLayout);
				delete tab.formLayout;
			}
			// Delete the QVBoxLayout.
			if (tab.vboxLayout != ui.vboxLayout) {
				delete tab.vboxLayout;
			}
		}
	);
	tabs.clear();
	ui.tabWidget->clear();
	ui.tabWidget->hide();

	// Initialize the header row.
	initHeaderRow();

	if (!romData) {
		// No ROM data to display.
		return;
	}

	// Get the fields.
	const RomFields *fields = romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Create the QTabWidget.
	Q_Q(RomDataView);
	const int tabCount = fields->tabCount();
	if (tabCount > 1) {
		tabs.resize(tabCount);
		ui.tabWidget->show();
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *name = fields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];
			QWidget *widget = new QWidget(q);

			// Layouts.
			// NOTE: We shouldn't zero out the QVBoxLayout margins here.
			// Otherwise, we end up with no margins.
			tab.vboxLayout = new QVBoxLayout(widget);
			tab.formLayout = new QFormLayout();
			tab.vboxLayout->addLayout(tab.formLayout, 1);

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
		tab.vboxLayout = ui.vboxLayout;

		// QFormLayout
		tab.formLayout = new QFormLayout();
		tab.vboxLayout->addLayout(tab.formLayout, 1);
	}

	// TODO: Ensure the description column has the
	// same width on all tabs.

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");

	// Create the data widgets.
	int prevTabIdx = 0;
	for (int i = 0; i < count; i++) {
		const RomFields::Field *const field = fields->field(i);
		assert(field != nullptr);
		if (!field || !field->isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field->tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)tabs.size());
		if (tabIdx < 0 || tabIdx >= (int)tabs.size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs[tabIdx].formLayout) {
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
		string txt = rp_sprintf(desc_label_fmt, field->name.c_str());
		QLabel *lblDesc = new QLabel(U82Q(txt.c_str()), q);
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);

		switch (field->type) {
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
				initString(lblDesc, field);
				break;
			case RomFields::RFT_BITFIELD:
				initBitfield(lblDesc, field);
				break;
			case RomFields::RFT_LISTDATA:
				initListData(lblDesc, field);
				break;
			case RomFields::RFT_DATETIME:
				initDateTime(lblDesc, field);
				break;
			case RomFields::RFT_AGE_RATINGS:
				initAgeRatings(lblDesc, field);
				break;
			case RomFields::RFT_DIMENSIONS:
				initDimensions(lblDesc, field);
				break;
		}
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

	// No display widgets to initialize...
}

RomDataView::RomDataView(RomData *romData, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, romData))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

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
	return false;

	// Get the height of the first item.
	QTreeWidgetItem *const item = treeWidget->topLevelItem(0);
	assert(item != 0);
	if (!item) {
		// No items...
		return false;
	}

	QRect rect = treeWidget->visualItemRect(item);
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
void RomDataView::bitfield_toggled_slot(bool checked)
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

	if (d->romData) {
		d->romData->unref();
	}
	d->romData = (romData ? romData->ref() : nullptr);
	d->initDisplayWidgets();

	if (romData != nullptr && prevAnimTimerRunning) {
		// Restart the animation timer.
		// FIXME: Ensure frame 0 is drawn?
		d->ui.lblIcon->startAnimTimer();
	}

	emit romDataChanged(romData);
}

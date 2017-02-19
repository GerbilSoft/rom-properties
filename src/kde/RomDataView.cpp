/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomDataView.hpp: RomData viewer.                                        *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/TextFuncs.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/IconAnimData.hpp"
#include "libromdata/img/IconAnimHelper.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>

// C++ includes.
#include <array>
#include <unordered_map>
#include <vector>
using std::unordered_map;
using std::vector;

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include <QLabel>
#include <QCheckBox>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QTreeWidget>

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
		struct Ui {
			void setupUi(QWidget *RomDataView);

			QVBoxLayout *vboxLayout;

			// Header row.
			QHBoxLayout *hboxHeaderRow;
			QLabel *lblSysInfo;
			QLabel *lblBanner;
			QLabel *lblIcon;
			QTimer *tmrIconAnim;

			// Tab layout.
			QTabWidget *tabWidget;
			struct tab {
				QVBoxLayout *vboxLayout;
				QFormLayout *formLayout;
				QLabel *lblCredits;
			};
			vector<tab> tabs;

			Ui()	: vboxLayout(nullptr)
				, hboxHeaderRow(nullptr)
				, lblSysInfo(nullptr)
				, lblBanner(nullptr)
				, lblIcon(nullptr)
				, tmrIconAnim(nullptr)
				, tabWidget(nullptr)
				{ }
			~Ui() {
				// hboxHeaderRow may need to be deleted.
				delete hboxHeaderRow;
			}
		};
		Ui ui;
		RomData *romData;

		// Map of bitfield checkboxes to their values.
		// Used to prevent the user from changing the values.
		unordered_map<QAbstractButton*, bool> mapBitfields;

		// Animated icon data.
		const IconAnimData *iconAnimData;
		std::array<QPixmap, IconAnimData::MAX_FRAMES> iconFrames;
		IconAnimHelper iconAnimHelper;
		bool anim_running;		// Animation is running.
		int last_frame_number;		// Last frame number.

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
		 * @param lblDesc Description label.
		 * @param field RomFields::Field
		 */
		void initString(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize a bitfield.
		 * @param lblDesc Description label.
		 * @param field RomFields::Field
		 */
		void initBitfield(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize a list data field.
		 * @param lblDesc Description label.
		 * @param field RomFields::Field
		 */
		void initListData(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize a Date/Time field.
		 * @param lblDesc Description label.
		 * @param field RomFields::Field
		 */
		void initDateTime(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize an Age Ratings field.
		 * @param lblDesc Description label.
		 * @param field RomFields::Field
		 */
		void initAgeRatings(QLabel *lblDesc, const RomFields::Field *field);

		/**
		 * Initialize the display widgets.
		 * If the widgets already exist, they will
		 * be deleted and recreated.
		 */
		void initDisplayWidgets(void);

		/**
		 * Start the animation timer.
		 */
		void startAnimTimer(void);

		/**
		 * Stop the animation timer.
		 */
		void stopAnimTimer(void);

		/**
		 * Convert a QImage to QPixmap.
		 * Automatically resizes the QImage if it's smaller
		 * than the minimum size.
		 * @param img QImage.
		 * @return QPixmap.
		 */
		QPixmap imgToPixmap(const QImage &img);
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
	, romData(romData->ref())
	, iconAnimData(nullptr)
	, anim_running(false)
	, last_frame_number(0)
{
	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
}

RomDataViewPrivate::~RomDataViewPrivate()
{
	stopAnimTimer();
	iconAnimHelper.setIconAnimData(nullptr);
	if (romData) {
		romData->unref();
	}
}

void RomDataViewPrivate::Ui::setupUi(QWidget *RomDataView)
{
	// Only the layouts and header row widgets are initialized here.
	// Header row data is initialized in initHeaderRow().
	// Field widgets are initialized in initDisplayWidgets().

	// Main layout is a QVBoxLayout.
	// This allows for the credits row to be separate.
	vboxLayout = new QVBoxLayout(RomDataView);
	// Zero the margins for the QVBoxLayout, since usually
	// only the QFormLayout is present.
	QMargins margins(0, 0, 0, 0);
	vboxLayout->setContentsMargins(margins);

	// Create the header row widgets.
	hboxHeaderRow = new QHBoxLayout();
	vboxLayout->addLayout(hboxHeaderRow);

	// Header row: System information.
	lblSysInfo = new QLabel(RomDataView);
	lblSysInfo->hide();
	lblSysInfo->setAlignment(Qt::AlignCenter);
	lblSysInfo->setTextFormat(Qt::PlainText);
	hboxHeaderRow->addWidget(lblSysInfo);

	// Use a bold font.
	QFont font = lblSysInfo->font();
	font.setBold(true);
	lblSysInfo->setFont(font);

	// Header row: Banner and icon.
	lblBanner = new QLabel(RomDataView);
	lblBanner->hide();
	hboxHeaderRow->addWidget(lblBanner);
	lblIcon = new QLabel(RomDataView);
	lblIcon->hide();
	hboxHeaderRow->addWidget(lblIcon);

	// Header row: Add spacers.
	hboxHeaderRow->insertStretch(0, 1);
	hboxHeaderRow->insertStretch(-1, 1);

	// formLayout is created in initDisplayWidgets().
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

	// System name.
	// TODO: System logo and/or game title?
	const rp_char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *const fileType = romData->fileType_string();

	QString sysInfo;
	if (systemName) {
		sysInfo = RP2Q(systemName);
	}
	if (fileType) {
		if (!sysInfo.isEmpty()) {
			sysInfo += QChar(L'\n');
		}
		sysInfo += RP2Q(fileType);
	}

	if (!sysInfo.isEmpty()) {
		ui.lblSysInfo->setText(sysInfo);
		ui.lblSysInfo->show();
	} else {
		ui.lblSysInfo->hide();
	}

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image *banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			QImage img = rpToQImage(banner);
			if (!img.isNull()) {
				ui.lblBanner->setPixmap(imgToPixmap(img));
				ui.lblBanner->show();
			} else {
				ui.lblBanner->hide();
			}
		}
	}

	// Icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Is this an animated icon?
			iconAnimData = romData->iconAnimData();
			if (iconAnimData) {
				// Convert the icons to QPixmaps.
				for (int i = 0; i < iconAnimData->count; i++) {
					const rp_image *const frame = iconAnimData->frames[i];
					if (frame && frame->isValid()) {
						QImage img = rpToQImage(frame);
						if (!img.isNull()) {
							iconFrames[i] = imgToPixmap(img);
						}
					}
				}

				// Set up the IconAnimHelper.
				iconAnimHelper.setIconAnimData(iconAnimData);
				if (iconAnimHelper.isAnimated()) {
					// Initialize the animation.
					last_frame_number = iconAnimHelper.frameNumber();
					// Create the animation timer.
					if (!ui.tmrIconAnim) {
						ui.tmrIconAnim = new QTimer(q);
						ui.tmrIconAnim->setSingleShot(true);
						QObject::connect(ui.tmrIconAnim, SIGNAL(timeout()),
								q, SLOT(tmrIconAnim_timeout()));
					}
				}

				// Show the first frame.
				ui.lblIcon->setPixmap(iconFrames[iconAnimHelper.frameNumber()]);
				ui.lblIcon->show();

				// Icon animation timer is set in startAnimTimer().
			} else {
				// Not an animated icon.
				last_frame_number = 0;
				QImage img = rpToQImage(icon);
				if (!img.isNull()) {
					iconFrames[0] = imgToPixmap(img);
					ui.lblIcon->setPixmap(iconFrames[0]);
					ui.lblIcon->show();
				} else {
					ui.lblIcon->hide();
				}
			}
		}
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
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initString(QLabel *lblDesc, const RomFields::Field *field)
{
	// String type.
	Q_Q(RomDataView);
	QLabel *lblString = new QLabel(q);
	if (field->desc.flags & RomFields::STRF_CREDITS) {
		// Credits text. Enable formatting and center text.
		lblString->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		lblString->setTextFormat(Qt::RichText);
		lblString->setOpenExternalLinks(true);
		if (field->data.str) {
			// Replace newlines with "<br/>".
			QString text = RP2Q(*(field->data.str)).replace(QChar(L'\n'), QLatin1String("<br/>"));
			lblString->setText(text);
		}
	} else {
		// Standard text with no formatting.
		lblString->setTextInteractionFlags(
			Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse |
			Qt::LinksAccessibleByKeyboard | Qt::TextSelectableByKeyboard);
		lblString->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblString->setTextFormat(Qt::PlainText);
		if (field->data.str) {
			lblString->setText(RP2Q(*(field->data.str)));
		}
	}

	// Check for any formatting options.

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

	// Credits?
	auto &tab = ui.tabs[field->tabIdx];
	if (field->desc.flags & RomFields::STRF_CREDITS) {
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

	int count = bitfieldDesc.elements;
	assert(count <= (int)bitfieldDesc.names->size());
	if (count > (int)bitfieldDesc.names->size()) {
		count = (int)bitfieldDesc.names->size();
	}

	QGridLayout *gridLayout = new QGridLayout();
	int row = 0, col = 0;
	for (int bit = 0; bit < count; bit++) {
		const rp_string &name = bitfieldDesc.names->at(bit);
		if (name.empty())
			continue;

		// TODO: Disable KDE's automatic mnemonic.
		QCheckBox *checkBox = new QCheckBox(q);
		checkBox->setText(RP2Q(name));
		bool value = !!(field->data.bitfield & (1 << bit));
		checkBox->setChecked(value);

		// Save the bitfield checkbox in the map.
		mapBitfields.insert(std::make_pair(checkBox, value));

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
	ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, gridLayout);
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
	assert(listDataDesc.names != nullptr);
	if (!listDataDesc.names) {
		// No column names...
		return;
	}

	Q_Q(RomDataView);
	QTreeWidget *treeWidget = new QTreeWidget(q);
	treeWidget->setRootIsDecorated(false);
	treeWidget->setUniformRowHeights(true);

	// Set up the column names.
	const int count = (int)listDataDesc.names->size();
	treeWidget->setColumnCount(count);
	QStringList columnNames;
	columnNames.reserve(count);
	for (int i = 0; i < count; i++) {
		const rp_string &name = listDataDesc.names->at(i);
		if (!name.empty()) {
			columnNames.append(RP2Q(name));
		} else {
			// Don't show this column.
			columnNames.append(QString());
			treeWidget->setColumnHidden(i, true);
		}
	}
	treeWidget->setHeaderLabels(columnNames);

	// Add the row data.
	auto list_data = field->data.list_data;
	assert(list_data != nullptr);
	if (list_data) {
		const int count = (int)list_data->size();
		for (int i = 0; i < count; i++) {
			const vector<rp_string> &data_row = list_data->at(i);
			QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeWidget);
			int col = 0;
			for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, ++col) {
				treeWidgetItem->setData(col, Qt::DisplayRole, RP2Q(*iter));
			}
		}
	}

	// Resize the columns to fit the contents.
	for (int i = 0; i < count; i++) {
		treeWidget->resizeColumnToContents(i);
	}
	treeWidget->resizeColumnToContents(count);

	ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, treeWidget);
}

/**
 * Initialize a Date/Time field.
 * @param lblDesc Description label.
 * @param field RomFields::Field
 */
void RomDataViewPrivate::initDateTime(QLabel *lblDesc, const RomFields::Field *field)
{
	// Date/Time.
	Q_Q(RomDataView);
	QLabel *lblDateTime = new QLabel(q);
	lblDateTime->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	lblDateTime->setTextFormat(Qt::PlainText);
	lblDateTime->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

	if (field->data.date_time == -1) {
		// Invalid date/time.
		lblDateTime->setText(RomDataView::tr("Unknown"));
		ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, lblDateTime);
		return;
	}

	QDateTime dateTime;
	dateTime.setTimeSpec(
		(field->desc.flags & RomFields::RFT_DATETIME_IS_UTC)
			? Qt::UTC : Qt::LocalTime);
	dateTime.setMSecsSinceEpoch(field->data.date_time * 1000);

	QString str;
	switch (field->desc.flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME))
	{
		case RomFields::RFT_DATETIME_HAS_DATE:
			// Date only.
			str = dateTime.date().toString(Qt::DefaultLocaleShortDate);
			break;

		case RomFields::RFT_DATETIME_HAS_TIME:
			// Time only.
			str = dateTime.time().toString(Qt::DefaultLocaleShortDate);
			break;

		case RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME:
			// Date and time.
			str = dateTime.toString(Qt::DefaultLocaleShortDate);
			break;

		default:
			// Invalid combination.
			assert(!"Invalid Date/Time formatting.");
			break;
	}

	if (!str.isEmpty()) {
		lblDateTime->setText(str);
		ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, lblDateTime);
	} else {
		// Invalid date/time.
		delete lblDateTime;
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
	lblAgeRatings->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

	const RomFields::age_ratings_t *age_ratings = field->data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		lblAgeRatings->setText(RomDataView::tr("ERROR"));
		ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, lblAgeRatings);
		return;
	}

	// Convert the age ratings field to a string.
	QString str;
	str.reserve(64);
	for (int i = 0; i < (int)age_ratings->size(); i++) {
		const uint16_t rating = age_ratings->at(i);
		if (!(rating & RomFields::AGEBF_ACTIVE))
			continue;

		if (!str.isEmpty()) {
			// Append a comma.
			str += QLatin1String(", ");
		}

		const char *abbrev = RomFields::ageRatingAbbrev(i);
		if (abbrev) {
			str += QLatin1String(abbrev);
		} else {
			// Invalid age rating.
			// Use the numeric index.
			str += QString::number(i);
		}
		str += QChar(L'=');
		str += RP2Q(utf8_to_rp_string(RomFields::ageRatingDecode(i, rating)));
	}

	if (str.isEmpty()) {
		// No age ratings.
		str = RomDataView::tr("None");
	}

	lblAgeRatings->setText(str);
	ui.tabs[field->tabIdx].formLayout->addRow(lblDesc, lblAgeRatings);
}

/**
 * Initialize the display widgets.
 * If the widgets already exist, they will
 * be deleted and recreated.
 */
void RomDataViewPrivate::initDisplayWidgets(void)
{
	// Clear the tabs.
	for (int i = ui.tabs.size()-1; i >= 0; i--) {
		auto &tab = ui.tabs[i];
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
	ui.tabs.clear();
	delete ui.tabWidget;
	ui.tabWidget = nullptr;

	// Clear the bitfields map.
	mapBitfields.clear();

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
	if (fields->tabCount() > 1) {
		ui.tabs.resize(fields->tabCount());
		ui.tabWidget = new QTabWidget(q);
		for (int i = 0; i < fields->tabCount(); i++) {
			// Create a tab.
			const rp_char *name = fields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}

			auto &tab = ui.tabs[i];
			QWidget *widget = new QWidget(q);

			// Layouts.
			// NOTE: We shouldn't zero out the QVBoxLayout margins here.
			// Otherwise, we end up with no margins.
			tab.vboxLayout = new QVBoxLayout(widget);
			tab.formLayout = new QFormLayout();
			tab.vboxLayout->addLayout(tab.formLayout, 1);

			// Add the tab.
			ui.tabWidget->addTab(widget, (name ? RP2Q(name) : QString()));
		}
		ui.vboxLayout->addWidget(ui.tabWidget, 1);
	} else {
		// No tabs.
		// Don't create a QTabWidget, but simulate a single
		// tab in ui.tabs[] to make it easier to work with.
		ui.tabs.resize(1);
		auto &tab = ui.tabs[0];

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

	// Create the data widgets.
	for (int i = 0; i < count; i++) {
		const RomFields::Field *field = fields->field(i);
		assert(field != nullptr);
		if (!field || !field->isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field->tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)ui.tabs.size());
		if (tabIdx < 0 || tabIdx >= (int)ui.tabs.size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!ui.tabs[tabIdx].formLayout) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		QLabel *lblDesc = new QLabel(q);
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);
		lblDesc->setText(RomDataView::tr("%1:").arg(RP2Q(field->name)));

		switch (field->type) {
			case RomFields::RFT_INVALID:
				// No data here.
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

			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				delete lblDesc;
				break;
		}
	}
}

/**
 * Start the animation timer.
 */
void RomDataViewPrivate::startAnimTimer(void)
{
	if (!iconAnimData || !ui.tmrIconAnim || !ui.lblIcon) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a single-shot timer for the current frame.
	anim_running = true;
	ui.tmrIconAnim->start(delay);
}

/**
 * Stop the animation timer.
 */
void RomDataViewPrivate::stopAnimTimer(void)
{
	if (ui.tmrIconAnim) {
		anim_running = false;
		ui.tmrIconAnim->stop();
	}
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
	d->startAnimTimer();

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
	d->stopAnimTimer();

	// Pass the event to the superclass.
	super::hideEvent(event);
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

	Q_D(RomDataView);
	auto iter = d->mapBitfields.find(sender);
	if (iter != d->mapBitfields.end()) {
		// Check if the status is correct.
		bool value = iter->second;
		if (checked != value) {
			// Toggle this box.
			sender->setChecked(!checked);
		}
	}
}

/**
 * Animated icon timer.
 */
void RomDataView::tmrIconAnim_timeout(void)
{
	Q_D(RomDataView);

	// Next frame.
	int delay = 0;
	int frame = d->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		return;
	}

	if (frame != d->last_frame_number) {
		// New frame number.
		// Update the icon.
		d->ui.lblIcon->setPixmap(d->iconFrames[frame]);
		d->last_frame_number = frame;
	}

	// Set the single-shot timer.
	if (d->anim_running) {
		d->ui.tmrIconAnim->start(delay);
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
 *
 * @return RomData object.
 */
void RomDataView::setRomData(LibRomData::RomData *romData)
{
	Q_D(RomDataView);
	if (d->romData == romData)
		return;

	bool prevAnimTimerRunning = d->anim_running;
	if (prevAnimTimerRunning) {
		// Animation is running.
		// Stop it temporarily and reset the frame number.
		d->stopAnimTimer();
		d->last_frame_number = 0;
	}

	if (d->romData) {
		d->romData->unref();
	}
	d->romData = (romData ? romData->ref() : nullptr);
	d->initDisplayWidgets();

	if (romData != nullptr && prevAnimTimerRunning) {
		// Restart the animation timer.
		d->startAnimTimer();
	}

	emit romDataChanged(romData);
}

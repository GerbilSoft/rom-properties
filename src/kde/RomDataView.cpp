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

#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
using LibRomData::RomData;
using LibRomData::RomFields;
using LibRomData::rp_string;

// C includes. (C++ namespace)
#include <cassert>
#include <cstdio>

// C++ includes.
#include <vector>
using std::vector;

#include <QtCore/QDateTime>

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

			QFormLayout *formLayout;
			// TODO: Store the field widgets?
		};
		Ui ui;
		RomData *romData;

		/**
		 * Update the display widgets.
		 * FIXME: Allow running this multiple times?
		 */
		void updateDisplay(void);

		bool displayInit;
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, RomData *romData)
	: q_ptr(q)
	, romData(romData)
	, displayInit(false)
{ }

RomDataViewPrivate::~RomDataViewPrivate()
{
	delete romData;
}

void RomDataViewPrivate::Ui::setupUi(QWidget *RomDataView)
{
	// Only the formLayout is initialized here.
	// Everything else is initialized in updateDisplay.
	formLayout = new QFormLayout(RomDataView);
}

/**
 * Update the display widgets.
 * FIXME: Allow running this multiple times?
 */
void RomDataViewPrivate::updateDisplay(void)
{
	if (!romData || displayInit)
		return;
	displayInit = true;

	// Get the fields.
	const RomFields *fields = romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Make sure the underlying file handle is closed,
	// since we don't need it anymore.
	romData->close();

	// System name.
	// TODO: Logo, game icon, and game title?
	Q_Q(RomDataView);
	const rp_char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *fileType = nullptr;
	switch (romData->fileType()) {
		case RomData::FTYPE_ROM_IMAGE:
			fileType = _RP("ROM Image");
			break;
		case RomData::FTYPE_DISC_IMAGE:
			fileType = _RP("Disc Image");
			break;
		case RomData::FTYPE_SAVE_FILE:
			fileType = _RP("Save File");
			break;
		case RomData::FTYPE_EMBEDDED_DISC_IMAGE:
			fileType = _RP("Embedded Disc Image");
			break;
		case RomData::FTYPE_APPLICATION_PACKAGE:
			fileType = _RP("Application Package");
			break;
		case RomData::FTYPE_UNKNOWN:
		default:
			fileType = nullptr;
			break;
	}

	QString header;
	if (systemName) {
		header = RP2Q(systemName);
	}
	if (fileType) {
		if (!header.isEmpty()) {
			header += QChar(L' ');
		}
		header += RP2Q(fileType);
	}

	if (!header.isEmpty()) {
		QLabel *lblHeader = new QLabel(q);
		lblHeader->setAlignment(Qt::AlignCenter);
		lblHeader->setTextFormat(Qt::PlainText);
		lblHeader->setText(header);

		// Use a bold font.
		QFont font = lblHeader->font();
		font.setBold(true);
		lblHeader->setFont(font);

		ui.formLayout->addRow(lblHeader);
	}

	// Create the data widgets.
	for (int i = 0; i < count; i++) {
		const RomFields::Desc *desc = fields->desc(i);
		const RomFields::Data *data = fields->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		QLabel *lblDesc = new QLabel(q);
		lblDesc->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		lblDesc->setTextFormat(Qt::PlainText);
		lblDesc->setText(RomDataView::tr("%1:").arg(RP2Q(desc->name)));

		switch (desc->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				delete lblDesc;
				break;

			case RomFields::RFT_STRING: {
				// String type.
				QLabel *lblString = new QLabel(q);
				lblString->setAlignment(Qt::AlignLeft | Qt::AlignTop);
				lblString->setTextFormat(Qt::PlainText);
				lblString->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
				if (data->str) {
					lblString->setText(RP2Q(data->str));
				}

				// Check for any formatting options.
				if (desc->str_desc) {
					// Monospace font?
					if (desc->str_desc->formatting & RomFields::StringDesc::STRF_MONOSPACE) {
						QFont font(QLatin1String("Monospace"));
						font.setStyleHint(QFont::TypeWriter);
						lblString->setFont(font);
					}
				}
\
				ui.formLayout->addRow(lblDesc, lblString);
				break;
			}

			case RomFields::RFT_BITFIELD: {
				// Bitfield type. Create a grid of checkboxes.
				const RomFields::BitfieldDesc *bitfieldDesc = desc->bitfield;
				QGridLayout *gridLayout = new QGridLayout();
				int row = 0, col = 0;
				for (int i = 0; i < bitfieldDesc->elements; i++) {
					const rp_char *name = bitfieldDesc->names[i];
					if (!name)
						continue;
					// TODO: Prevent toggling; disable automatic alt key.
					QCheckBox *checkBox = new QCheckBox(q);
					checkBox->setText(RP2Q(name));
					if (data->bitfield & (1 << i)) {
						checkBox->setChecked(true);
					}
					gridLayout->addWidget(checkBox, row, col, 1, 1);
					col++;
					if (col == bitfieldDesc->elemsPerRow) {
						row++;
						col = 0;
					}
				}
				ui.formLayout->addRow(lblDesc, gridLayout);
				break;
			}

			case RomFields::RFT_LISTDATA: {
				// ListData type. Create a QTreeWidget.
				const RomFields::ListDataDesc *listDataDesc = desc->list_data;
				QTreeWidget *treeWidget = new QTreeWidget(q);
				treeWidget->setRootIsDecorated(false);
				treeWidget->setUniformRowHeights(true);

				// Set up the column names.
				const int count = listDataDesc->count;
				treeWidget->setColumnCount(count);
				QStringList columnNames;
				columnNames.reserve(count);
				for (int i = 0; i < count; i++) {
					if (listDataDesc->names[i]) {
						columnNames.append(RP2Q(listDataDesc->names[i]));
					} else {
						// Don't show this column.
						columnNames.append(QString());
						treeWidget->setColumnHidden(i, true);
					}
				}
				treeWidget->setHeaderLabels(columnNames);

				// Add the row data.
				const RomFields::ListData *listData = data->list_data;
				for (int i = 0; i < (int)listData->data.size(); i++) {
					const vector<rp_string> &data_row = listData->data.at(i);
					QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeWidget);
					int field = 0;
					for (vector<rp_string>::const_iterator iter = data_row.begin();
					     iter != data_row.end(); ++iter, ++field)
					{
						treeWidgetItem->setData(field, Qt::DisplayRole, RP2Q(*iter));
					}
				}

				// Resize the columns to fit the contents.
				for (int i = 0; i < count; i++) {
					treeWidget->resizeColumnToContents(i);
				}
				treeWidget->resizeColumnToContents(count);

				ui.formLayout->addRow(lblDesc, treeWidget);
				break;
			}

			case RomFields::RFT_DATETIME: {
				// Date/Time.
				const RomFields::DateTimeDesc *const dateTimeDesc = desc->date_time;

				QLabel *lblDateTime = new QLabel(q);
				lblDateTime->setTextFormat(Qt::PlainText);
				lblDateTime->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

				QDateTime dateTime;
				dateTime.setTimeSpec(
					(dateTimeDesc->flags & RomFields::RFT_DATETIME_IS_UTC)
						? Qt::UTC : Qt::LocalTime);
				dateTime.setMSecsSinceEpoch(data->date_time * 1000);

				QString str;
				switch (dateTimeDesc->flags &
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
						delete lblDateTime;
						delete lblDesc;
						break;
				}
				lblDateTime->setText(str);

				ui.formLayout->addRow(lblDesc, lblDateTime);
				break;
			}

			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				delete lblDesc;
				break;
		}
	}
}

/** RomDataView **/

RomDataView::RomDataView(RomData *rom, QWidget *parent)
	: super(parent)
	, d_ptr(new RomDataViewPrivate(this, rom))
{
	Q_D(RomDataView);
	d->ui.setupUi(this);

	// Update the display widgets.
	d->updateDisplay();
}

RomDataView::~RomDataView()
{
	delete d_ptr;
}

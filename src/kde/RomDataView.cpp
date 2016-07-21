/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RomDataView.cpp: Sega Mega Drive ROM viewer.                          *
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
#include <cstdio>

#include "libromdata/RomData.hpp"
using LibRomData::RomData;

#include <QLabel>
#include <QCheckBox>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSpacerItem>

class RomDataViewPrivate
{
	public:
		RomDataViewPrivate(RomDataView *q, const RomData *romData);
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
		const RomData *romData;

		/**
		 * Update the display widgets.
		 * FIXME: Allow running this multiple times?
		 */
		void updateDisplay(void);

		bool displayInit;
};

/** RomDataViewPrivate **/

RomDataViewPrivate::RomDataViewPrivate(RomDataView *q, const RomData *romData)
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

// TODO: Move this elsehwere.
static inline QString rpToQS(const LibRomData::rp_string &rps)
{
#if defined(RP_UTF8)
	return QString::fromUtf8(rps.c_str(), (int)rps.size());
#elif defined(RP_UTF16)
	return QString::fromUtf16(reinterpret_cast<const ushort*>(rps.c_str()), (int)rps.size());
#else
#error Text conversion not available on this system.
#endif
}

static inline QString rpToQS(const rp_char *rps)
{
#if defined(RP_UTF8)
	return QString::fromUtf8(rps);
#elif defined(RP_UTF16)
	return QString::fromUtf16(reinterpret_cast<const ushort*>(rps));
#else
#error Text conversion not available on this system.
#endif
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
	Q_Q(RomDataView);
	const int count = romData->count();
	for (int i = 0; i < count; i++) {
		const RomData::RomFieldDesc *desc = romData->desc(i);
		const RomData::RomFieldData *data = romData->data(i);
		if (!desc || !data)
			continue;
		if (desc->type != data->type)
			continue;
		if (!desc->name || desc->name[0] == '\0')
			continue;

		QWidget *dataWidget = nullptr;
		switch (desc->type) {
			case RomData::RFT_STRING: {
				// String type.
				// TODO: Monospace hint?
				QLabel *label = new QLabel(q);
				label->setTextFormat(Qt::PlainText);
				label->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
				if (data->str) {
					label->setText(rpToQS(data->str));
				}
				dataWidget = label;
				break;
			}

			default:
				// Unsupported right now.
				break;
		}

		if (dataWidget) {
			QLabel *lblDesc = new QLabel(q);
			lblDesc->setTextFormat(Qt::PlainText);
			lblDesc->setText(RomDataView::tr("%1:").arg(rpToQS(desc->name)));

			// Add the widgets to the form layout.
			ui.formLayout->addRow(lblDesc, dataWidget);
		}
	}
}

/** RomDataView **/

RomDataView::RomDataView(const RomData *rom, QWidget *parent)
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

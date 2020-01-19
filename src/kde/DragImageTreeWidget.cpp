/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageTreeView.hpp: Drag & Drop tree view.                           *
 *                                                                         *
 * Copyright (c) 2019-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://doc.qt.io/qt-5/dnd.html
// - https://wiki.qt.io/QList_Drag_and_Drop_Example

#include "DragImageTreeWidget.hpp"
#include "RpQt.hpp"
#include "RpQByteArrayFile.hpp"

// librpbase
#include "librpbase/img/RpPngWriter.hpp"
using LibRpBase::RpPngWriter;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// Qt includes.
#include <QDrag>
#include <QMimeData>

void DragImageTreeWidget::startDrag(Qt::DropActions supportedActions)
{
	// TODO: Handle supportedActions?
	// TODO: Multiple PNG images if multiple items are selected?
	// - May need to write images to a temp directory and use a URI list...
	Q_UNUSED(supportedActions)

	// Get the current item.
	QTreeWidgetItem *const item = currentItem();
	if (!item) {
		// No item selected.
		return;
	}

#if 0
	// TODO
	QList<QTreeWidgetItem*> items = selectedItems();
	if (items.isEmpty()) {
		// No items selected.
		return;
	}
#endif

	// Find rp_image* objects in the items.
	QMimeData *const mimeData = new QMimeData;
	bool hasOne = false;
	//foreach (QTreeWidgetItem *item, items) {
	do {
		const rp_image *const img = static_cast<const rp_image*>(item->data(0, RpImageRole).value<void*>());
		if (!img)
			continue;

		// Convert the rp_image to PNG.
		RpQByteArrayFile *const pngData = new RpQByteArrayFile();
		RpPngWriter *const pngWriter = new RpPngWriter(pngData, img);
		if (!pngWriter->isOpen()) {
			// Unable to open the PNG writer.
			pngData->unref();
			continue;
		}

		// TODO: Add text fields indicating the source game.

		int pwRet = pngWriter->write_IHDR();
		if (pwRet != 0) {
			// Error writing the PNG image...
			pngData->unref();
			continue;
		}
		pwRet = pngWriter->write_IDAT();
		if (pwRet != 0) {
			// Error writing the PNG image...
			pngData->unref();
			continue;
		}

		// RpPngWriter will finalize the PNG on delete.
		delete pngWriter;

		// Set the PNG data.
		QByteArray ba = pngData->qByteArray();
		mimeData->setData(QLatin1String("image/png"), pngData->qByteArray());
		pngData->unref();
		hasOne = true;
	} while (0);

	if (!hasOne) {
		// No rp_image* objects...
		delete mimeData;
		return;
	}

	QDrag *const drag = new QDrag(this);
	drag->setMimeData(mimeData);

	// TODO: Ideal icon size?
	// Using 32x32 for now.
	// TODO: Make the icon size accessible to the QTreeWidgetItem.
	// It's currently set in the QTreeWidget in RomDataView.
	QPixmap qpxm;
	const QIcon icon = item->icon(0);
	if (!icon.isNull()) {
		qpxm = icon.pixmap(QSize(32, 32));
	}
	if (!qpxm.isNull()) {
		drag->setPixmap(qpxm);
	}

	drag->exec(Qt::CopyAction);
}

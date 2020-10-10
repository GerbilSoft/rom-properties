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
#include "stdafx.h"
#include "DragImageTreeWidget.hpp"
#include "RpQByteArrayFile.hpp"

// librpbase, librptexture
using LibRpBase::RpPngWriter;
using LibRpTexture::rp_image;

void DragImageTreeWidget::startDrag(Qt::DropActions supportedActions)
{
	// TODO: Handle supportedActions?
	// TODO: Multiple PNG images if multiple items are selected?
	// - May need to write images to a temp directory and use a URI list...
	Q_UNUSED(supportedActions)

	// Get the selected items.
	QList<QTreeWidgetItem*> items = selectedItems();
	if (items.isEmpty()) {
		// No items selected.
		return;
	}

	// TODO: Handle more than one selected item.
	items = items.mid(0, 1);

	// Find rp_image* objects in the items.
	QMimeData *const mimeData = new QMimeData;
	QIcon dragIcon;
	bool hasOne = false;
	const auto iter_end = items.end();
	for (auto iter = items.begin(); iter != iter_end; ++iter) {
		const QTreeWidgetItem *const item = *iter;
		const rp_image *const img = static_cast<const rp_image*>(item->data(0, RpImageRole).value<void*>());
		if (!img)
			continue;

		// Convert the rp_image to PNG.
		RpQByteArrayFile *const pngData = new RpQByteArrayFile();
		RpPngWriter *const pngWriter = new RpPngWriter(pngData, img);
		if (!pngWriter->isOpen()) {
			// Unable to open the PNG writer.
			delete pngWriter;
			pngData->unref();
			continue;
		}

		// TODO: Add text fields indicating the source game.

		int pwRet = pngWriter->write_IHDR();
		if (pwRet != 0) {
			// Error writing the PNG image...
			delete pngWriter;
			pngData->unref();
			continue;
		}
		pwRet = pngWriter->write_IDAT();
		if (pwRet != 0) {
			// Error writing the PNG image...
			delete pngWriter;
			pngData->unref();
			continue;
		}

		// RpPngWriter will finalize the PNG on delete.
		delete pngWriter;

		// Set the PNG data.
		QByteArray ba = pngData->qByteArray();
		mimeData->setData(QLatin1String("image/png"), pngData->qByteArray());
		pngData->unref();

		// Save the icon.
		if (dragIcon.isNull()) {
			dragIcon = item->icon(0);
		}

		hasOne = true;
	}

	if (!hasOne) {
		// No rp_image* objects...
		delete mimeData;
		return;
	}

	QDrag *const drag = new QDrag(this);
	drag->setMimeData(mimeData);

	QPixmap qpxm;
	if (!dragIcon.isNull()) {
		qpxm = dragIcon.pixmap(this->iconSize());
	}
	if (!qpxm.isNull()) {
		drag->setPixmap(qpxm);
	}

	drag->exec(Qt::CopyAction);
}

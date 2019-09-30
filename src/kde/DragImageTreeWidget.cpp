/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * DragImageTreeView.hpp: Drag & Drop tree view.                           *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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
	Q_UNUSED(supportedActions)

	// Check if we have an rp_image* object here.
	QTreeWidgetItem *const item = currentItem();
	if (!item)
		return;

	const rp_image *const img = static_cast<const rp_image*>(item->data(0, RpImageRole).value<void*>());
	if (!img)
		return;

	// Convert the rp_image to PNG.
	RpQByteArrayFile *const pngData = new RpQByteArrayFile();
	RpPngWriter *const pngWriter = new RpPngWriter(pngData, img);
	if (!pngWriter->isOpen()) {
		// Unable to open the PNG writer.
		pngData->unref();
		return;
	}

	// TODO: Add text fields indicating the source game.

	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing the PNG image...
		pngData->unref();
		return;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing the PNG image...
		pngData->unref();
		return;
	}

	// RpPngWriter will finalize the PNG on delete.
	delete pngWriter;

	QDrag *const drag = new QDrag(this);
	QMimeData *const mimeData = new QMimeData;

	QByteArray ba = pngData->qByteArray();
	mimeData->setData(QLatin1String("image/png"), pngData->qByteArray());
	drag->setMimeData(mimeData);

	drag->exec(Qt::CopyAction);
	pngData->unref();
}

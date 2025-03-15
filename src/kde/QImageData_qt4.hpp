/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

// QImageData struct from qimage_p.h. (Qt 4.8.7)
// This is needed in order to trick Qt4's QImage into
// thinking it owns the aligned memory buffer.

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// (...except QImageData hasn't changed from Qt 4.5 all the way to Qt 4.8.7,
//  aside from additional non-virtual member functions. Therefore, it should
//  be usable for all supported Qt4 environments.)

#include <QtCore/qglobal.h>
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

#include <QtGui/QImage>
#include <QtCore/QMap>
#include <QtCore/QString>
class QImageWriter;

struct Q_GUI_EXPORT QImageData {        // internal image data
	QImageData();
	~QImageData();
	static QImageData *create(const QSize &size, QImage::Format format, int numColors = 0);
	static QImageData *create(uchar *data, int w, int h, int bpl, QImage::Format format, bool readOnly);

	QAtomicInt ref;

	int width;
	int height;
	int depth;
	int nbytes;               // number of bytes data
	QVector<QRgb> colortable;
	uchar *data;
#ifdef QT3_SUPPORT
	uchar **jumptable;
#endif
	QImage::Format format;
	int bytes_per_line;
	int ser_no;               // serial number
	int detach_no;

	qreal  dpmx;                // dots per meter X (or 0)
	qreal  dpmy;                // dots per meter Y (or 0)
	QPoint  offset;           // offset in pixels

	uint own_data : 1;
	uint ro_data : 1;
	uint has_alpha_clut : 1;
	uint is_cached : 1;

	bool checkForAlphaPixels() const;

	// Convert the image in-place, minimizing memory reallocation
	// Return false if the conversion cannot be done in-place.
	bool convertInPlace(QImage::Format newFormat, Qt::ImageConversionFlags);

#ifndef QT_NO_IMAGE_TEXT
	QMap<QString, QString> text;
#endif
	bool doImageIO(const QImage *image, QImageWriter* io, int quality) const;

	QPaintEngine *paintEngine;
};

#endif /* QT_VERSION >= QT_VERSION_CHECK(4, 0, 0) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */

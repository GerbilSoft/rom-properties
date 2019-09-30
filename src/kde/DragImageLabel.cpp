/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "DragImageLabel.hpp"
#include "RpQt.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

/**
 * Set the pixmap from the specified rp_image.
 *
 * NOTE: The rp_image pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the rp_image object.
 *
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setPixmapFromRpImage(const rp_image *img)
{
	if (!img) {
		m_img = nullptr;
		this->clear();
		return false;
	}

	// Don't check if the image pointer matches the
	// previously stored image, since the underlying
	// image may have changed.
	m_img = img;
	return updatePixmapFromRpImage();
}

/**
 * Convert a QImage to QPixmap.
 * Automatically resizes the QImage if it's smaller
 * than the minimum size.
 * @param img QImage.
 * @return QPixmap.
 */
QPixmap DragImageLabel::imgToPixmap(const QImage &img) const
{
	if (img.width() >= m_minimumImageSize.width() &&
	    img.height() >= m_minimumImageSize.height())
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
	} while (img_size.width() < m_minimumImageSize.width() &&
		 img_size.height() < m_minimumImageSize.height());

	return QPixmap::fromImage(img.scaled(img_size, Qt::KeepAspectRatio, Qt::FastTransformation));
}

/**
 * Update the pixmap using the rp_image.
 * @return True on success; false on error.
 */
bool DragImageLabel::updatePixmapFromRpImage(void)
{
	if (!m_img || !m_img->isValid()) {
		// No image, or image is not valid.
		return false;
	}

	// Convert the rp_image to a QImage.
	QImage qImg = rpToQImage(m_img);
	if (qImg.isNull()) {
		// Unable to convert the image.
		return false;
	}

	// Image converted successfully.
	this->setPixmap(imgToPixmap(qImg));
	return true;
}

/** Overridden QWidget functions **/

void DragImageLabel::mousePressEvent(QMouseEvent *event)
{
	// TODO
	Q_UNUSED(event)
}

void DragImageLabel::mouseMoveEvent(QMouseEvent *event)
{
	// TODO
	Q_UNUSED(event)
}

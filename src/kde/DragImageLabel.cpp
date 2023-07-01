/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "stdafx.h"
#include "DragImageLabel.hpp"
#include "RpQByteArrayFile.hpp"

// librpbase, librptexture
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"
using LibRpBase::IconAnimData;
using LibRpBase::RpPngWriter;
using LibRpTexture::rp_image;

DragImageLabel::DragImageLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
	: super(text, parent, f)
	, m_minimumImageSize(DIL_MIN_IMAGE_SIZE, DIL_MIN_IMAGE_SIZE)
	, m_img(nullptr)
	, m_anim(nullptr)
{ }

DragImageLabel::DragImageLabel(QWidget *parent, Qt::WindowFlags f)
	: super(parent, f)
	, m_minimumImageSize(DIL_MIN_IMAGE_SIZE, DIL_MIN_IMAGE_SIZE)
	, m_img(nullptr)
	, m_anim(nullptr)
{ }

DragImageLabel::~DragImageLabel()
{
	delete m_anim;
	UNREF(m_img);
}

/**
 * Set the rp_image for this label.
 *
 * NOTE: The rp_image pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the rp_image object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setRpImage(const rp_image *img)
{
	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	UNREF_AND_NULL(m_img);

	if (!img) {
		if (!m_anim || !m_anim->iconAnimData) {
			this->clear();
		} else {
			return updatePixmaps();
		}
		return false;
	}

	m_img = img->ref();
	return updatePixmaps();
}

/**
 * Set the icon animation data for this label.
 *
 * NOTE: The iconAnimData pointer is stored and used if necessary.
 * Make sure to call this function with nullptr before deleting
 * the IconAnimData object.
 *
 * NOTE 2: If animated icon data is specified, that supercedes
 * the individual rp_image.
 *
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setIconAnimData(const IconAnimData *iconAnimData)
{
	if (!m_anim) {
		m_anim = new anim_vars();
	}

	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	UNREF_AND_NULL(m_anim->iconAnimData);

	if (!iconAnimData) {
		if (m_anim->tmrIconAnim) {
			m_anim->tmrIconAnim->stop();
		}
		m_anim->anim_running = false;

		if (!m_img) {
			this->clear();
		} else {
			return updatePixmaps();
		}
		return false;
	}

	m_anim->iconAnimData = iconAnimData->ref();
	return updatePixmaps();
}

/**
 * Clear the rp_image and iconAnimData.
 * This will stop the animation timer if it's running.
 */
void DragImageLabel::clearRp(void)
{
	if (m_anim) {
		if (m_anim->tmrIconAnim) {
			m_anim->tmrIconAnim->stop();
		}
		m_anim->anim_running = false;
		UNREF_AND_NULL(m_anim->iconAnimData);
	}

	UNREF_AND_NULL(m_img);
	this->clear();
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
 * Update the pixmap(s).
 * @return True on success; false on error.
 */
bool DragImageLabel::updatePixmaps(void)
{
	if (m_anim && m_anim->iconAnimData) {
		const IconAnimData *const iconAnimData = m_anim->iconAnimData;

		// Convert the icons to QPixmaps.
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			const rp_image *const frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				// NOTE: Allowing NULL frames here...
				m_anim->iconFrames[i] = imgToPixmap(rpToQImage(frame));
			}
		}

		// Set up the IconAnimHelper.
		m_anim->iconAnimHelper.setIconAnimData(iconAnimData);
		if (m_anim->iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			m_anim->last_frame_number = m_anim->iconAnimHelper.frameNumber();
			// Create the animation timer.
			if (!m_anim->tmrIconAnim) {
				m_anim->tmrIconAnim = new QTimer(this);
				m_anim->tmrIconAnim->setObjectName(QLatin1String("tmrIconAnim"));
				m_anim->tmrIconAnim->setSingleShot(true);
				QObject::connect(m_anim->tmrIconAnim, SIGNAL(timeout()),
						 this, SLOT(tmrIconAnim_timeout()));
			}
		}

		// Show the first frame.
		this->setPixmap(m_anim->iconFrames[m_anim->iconAnimHelper.frameNumber()]);
		return true;
	}

	if (m_img && m_img->isValid()) {
		// Single image.

		// Convert the rp_image to a QImage.
		const QImage qImg = rpToQImage(m_img);
		if (qImg.isNull()) {
			// Unable to convert the image.
			return false;
		}

		// Image converted successfully.
		this->setPixmap(imgToPixmap(qImg));
		return true;
	}

	// No image or animated icon data.
	return false;
}

/**
 * Start the animation timer.
 */
void DragImageLabel::startAnimTimer(void)
{
	if (!m_anim || !m_anim->iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Sanity check: Timer should have been created already.
	assert(m_anim->tmrIconAnim != nullptr);

	// Get the current frame information.
	m_anim->last_frame_number = m_anim->iconAnimHelper.frameNumber();
	const int delay = m_anim->iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a single-shot timer for the current frame.
	m_anim->anim_running = true;
	m_anim->tmrIconAnim->start(delay);
}

/**
 * Stop the animation timer.
 */
void DragImageLabel::stopAnimTimer(void)
{
	if (m_anim && m_anim->tmrIconAnim) {
		m_anim->anim_running = false;
		m_anim->tmrIconAnim->stop();
	}
}

/**
 * Animated icon timer.
 */
void DragImageLabel::tmrIconAnim_timeout(void)
{
	assert(m_anim != nullptr);
	if (!m_anim) {
		// Should not happen...
		return;
	}

	// Next frame.
	int delay = 0;
	const int frame = m_anim->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		return;
	}

	if (frame != m_anim->last_frame_number) {
		// New frame number.
		// Update the icon.
		this->setPixmap(m_anim->iconFrames[frame]);
		m_anim->last_frame_number = frame;
	}

	// Set the single-shot timer.
	if (m_anim->anim_running) {
		m_anim->tmrIconAnim->start(delay);
	}
}

/** Overridden QWidget functions **/

void DragImageLabel::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		m_dragStartPos = event->pos();

	return super::mousePressEvent(event);
}

void DragImageLabel::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton))
		return;
	if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
		return;

	const bool isAnimated = (m_anim && m_anim->iconAnimData && m_anim->iconAnimHelper.isAnimated());

	RpQByteArrayFile *const pngData = new RpQByteArrayFile();
	RpPngWriter *pngWriter;
	if (isAnimated) {
		// Animated icon.
		pngWriter = new RpPngWriter(pngData, m_anim->iconAnimData);
	} else if (m_img) {
		// Standard icon.
		// NOTE: Using the source image because we want the original
		// size, not the resized version.
		pngWriter = new RpPngWriter(pngData, m_img);
	} else {
		// No icon...
		pngData->unref();
		return;
	}

	if (!pngWriter->isOpen()) {
		// Unable to open the PNG writer.
		delete pngWriter;
		pngData->unref();
		return;
	}

	// TODO: Add text fields indicating the source game.

	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing the PNG image...
		delete pngWriter;
		pngData->unref();
		return;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing the PNG image...
		delete pngWriter;
		pngData->unref();
		return;
	}

	// RpPngWriter will finalize the PNG on delete.
	delete pngWriter;

	QMimeData *const mimeData = new QMimeData;
	mimeData->setObjectName(QLatin1String("mimeData"));
	mimeData->setData(QLatin1String("image/png"), pngData->qByteArray());

	QDrag *const drag = new QDrag(this);
	drag->setObjectName(QLatin1String("drag"));
	drag->setMimeData(mimeData);

	// Get the first frame and use it for the drag pixmap.
	if (m_anim && m_anim->iconAnimHelper.isAnimated()) {
		const int frame = m_anim->iconAnimData->seq_index[0];
		if (!m_anim->iconFrames[frame].isNull()) {
			drag->setPixmap(m_anim->iconFrames[frame]);
		}
	} else {
		// Not animated. Use the QLabel pixmap directly.
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		drag->setPixmap(this->pixmap(Qt::ReturnByValue));
#else /* QT_VERSION < QT_VERSION_CHECK(5,15,0) */
		const QPixmap *const qpxm = this->pixmap();
		if (qpxm) {
			drag->setPixmap(*qpxm);
		}
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,15,0) */
	}

	drag->exec(Qt::CopyAction);
	pngData->unref();
}

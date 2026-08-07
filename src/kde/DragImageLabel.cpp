/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageLabel.cpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html
#include "DragImageLabel.hpp"
#include "RpQByteArrayFile.hpp"

// Other rom-properties libraries
#include "librpbase/img/RpPngWriter.hpp"
using namespace LibRpBase;
using namespace LibRpTexture;

// C++ STL classes
using std::shared_ptr;
using std::unique_ptr;

// Qt includes
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QAction>
#include <QDesktopServices>

#include "RpQt.hpp"

DragImageLabel::DragImageLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
	: super(text, parent, f)
	, m_minimumImageSize(DIL_MIN_IMAGE_SIZE, DIL_MIN_IMAGE_SIZE)
	, m_ecksBawks(false)
{}

DragImageLabel::DragImageLabel(QWidget *parent, Qt::WindowFlags f)
	: super(parent, f)
	, m_minimumImageSize(DIL_MIN_IMAGE_SIZE, DIL_MIN_IMAGE_SIZE)
	, m_ecksBawks(false)
{}

void DragImageLabel::setEcksBawks(bool newEcksBawks)
{
	m_ecksBawks = newEcksBawks;
	setContextMenuPolicy(m_ecksBawks ? Qt::ActionsContextMenu : Qt::DefaultContextMenu);
	if (!m_ecksBawks) {
		return;
	}
	if (!actions().isEmpty()) {
		return;
	}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	// Need to initialize Ecks Bawks actions.
	// NOTE: Only supporting Qt 5 for lambda functions.
	QAction *const actMenu1 = new QAction(QLatin1String("ermahgerd! an ecks bawks ISO!"), this);
	connect(actMenu1, &QAction::triggered, this, [](bool) -> void {
		QDesktopServices::openUrl(QUrl(QLatin1String("https://twitter.com/DeaThProj/status/1684469412978458624")));
	});

	QAction *const actMenu2 = new QAction(QLatin1String("Yar, har, fiddle dee dee"), this);
	connect(actMenu2, &QAction::triggered, this, [](bool) -> void {
		QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/xenia-canary/xenia-canary/pull/180")));
	});

	addAction(actMenu1);
	addAction(actMenu2);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
}

/**
 * Set the rp_image for this label.
 * This will replace any previously set rp_image or IconAnimData.
 *
 * @param img rp_image, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setRpImage(const rp_image_const_ptr &img)
{
	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	m_imgData.emplace<non_anim_vars_t>(img);
	if (!img) {
		this->clear();
		return false;
	}
	return updatePixmaps();
}

/**
 * Set the icon animation data for this label.
 * This will replace any previously set rp_image or IconAnimData.
 *
 * @param iconAnimData IconAnimData, or nullptr to clear.
 * @return True on success; false on error or if clearing.
 */
bool DragImageLabel::setIconAnimData(const IconAnimDataConstPtr &iconAnimData)
{
	// NOTE: We're not checking if the image pointer matches the
	// previously stored image, since the underlying image may
	// have changed.
	m_imgData.emplace<anim_vars_t>(iconAnimData);
	if (!iconAnimData) {
		this->clear();
		return false;
	}
	return updatePixmaps();
}

/**
 * Clear the rp_image and/or iconAnimData.
 * This will stop the animation timer if it's running.
 */
void DragImageLabel::clearRp(void)
{
	m_imgData.emplace<std::monostate>();
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
	if (isAnim()) {
		anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);
		const IconAnimDataConstPtr &iconAnimData = anim.iconAnimData;

		assert(iconAnimData->count > 0);
		assert(iconAnimData->count <= IconAnimData::MAX_FRAMES);
		if (iconAnimData->count <= 0 || iconAnimData->count > IconAnimData::MAX_FRAMES) {
			// Icon frame count is out of range...
			anim.iconFrames.clear();
			return false;
		}
		anim.iconFrames.resize(iconAnimData->count);

		// Convert the icons to QPixmaps.
		for (int i = iconAnimData->count-1; i >= 0; i--) {
			const rp_image_ptr &frame = iconAnimData->frames[i];
			if (frame && frame->isValid()) {
				// NOTE: Allowing NULL frames here...
				anim.iconFrames[i] = imgToPixmap(rpToQImage(frame));
			} else {
				anim.iconFrames[i] = QPixmap();
			}
		}

		// Set up the IconAnimHelper.
		IconAnimHelper &iconAnimHelper = anim.iconAnimHelper;
		iconAnimHelper.setIconAnimData(iconAnimData);
		if (iconAnimHelper.isAnimated()) {
			// Initialize the animation.
			anim.last_frame_number = iconAnimHelper.frameNumber();
			// Create the animation timer.
			if (!anim.tmrIconAnim) {
				anim.tmrIconAnim.reset(new QTimer(this));
				anim.tmrIconAnim->setObjectName(QLatin1String("tmrIconAnim"));
				anim.tmrIconAnim->setSingleShot(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
				connect(anim.tmrIconAnim.get(), &QTimer::timeout,
					this, &DragImageLabel::tmrIconAnim_timeout);
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
				connect(anim.tmrIconAnim.get(), SIGNAL(timeout()),
					this, SLOT(tmrIconAnim_timeout()));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */
			}
		}

		// Show the first frame.
		const int frame = anim.iconAnimHelper.frameNumber();
		assert(frame >= 0);
		assert(frame < static_cast<int>(anim.iconFrames.size()));
		if (frame >= 0 && frame < static_cast<int>(anim.iconFrames.size())) {
			this->setPixmap(anim.iconFrames[frame]);
		}
		return true;
	} else if (isNonAnim()) {
		// Single image
		non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(m_imgData);

		// Convert the rp_image to a QImage.
		QImage qImg = rpToQImage(non_anim.img);
		if (qImg.isNull()) {
			// Unable to convert the image.
			return false;
		}

		// Convert the QImage to a QPixmap.
		non_anim.qPixmap = imgToPixmap(qImg);
		if (non_anim.qPixmap.isNull()) {
			// Unable to convert the image.
			return false;
		}

		// Image converted successfully.
		this->setPixmap(non_anim.qPixmap);
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
	if (!isAnim()) {
		// Not an animated icon.
		return;
	}
	anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);

	// Sanity check: Timer should have been created already.
	assert((bool)anim.tmrIconAnim);

	const IconAnimHelper &iconAnimHelper = anim.iconAnimHelper;
	if (!iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	anim.last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a single-shot timer for the current frame.
	anim.anim_running = true;
	anim.tmrIconAnim->start(delay);
}

/**
 * Stop the animation timer.
 */
void DragImageLabel::stopAnimTimer(void)
{
	if (isAnim()) {
		anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);
		if (anim.tmrIconAnim) {
			anim.anim_running = false;
			anim.tmrIconAnim->stop();
		}
	}
}

/**
 * Animated icon timer.
 */
void DragImageLabel::tmrIconAnim_timeout(void)
{
	assert(isAnim());
	if (!isAnim()) {
		// Should not happen...
		return;
	}
	anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);

	// Next frame.
	int delay = 0;
	const int frame = anim.iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0 || frame >= static_cast<int>(anim.iconFrames.size())) {
		// Invalid frame...
		return;
	}

	if (frame != anim.last_frame_number) {
		// New frame number.
		// Update the icon.
		this->setPixmap(anim.iconFrames[frame]);
		anim.last_frame_number = frame;
	}

	// Set the single-shot timer.
	if (anim.anim_running) {
		anim.tmrIconAnim->start(delay);
	}
}

/** Overridden QWidget functions **/

void DragImageLabel::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragStartPos = event->pos();
	}

	return super::mousePressEvent(event);
}

void DragImageLabel::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		return;
	}

	shared_ptr<RpQByteArrayFile> pngData = std::make_shared<RpQByteArrayFile>();
	unique_ptr<RpPngWriter> pngWriter;
	if (isAnim()) {
		// Animated icon
		const anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);
		pngWriter.reset(new RpPngWriter(pngData, anim.iconAnimData));
	} else if (isNonAnim()) {
		// Standard icon
		// NOTE: Using the source image because we want the original
		// size, not the resized version.
		const non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(m_imgData);
		pngWriter.reset(new RpPngWriter(pngData, non_anim.img));
	} else {
		// No icon...
		return;
	}

	if (!pngWriter->isOpen()) {
		// Unable to open the PNG writer.
		return;
	}

	// TODO: Add text fields indicating the source game.

	int pwRet = pngWriter->write_IHDR();
	if (pwRet != 0) {
		// Error writing the PNG image...
		return;
	}
	pwRet = pngWriter->write_IDAT();
	if (pwRet != 0) {
		// Error writing the PNG image...
		return;
	}

	// RpPngWriter will finalize the PNG on delete.
	pngWriter.reset();

	QMimeData *const mimeData = new QMimeData;
	mimeData->setObjectName(QLatin1String("mimeData"));
	mimeData->setData(QLatin1String("image/png"), pngData->qByteArray());
	mimeData->setData(QLatin1String("application/octet-stream"), pngData->qByteArray());

	QDrag *const drag = new QDrag(this);
	drag->setObjectName(QLatin1String("drag"));
	drag->setMimeData(mimeData);

	// Get drag pixmap.
	bool dragPixmapSetFromAnim = false;
	if (isAnim()) {
		const anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);
		if (anim.iconAnimHelper.isAnimated()) {
			// Get the first frame from the animation.
			const int frame = anim.iconAnimData->seq_index[0];
			if (frame >= 0 && frame < static_cast<int>(anim.iconFrames.size()) &&
			    !anim.iconFrames[frame].isNull())
			{
				drag->setPixmap(anim.iconFrames[frame]);
				dragPixmapSetFromAnim = true;
			}
		}
	}

	if (!dragPixmapSetFromAnim) {
		// Not animated. Use the QLabel pixmap directly.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#  if defined(QT_DISABLE_DEPRECATED_BEFORE) && QT_DISABLE_DEPRECATED_BEFORE >= 0x050F00
		drag->setPixmap(this->pixmap());
#  else /* !QT_DISABLE_DEPRECATED_BEFORE || QT_DISABLE_DEPRECATED_BEFORE < 0x050F00 */
		drag->setPixmap(this->pixmap(Qt::ReturnByValue));
#  endif /* QT_DISABLE_DEPRECATED_BEFORE && QT_DISABLE_DEPRECATED_BEFORE >= 0x050F00 */
#else /* QT_VERSION < QT_VERSION_CHECK(5, 15, 0) */
		const QPixmap *const qpxm = this->pixmap();
		if (qpxm) {
			drag->setPixmap(*qpxm);
		}
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) */
	}

	drag->exec(Qt::CopyAction);
}

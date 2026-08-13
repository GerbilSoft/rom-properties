/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * DragImageLabel.hpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html

#pragma once

// Other rom-properties libraries
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/T_img_vars.hpp"
#include "librptexture/img/rp_image.hpp"

// C++ includes
#include <array>
#include <memory>

// Qt includes
#include <QtCore/QTimer>
#include <QLabel>

class DragImageLabel : public QLabel
{
	Q_OBJECT

	Q_PROPERTY(QSize minimumImageSize READ minimumImageSize WRITE setMinimumImageSize)
	Q_PROPERTY(bool ecksBawks READ ecksBawks WRITE setEcksBawks)

// TODO: Adjust minimum image size based on DPI.
#define DIL_MIN_IMAGE_SIZE 32

public:
	explicit DragImageLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	explicit DragImageLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

private:
	typedef QLabel super;
	Q_DISABLE_COPY(DragImageLabel)

public:
	QSize minimumImageSize(void) const
	{
		return m_minimumImageSize;
	}

	void setMinimumImageSize(QSize minimumImageSize)
	{
		if (m_minimumImageSize != minimumImageSize) {
			m_minimumImageSize = minimumImageSize;
			updatePixmaps();
		}
	}

	bool ecksBawks(void) const
	{
		return m_ecksBawks;
	}
	void setEcksBawks(bool newEcksBawks);

	/**
	 * Set the rp_image for this label.
	 * This will replace any previously set rp_image or IconAnimData.
	 *
	 * @param img rp_image, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setRpImage(const LibRpTexture::rp_image_const_ptr &img);

	/**
	 * Set the icon animation data for this label.
	 * This will replace any previously set rp_image or IconAnimData.
	 *
	 * @param iconAnimData IconAnimData, or nullptr to clear.
	 * @return True on success; false on error or if clearing.
	 */
	bool setIconAnimData(const LibRpBase::IconAnimDataConstPtr &iconAnimData);

	/**
	 * Clear the rp_image and/or iconAnimData.
	 * This will stop the animation timer if it's running.
	 */
	void clearRp(void);

protected:
	/**
	 * Convert a QImage to QPixmap.
	 * Automatically resizes the QImage if it's smaller
	 * than the minimum size.
	 * @param img QImage.
	 * @return QPixmap.
	 */
	QPixmap imgToPixmap(const QImage &img) const;

	/**
	 * Update the pixmap(s).
	 * @return True on success; false on error.
	 */
	bool updatePixmaps(void);

public:
	/**
	 * Start the animation timer.
	 */
	void startAnimTimer(void);

	/**
	 * Stop the animation timer.
	 */
	void stopAnimTimer(void);

	/**
	 * Is the animation timer running?
	 * @return True if running; false if not.
	 */
	bool isAnimTimerRunning(void) const
	{
		if (!m_imgData.isAnim()) {
			return false;
		}
		const auto &anim = m_imgData.animVars();
		return anim.tmrIconAnim.isActive();
	}

	/**
	 * Reset the animation frame.
	 * This does NOT update the animation frame.
	 */
	void resetAnimFrame(void)
	{
		if (m_imgData.isAnim()) {
			auto &anim = m_imgData.animVars();
			anim.last_frame_number = 0;
		}
	}

protected slots:
	/**
	 * Animated icon timer.
	 */
	void tmrIconAnim_timeout(void);

protected:
	/** Overridden QWidget functions **/
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

private:
	QSize m_minimumImageSize;
	QPoint m_dragStartPos;
	bool m_ecksBawks;

	LibRpBase::T_img_vars_t<QPixmap, QTimer> m_imgData;
};

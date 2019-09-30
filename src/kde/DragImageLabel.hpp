/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * DragImageLabel.hpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: https://doc.qt.io/qt-5/dnd.html

#ifndef __ROMPROPERTIES_KDE_DRAGIMAGELABEL_HPP__
#define __ROMPROPERTIES_KDE_DRAGIMAGELABEL_HPP__

namespace LibRpTexture {
	class rp_image;
}

#include <QLabel>

class DragImageLabel : public QLabel
{
	Q_OBJECT

	Q_PROPERTY(QSize minimumImageSize READ minimumImageSize WRITE setMinimumImageSize)

	public:
		explicit DragImageLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
			: super(text, parent, f) { }
		explicit DragImageLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
			: super(parent, f) { }

	private:
		typedef QLabel super;
		Q_DISABLE_COPY(DragImageLabel)

	public:
		QSize minimumImageSize(void) const
		{
			return m_minimumImageSize;
		}

		void setMinimumImageSize(const QSize &minimumImageSize)
		{
			if (m_minimumImageSize != minimumImageSize) {
				m_minimumImageSize = minimumImageSize;
				updatePixmapFromRpImage();
			}
		}

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
		bool setPixmapFromRpImage(const LibRpTexture::rp_image *img);

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
		 * Update the pixmap using the rp_image.
		 * @return True on success; false on error.
		 */
		bool updatePixmapFromRpImage();

	protected:
		/** Overridden QWidget functions **/
		void mousePressEvent(QMouseEvent *event) override;
		void mouseMoveEvent(QMouseEvent *event) override;

	private:
		QSize m_minimumImageSize;
		QPoint m_dragStartPos;

		const LibRpTexture::rp_image *m_img;	// NOTE: Not owned by this object.
};

#endif /* __ROMPROPERTIES_KDE_DRAGIMAGELABEL_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQImageBackend.hpp: rp_image_backend using QImage.                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librptexture
#include "librptexture/img/rp_image_backend.hpp"

// Qt
#include <QtCore/QVector>
#include <QtGui/QImage>

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class RpQImageBackend : public LibRpTexture::rp_image_backend
{
public:
	RpQImageBackend(int width, int height, LibRpTexture::rp_image::Format format);

private:
	typedef LibRpTexture::rp_image_backend super;
	Q_DISABLE_COPY(RpQImageBackend)

public:
	/**
	 * Creator function for rp_image::setBackendCreatorFn().
	 */
	static LibRpTexture::rp_image_backend *creator_fn(int width, int height, LibRpTexture::rp_image::Format format);

	// Image data.
	void *data(void) final;
	const void *data(void) const final;
	size_t data_len(void) const final;

	// Image palette.
	uint32_t *palette(void) final;
	const uint32_t *palette(void) const final;
	unsigned int palette_len(void) const final;

public:
	/**
	 * Shrink image dimensions.
	 * @param width New width.
	 * @param height New height.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int shrink(int width, int height) final;

public:
	/**
	 * Get the underlying QImage.
	 *
	 * NOTE: On Qt4, you *must* detach the image if it
	 * will be used after the rp_image is deleted.
	 *
	 * NOTE: Detached QImages may not have the required
	 * row alignment for rp_image functions.
	 *
	 * @return QImage.
	 */
	QImage getQImage(void) const;

protected:
	QImage m_qImage;
	QVector<QRgb> m_qPalette;
};

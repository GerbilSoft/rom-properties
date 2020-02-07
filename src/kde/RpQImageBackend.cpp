/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpQImageBackend.cpp: rp_image_backend using QImage.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpQImageBackend.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(4,0,0) && QT_VERSION < QT_VERSION_CHECK(5,0,0)
# include "QImageData_qt4.hpp"
#elif QT_VERSION < QT_VERSION_CHECK(4,0,0)
# error Unsupported Qt version.
#endif

// C includes. (C++ namespace)
#include <cassert>

// librpbase
#include "librpbase/aligned_malloc.h"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::rp_image_backend;

RpQImageBackend::RpQImageBackend(int width, int height, rp_image::Format format)
	: super(width, height, format)
{
	// Initialize the QImage.
	QImage::Format qfmt;
	switch (format) {
		case rp_image::FORMAT_CI8:
			qfmt = QImage::Format_Indexed8;
			this->stride = ALIGN_BYTES(16, width);
			break;
		case rp_image::FORMAT_ARGB32:
			qfmt = QImage::Format_ARGB32;
			this->stride = ALIGN_BYTES(16, width * sizeof(uint32_t));
			break;
		default:
			assert(!"Unsupported rp_image::Format.");
			this->width = 0;
			this->height = 0;
			this->stride = 0;
			this->format = rp_image::FORMAT_NONE;
			return;
	}

	// Allocate our own memory buffer.
	// This is needed in order to use 16-byte row alignment.
	uint8_t *data = static_cast<uint8_t*>(aligned_malloc(16, height * this->stride));
	if (!data) {
		// Error allocating the memory buffer.
		clear_properties();
		return;
	}

	// NOTE: Qt5 allows us to specify a custom deletion function
	// for the memory buffer. Qt4 does not, so we'll delete the
	// buffer when rp_image is deleted. Note that this may crash
	// if QImages aren't detached before the rp_image is deleted.

	// Create the QImage using the allocated memory buffer.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	m_qImage = QImage(data, width, height, this->stride, qfmt, aligned_free, data);
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	m_qImage = QImage(data, width, height, this->stride, qfmt);
#endif
	if (m_qImage.isNull()) {
		// Error creating the QImage.
		aligned_free(data);
		clear_properties();
		return;
	}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	// Trick QImage into thinking it owns the data buffer.
	// FIXME: NEEDS MASSIVE TESTING.
	QImage::DataPtr d = m_qImage.data_ptr();
	d->own_data = true;
	d->ro_data = false;
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */

	// We're using the full stride for the last row
	// to make it easier to manage. (Qt does this as well.)

	// Make sure we have the correct stride.
	assert(this->stride == m_qImage.bytesPerLine());

	if (format == rp_image::FORMAT_CI8) {
		// Initialize the palette.
		m_qPalette.resize(256);
	}
}

/**
 * Creator function for rp_image::setBackendCreatorFn().
 */
rp_image_backend *RpQImageBackend::creator_fn(int width, int height, rp_image::Format format)
{
	return new RpQImageBackend(width, height, format);
}

void *RpQImageBackend::data(void)
{
	// Return the data from the QImage.
	// Note that this may cause the QImage to
	// detach if it has been retrieved using
	// the getQImage() function.
	return m_qImage.bits();
}

const void *RpQImageBackend::data(void) const
{
	// Return the data from the QImage.
	return m_qImage.bits();
}

size_t RpQImageBackend::data_len(void) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
	return m_qImage.sizeInBytes();
#else /* QT_VERSION < QT_VERSION_CHECK(5,10,0) */
	return m_qImage.byteCount();
#endif
}

uint32_t *RpQImageBackend::palette(void)
{
	// Return the current palette.
	// Note that this may cause the QVector to
	// detach if it has been used by the
	// getQImage() function.
	if (m_qPalette.isEmpty())
		return nullptr;
	return m_qPalette.data();
}

const uint32_t *RpQImageBackend::palette(void) const
{
	if (m_qPalette.isEmpty())
		return nullptr;
	return m_qPalette.constData();
}

int RpQImageBackend::palette_len(void) const
{
	return m_qPalette.size();
}

/**
 * Shrink image dimensions.
 * @param width New width.
 * @param height New height.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpQImageBackend::shrink(int width, int height)
{
	assert(width > 0);
	assert(height > 0);
	assert(this->width > 0);
	assert(this->height > 0);
	assert(width <= this->width);
	assert(height <= this->height);
	if (width <= 0 || height <= 0 ||
	    this->width <= 0 || this->height <= 0 ||
	    width > this->width || height > this->height)
	{
		return -EINVAL;
	}

	// TODO: Is there a way to resize the QImage in place?
	m_qImage = m_qImage.copy(0, 0, width, height);
	return 0;
}

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
QImage RpQImageBackend::getQImage(void) const
{
	if (this->format == rp_image::FORMAT_CI8) {
		// Copy the local color table to the QImage.
		const_cast<RpQImageBackend*>(this)->m_qImage.setColorTable(m_qPalette);
	}

	return m_qImage;
}

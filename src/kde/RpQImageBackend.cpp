/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RpQImageBackend.cpp: rp_image_backend using QImage.                     *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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
#include "librpbase/img/rp_image.hpp"
using LibRpBase::rp_image;
using LibRpBase::rp_image_backend;

RpQImageBackend::RpQImageBackend(int width, int height, rp_image::Format format)
	: super(width, height, format)
{
	// Initialize the QImage.
	QImage::Format qfmt;
	switch (format) {
		case rp_image::FORMAT_CI8:
			qfmt = QImage::Format_Indexed8;
			this->stride = ALIGN(16, width);
			break;
		case rp_image::FORMAT_ARGB32:
			qfmt = QImage::Format_ARGB32;
			this->stride = ALIGN(16, width * sizeof(uint32_t));
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

	// FIXME: Need to test with Qt4 to make sure this doesn't break!
	// TODO: Manually hack into QImageData and set own_data?
	// This will call free(), which is usually fine on POSIX systems
	// when using POSIX aligned memory allocation functions.

	// Create the QImage using the allocated memory buffer.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	m_qImage = QImage(data, width, height, this->stride, qfmt, aligned_free, data);
#else
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
#endif

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
	return m_qImage.byteCount();
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

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

// C includes. (C++ namespace)
#include <cassert>

// librpbase
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
			break;
		case rp_image::FORMAT_ARGB32:
			qfmt = QImage::Format_ARGB32;
			break;
		default:
			assert(!"Unsupported rp_image::Format.");
			this->width = 0;
			this->height = 0;
			this->stride = 0;
			this->format = rp_image::FORMAT_NONE;
			return;
	}

	m_qImage = QImage(width, height, qfmt);
	if (m_qImage.isNull()) {
		// Error creating the QImage.
		clear_properties();
		return;
	}

	// Make sure we have the correct stride.
	// This might be larger than width*pxSize in some cases.
	this->stride = m_qImage.bytesPerLine();

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

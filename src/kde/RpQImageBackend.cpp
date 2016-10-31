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

#include "libromdata/img/rp_image.hpp"
using LibRomData::rp_image;

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
		// Note that QImage doesn't support directly
		// modifying the palette, so we have to copy
		// our palette data every time the underlying
		// QImage is requested.
		this->palette = (uint32_t*)calloc(256, sizeof(*palette));
		if (!this->palette) {
			// Failed to allocate memory.
			clear_properties();
			m_qImage = QImage();
			return;
		}

		// 256 colors allocated in the palette.
		this->palette_len = 256;
	}
}

RpQImageBackend::~RpQImageBackend()
{
	// NOTE: data is pointing to QImage::bits(), so don't free it.
	free(this->palette);
}

/**
 * Creator function for rp_image::setBackendCreatorFn().
 */
LibRomData::rp_image_backend *RpQImageBackend::creator_fn(int width, int height, rp_image::Format format)
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

/**
 * Get the underlying QImage.
 * @return QImage.
 */
QImage RpQImageBackend::getQImage(void) const
{
	if (this->format == rp_image::FORMAT_CI8) {
		// Copy the local color table to the QImage.
		// TODO: Optimize by not initializing?
		QVector<QRgb> colorTable(this->palette_len);
		memcpy(colorTable.data(), this->palette, this->palette_len * sizeof(*palette));
		const_cast<RpQImageBackend*>(this)->m_qImage.setColorTable(colorTable);
	}

	return m_qImage;
}

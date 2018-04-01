/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RpQImageBackend.hpp: rp_image_backend using QImage.                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__
#define __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__

// librpbase
#include "librpbase/img/rp_image_backend.hpp"

// Qt
#include <QtCore/QVector>
#include <QtGui/QImage>

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class RpQImageBackend : public LibRpBase::rp_image_backend
{
	public:
		RpQImageBackend(int width, int height, LibRpBase::rp_image::Format format);

	private:
		typedef LibRpBase::rp_image_backend super;
		Q_DISABLE_COPY(RpQImageBackend)

	public:
		/**
		 * Creator function for rp_image::setBackendCreatorFn().
		 */
		static LibRpBase::rp_image_backend *creator_fn(int width, int height, LibRpBase::rp_image::Format format);

		// Image data.
		void *data(void) final;
		const void *data(void) const final;
		size_t data_len(void) const final;

		// Image palette.
		uint32_t *palette(void) final;
		const uint32_t *palette(void) const final;
		int palette_len(void) const final;

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

#endif /* __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__ */

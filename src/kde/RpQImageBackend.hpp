/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RpQImageBackend.hpp: rp_image_backend using QImage.                     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__
#define __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__

#include "libromdata/img/rp_image_backend.hpp"
#include <QtCore/QVector>
#include <QtGui/QImage>

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class RpQImageBackend : public LibRomData::rp_image_backend
{
	public:
		RpQImageBackend(int width, int height, LibRomData::rp_image::Format format);

	private:
		typedef LibRomData::rp_image_backend super;
		Q_DISABLE_COPY(RpQImageBackend)

	public:
		/**
		 * Creator function for rp_image::setBackendCreatorFn().
		 */
		static LibRomData::rp_image_backend *creator_fn(int width, int height, LibRomData::rp_image::Format format);

		// Image data.
		virtual void *data(void) override final;
		virtual const void *data(void) const override final;
		virtual size_t data_len(void) const override final;

		// Image palette.
		virtual uint32_t *palette(void) override final;
		virtual const uint32_t *palette(void) const override final;
		virtual int palette_len(void) const override final;

	public:
		/**
		 * Get the underlying QImage.
		 * @return QImage.
		 */
		QImage getQImage(void) const;

	protected:
		QImage m_qImage;
		QVector<QRgb> m_qPalette;
};

#endif /* __ROMPROPERTIES_KDE_RPQIMAGEBACKEND_HPP__ */

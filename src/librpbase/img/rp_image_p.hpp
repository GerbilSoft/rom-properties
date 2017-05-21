/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image_p.hpp: Image class. (Private class)                            *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RP_IMAGE_P_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RP_IMAGE_P_HPP__

#include "rp_image.hpp"

namespace LibRpBase {

class rp_image_backend;
class rp_image_private
{
	public:
		/**
		 * Create an rp_image_private.
		 *
		 * If an rp_image_backend has been registered, that backend
		 * will be used; otherwise, the defaul tbackend will be used.
		 *
		 * @param width Image width.
		 * @param height Image height.
		 * @param format Image format.
		 */
		rp_image_private(int width, int height, rp_image::Format format);

		/**
		 * Create an rp_image_private using the specified rp_image_backend.
		 *
		 * NOTE: This rp_image will take ownership of the rp_image_backend.
		 *
		 * @param backend rp_image_backend.
		 */
		explicit rp_image_private(rp_image_backend *backend);

		~rp_image_private();

	private:
		rp_image_private(const rp_image_private &other);
		rp_image_private &operator=(const rp_image_private &other);

	public:
		static rp_image::rp_image_backend_creator_fn backend_fn;

	public:
		// Image backend.
		rp_image_backend *backend;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RP_IMAGE_P_HPP__ */

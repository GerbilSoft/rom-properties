/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * un-premultiply.hpp: Un-premultiply function.                            *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_UN_PREMULTIPLY_HPP__
#define __ROMPROPERTIES_LIBRPBASE_UN_PREMULTIPLY_HPP__

namespace LibRpBase {

class rp_image;

/**
 * Un-premultiply an ARGB32 rp_image.
 * @param img	[in] rp_image to un-premultiply.
 * @return 0 on success; non-zero on error.
 */
int un_premultiply_image(rp_image *img);

}

#endif /* __ROMPROPERTIES_LIBRPBASE_UN_PREMULTIPLY_HPP__ */

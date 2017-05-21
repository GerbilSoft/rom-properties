/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * bmp.hpp: BMP output for rp_image.                                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by Egor.                                        *
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

#ifndef __ROMPROPERTIES_RPCLI_BMP_HPP__
#define __ROMPROPERTIES_RPCLI_BMP_HPP__

#include "librpbase/config.librpbase.h"
namespace LibRpBase {
	class rp_image;
}

#include <ostream>

int rpbmp(std::ostream& os, const LibRpBase::rp_image *img);
int rpbmp(const rp_char *filename, const LibRpBase::rp_image *img);

#endif /* __ROMPROPERTIES_RPCLI_BMP_HPP__ */

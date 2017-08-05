/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__

#include "librpbase/common.h"
#include <stdint.h>

namespace LibRomData {

class SuperMagicDrive
{
	private:
		// Static class.
		SuperMagicDrive();
		~SuperMagicDrive();
		RP_DISABLE_COPY(SuperMagicDrive)

	public:
		/**
		 * Decode a Super Magic Drive interleaved block.
		 * Standard version using regular C++ code.
		 * @param dest	[out] Destination block. (Must be 16 KB.)
		 * @param src	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_cpp(uint8_t *RESTRICT dest, const uint8_t *RESTRICT src);

	public:
		// SMD block size.
		static const unsigned int SMD_BLOCK_SIZE = 16384;

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * @param dest	[out] Destination block. (Must be 16 KB.)
		 * @param src	[in] Source block. (Must be 16 KB.)
		 */
		static inline void decodeBlock(uint8_t *RESTRICT dest, const uint8_t *RESTRICT src);
};

/**
 * Decode a Super Magic Drive interleaved block.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
inline void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT dest, const uint8_t *RESTRICT src)
{
	// TODO: Add an SSE2-optimized version.
	decodeBlock_cpp(dest, src);
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__ */

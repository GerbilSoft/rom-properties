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
		/** Internal algorithms. **/
		// NOTE: These are public to allow for unit tests and benchmarking.

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * Standard version using regular C++ code.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_cpp(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);

#if defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
		/**
		 * Decode a Super Magic Drive interleaved block.
		 * SSE2-optimized version.
		 * NOTE: Pointers must be 16-byte aligned.
		 * @param dest	[out] Destination block. (Must be 16 KB.)
		 * @param src	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_sse2(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif

	public:
		// SMD block size.
		static const unsigned int SMD_BLOCK_SIZE = 16384;

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * NOTE: Pointers must be 16-byte aligned if using SSE2.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static inline void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
};

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
inline void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// TODO:
	// - Common cpuid macro that works for gcc and msvc.
	// - Use pthread_once() + cpuid to check if the system supports SSE2.
#if defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
	decodeBlock_sse2(pDest, pSrc);
#else
	decodeBlock_cpp(pDest, pSrc);
#endif
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__ */

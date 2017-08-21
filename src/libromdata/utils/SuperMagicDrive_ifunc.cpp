/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SuperMagicDrive_ifunc.cpp: SuperMagicDrive IFUNC resolution functions.  *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"
#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "SuperMagicDrive.hpp"
using LibRomData::SuperMagicDrive;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

#ifndef SMD_ALWAYS_HAS_SSE2
/**
 * IFUNC resolver function for decodeBlock().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t decodeBlock_resolve(void)
{
#ifdef SMD_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return (RP_IFUNC_ptr_t)&SuperMagicDrive::decodeBlock_sse2;
	} else
#endif /* SMD_HAS_SSE2 */
#ifdef SMD_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return (RP_IFUNC_ptr_t)&SuperMagicDrive::decodeBlock_mmx;
	} else
#endif /* SMD_HAS_MMX */
	{
		return (RP_IFUNC_ptr_t)&SuperMagicDrive::decodeBlock_cpp;
	}
}
#endif /* SMD_ALWAYS_HAS_SSE2 */

}

#ifndef SMD_ALWAYS_HAS_SSE2
void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
	IFUNC_ATTR(decodeBlock_resolve);
#endif /* SMD_ALWAYS_HAS_SSE2 */

#endif /* RP_HAS_IFUNC */

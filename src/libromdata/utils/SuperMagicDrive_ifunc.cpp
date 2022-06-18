/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * SuperMagicDrive_ifunc.cpp: SuperMagicDrive IFUNC resolution functions.  *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "config.librpcpu.h"

#ifdef HAVE_IFUNC

#include "SuperMagicDrive.hpp"
using namespace LibRomData::SuperMagicDrive;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

#ifndef SMD_ALWAYS_HAS_SSE2
/**
 * IFUNC resolver function for decodeBlock().
 * @return Function pointer.
 */
static __typeof__(&SuperMagicDrive::decodeBlock_cpp) decodeBlock_resolve(void)
{
#ifdef SMD_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &SuperMagicDrive::decodeBlock_sse2;
	} else
#endif /* SMD_HAS_SSE2 */
#ifdef SMD_HAS_MMX
	if (RP_CPU_HasMMX()) {
		return &SuperMagicDrive::decodeBlock_mmx;
	} else
#endif /* SMD_HAS_MMX */
	{
		return &SuperMagicDrive::decodeBlock_cpp;
	}
}
#endif /* SMD_ALWAYS_HAS_SSE2 */

}

#ifndef SMD_ALWAYS_HAS_SSE2
void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
	IFUNC_ATTR(decodeBlock_resolve);
#endif /* SMD_ALWAYS_HAS_SSE2 */

#endif /* HAVE_IFUNC */

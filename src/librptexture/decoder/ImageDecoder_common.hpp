/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_common.hpp: Common image decoder definitions.              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librptexture.h"

#include "common.h"
#include "librpcpuid/cpu_dispatch.h"

// C includes (C++ namespace)
#include <cstdint>

// C++ includes
#include <memory>

// Common definitions, including function attributes.
#include "common.h"

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "librpcpuid/cpuflags_x86.h"
#  define IMAGEDECODER_HAS_SSE2 1
#  define IMAGEDECODER_HAS_SSSE3 1
#endif
#ifdef RP_CPU_AMD64
#  define IMAGEDECODER_ALWAYS_HAS_SSE2 1
#endif

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpTexture { namespace ImageDecoder {

// Pixel formats.
enum class PixelFormat : uint8_t {
	Unknown = 0,

	// 16-bit
	RGB565,		// xRRRRRGG GGGBBBBB
	BGR565,		// xBBBBBGG GGGRRRRR
	ARGB1555,	// ARRRRRGG GGGBBBBB
	ABGR1555,	// ABBBBBGG GGGRRRRR
	RGBA5551,	// RRRRRGGG GGBBBBBA
	BGRA5551,	// BBBBBGGG GGRRRRRA
	ARGB4444,	// AAAARRRR GGGGBBBB
	ABGR4444,	// AAAABBBB GGGGRRRR
	RGBA4444,	// RRRRGGGG BBBBAAAA
	BGRA4444,	// BBBBGGGG RRRRAAAA
	xRGB4444,	// xxxxRRRR GGGGBBBB
	xBGR4444,	// xxxxBBBB GGGGRRRR
	RGBx4444,	// RRRRGGGG BBBBxxxx
	BGRx4444,	// BBBBGGGG RRRRxxxx

	// Uncommon 16-bit formats.
	ARGB8332,	// AAAAAAAA RRRGGGBB

	// GameCube-specific 16-bit
	RGB5A3,		// High bit determines RGB555 or ARGB4444.
	IA8,		// Intensity/Alpha.

	// PlayStation 2-specific 16-bit
	BGR5A3,		// Like RGB5A3, but with
			// swapped R and B channels.

	// 15-bit
	RGB555,
	BGR555,
	BGR555_PS1,	// Special transparency handling.

	// 24-bit
	RGB888,
	BGR888,

	// 32-bit with alpha channel.
	ARGB8888,
	ABGR8888,
	RGBA8888,
	BGRA8888,
	// 32-bit with unused alpha channel.
	xRGB8888,
	xBGR8888,
	RGBx8888,
	BGRx8888,

	// PlayStation 2-specific 32-bit
	BGR888_ABGR7888,	// Why is this a thing.
				// If the high bit is set, it's BGR888.
				// Otherwise, it's ABGR7888.

	// Uncommon 32-bit formats.
	G16R16,
	A2R10G10B10,
	A2B10G10R10,
	RGB9_E5,

	// Uncommon 16-bit formats.
	RG88,
	GR88,

	// VTFEdit uses this as "ARGB8888".
	// TODO: Might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
	RABG8888,

	// Luminance
	L8,		// LLLLLLLL
	A4L4,		// AAAAllll
	L16,		// LLLLLLLL llllllll
	A8L8,		// AAAAAAAA LLLLLLLL
	L8A8,		// LLLLLLLL AAAAAAAA

	// Alpha
	A8,		// AAAAAAAA

	// Other
	R8,		// RRRRRRRR
	RGB332,		// RRRGGGBB

	// Endian-specific ARGB32 definitions.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	Host_ARGB32 = ARGB8888,
	Host_RGBA32 = RGBA8888,
	Host_xRGB32 = xRGB8888,
	Host_RGBx32 = RGBx8888,
	Swap_ARGB32 = BGRA8888,
	Swap_RGBA32 = ABGR8888,
	Swap_xRGB32 = BGRx8888,
	Swap_RGBx32 = xBGR8888,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	Host_ARGB32 = BGRA8888,
	Host_RGBA32 = ABGR8888,
	Host_xRGB32 = BGRx8888,
	Host_RGBx32 = xBGR8888,
	Swap_ARGB32 = ARGB8888,
	Swap_RGBA32 = RGBA8888,
	Swap_xRGB32 = xRGB8888,
	Swap_RGBx32 = RGBx8888,
#endif
};

} }

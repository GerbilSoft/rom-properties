/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PixelConversion.hpp: Pixel conversion inline functions.                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// argb32_t
#include "argb32_t.hpp"

// C includes
#include <stdint.h>

// C++ includes
#include <array>

namespace LibRpTexture { namespace PixelConversion {

/** Color conversion functions. **/
// NOTE: px16 and px32 are always in host-endian.

// FIXME: MSVC 2015 doesn't like functions being marked as constexpr
// if any variables are declared in them.

/** Lookup tables **/

// 2-bit alpha lookup table
static constexpr std::array<uint32_t, 4> a2_lookup = {{
	0x00000000, 0x55000000,
	0xAA000000, 0xFF000000
}};

// 3-bit alpha lookup table
static constexpr std::array<uint32_t, 8> a3_lookup = {{
	0x00000000, 0x24000000,
	0x49000000, 0x6D000000,
	0x92000000, 0xB6000000,
	0xDB000000, 0xFF000000
}};

// 2-bit color lookup table
static constexpr std::array<uint8_t, 4> c2_lookup = {{
	0x00, 0x55, 0xAA, 0xFF
}};

// 3-bit color lookup table
static constexpr std::array<uint8_t, 8> c3_lookup = {{
	0x00, 0x24, 0x49, 0x6D,
	0x92, 0xB6, 0xDB, 0xFF
}};

/** 16-bit RGB **/

/**
 * Convert an RGB565 pixel to ARGB32.
 * @param px16 RGB565 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB565_to_ARGB32(uint16_t px16)
{
	// RGB565: RRRRRGGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 << 8) & 0xF80000) |	// Red
		((px16 << 3) & 0x0000F8);	// Blue
	px32 |=  (px32 >> 5) & 0x070007;	// Expand from 5-bit to 8-bit
	// Green
	px32 |= (((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300));
	return px32;
}

/**
 * Convert a BGR565 pixel to ARGB32.
 * @param px16 BGR565 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t BGR565_to_ARGB32(uint16_t px16)
{
	// RGB565: BBBBBGGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 << 19) & 0xF80000) |	// Red
		((px16 >>  8) & 0x0000F8);	// Blue
	px32 |=  (px32 >>  5) & 0x070007;	// Expand from 5-bit to 8-bit
	// Green
	px32 |= (((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300));
	return px32;
}

/**
 * Convert an ARGB1555 pixel to ARGB32.
 * @param px16 ARGB1555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t ARGB1555_to_ARGB32(uint16_t px16)
{
	// ARGB1555: ARRRRRGG GGGBBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = (((px16 << 9) & 0xF80000)) |	// Red
	                (((px16 << 6) & 0x00F800)) |	// Green
	                (((px16 << 3) & 0x0000F8));	// Blue
	px32 |= ((px32 >> 5) & 0x070707);		// Expand from 5-bit to 8-bit
	// Alpha channel.
	if (px16 & 0x8000) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an ABGR1555 pixel to ARGB32.
 * @param px16 ABGR1555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t ABGR1555_to_ARGB32(uint16_t px16)
{
	// ABGR1555: ABBBBBGG GGGRRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = (((px16 << 19) & 0xF80000)) |	// Red
	                (((px16 <<  6) & 0x00F800)) |	// Green
	                (((px16 >>  7) & 0x0000F8));	// Blue
	px32 |= ((px32 >>  5) & 0x070707);		// Expand from 5-bit to 8-bit
	// Alpha channel.
	if (px16 & 0x8000) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an RGBA5551 pixel to ARGB32.
 * @param px16 RGBA5551 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGBA5551_to_ARGB32(uint16_t px16)
{
	// RGBA5551: RRRRRGGG GGBBBBBA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = (((px16 << 8) & 0xF80000)) |	// Red
	                (((px16 << 5) & 0x00F800)) |	// Green
	                (((px16 << 2) & 0x0000F8));	// Blue
	px32 |= ((px32 >> 5) & 0x070707);		// Expand from 5-bit to 8-bit
	// Alpha channel.
	if (px16 & 0x0001) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert a BGRA5551 pixel to ARGB32.
 * @param px16 BGRA5551 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t BGRA5551_to_ARGB32(uint16_t px16)
{
	// BGRA5551: BBBBBGGG GGRRRRRA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = (((px16 << 18) & 0xF80000)) |	// Red
	                (((px16 <<  5) & 0x00F800)) |	// Green
	                (((px16 >>  8) & 0x0000F8));	// Blue
	px32 |= ((px32 >>  5) & 0x070707);		// Expand from 5-bit to 8-bit
	// Alpha channel.
	if (px16 & 0x0001) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an ARGB4444 pixel to ARGB32.
 * @param px16 ARGB4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t ARGB4444_to_ARGB32(uint16_t px16)
{
	// ARGB4444: AAAARRRR GGGGBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 =  (px16 & 0x000F) |		// B
	                ((px16 & 0x00F0) <<  4) |	// G
	                ((px16 & 0x0F00) <<  8) |	// R
	                ((px16 & 0xF000) << 12);	// A
	px32 |= (px32 << 4);				// Copy to the top nybble.
	return px32;
}

/**
 * Convert an ABGR4444 pixel to ARGB32.
 * @param px16 ABGR4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t ABGR4444_to_ARGB32(uint16_t px16)
{
	// ARGB4444: AAAABBBB GGGGRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = ((px16 & 0x000F) << 16) |	// R
	                ((px16 & 0x00F0) <<  4) |	// G
	                ((px16 & 0x0F00) >>  8) |	// B
	                ((px16 & 0xF000) << 12);	// A
	px32 |= (px32 << 4);				// Copy to the top nybble.
	return px32;
}

/**
 * Convert an RGBA4444 pixel to ARGB32.
 * @param px16 RGBA4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGBA4444_to_ARGB32(uint16_t px16)
{
	// RGBA4444: RRRRGGGG BBBBAAAA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = ((px16 & 0x000F) << 24) |	// A
	                ((px16 & 0x00F0) >>  4) |	// B
	                 (px16 & 0x0F00)        |	// G
	                ((px16 & 0xF000) << 4);		// R
	px32 |=  (px32 << 4);				// Copy to the top nybble.
	return px32;
}

/**
 * Convert a BGRA4444 pixel to ARGB32.
 * @param px16 BGRA4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t BGRA4444_to_ARGB32(uint16_t px16)
{
	// RGBA4444: BBBBGGGG RRRRAAAA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = ((px16 & 0x000F) << 24) |	// A
	                ((px16 & 0x00F0) << 12) |	// R
	                 (px16 & 0x0F00)        |	// G
	                ((px16 & 0xF000) >> 12);	// B
	px32 |=  (px32 << 4);				// Copy to the top nybble.
	return px32;
}

/**
 * Convert an xRGB4444 pixel to ARGB32.
 * @param px16 xRGB4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t xRGB4444_to_ARGB32(uint16_t px16)
{
	// xRGB4444: xxxxRRRR GGGGBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |=  (px16 & 0x000F);		// B
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) << 8);		// R
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an xBGR4444 pixel to ARGB32.
 * @param px16 xBGR4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t xBGR4444_to_ARGB32(uint16_t px16)
{
	// xRGB4444: xxxxBBBB GGGGRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x000F) << 16);	// R
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) >> 8);		// B
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an RGBx4444 pixel to ARGB32.
 * @param px16 RGBx4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGBx4444_to_ARGB32(uint16_t px16)
{
	// RGBx4444: RRRRGGGG BBBBxxxx
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x00F0) >> 4);		// B
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) << 4);		// R
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert a BGRx4444 pixel to ARGB32.
 * @param px16 BGRx4444 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t BGRx4444_to_ARGB32(uint16_t px16)
{
	// RGBx4444: BBBBGGGG RRRRxxxx
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x00F0) << 12);	// R
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) >> 12);	// B
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an ARGB8332 pixel to ARGB32.
 * @param px16 ARGB8332 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t ARGB8332_to_ARGB32(uint16_t px16)
{
	// ARGB8332: AAAAAAAA RRRGGGBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return (c3_lookup[(px16 >> 5) & 7] << 16) |	// Red
	       (c3_lookup[(px16 >> 2) & 7] <<  8) |	// Green
	       (c2_lookup[ px16       & 3]      ) |	// Blue
	       ((px16 << 16) & 0xFF000000);		// Alpha
}

/**
 * Convert an RG88 pixel to ARGB32.
 * @param px16 RG88 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t RG88_to_ARGB32(uint16_t px16)
{
	// RG88:     RRRRRRRR GGGGGGGG
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000 | (static_cast<uint32_t>(px16) << 8);
}

/**
 * Convert a GR88 pixel to ARGB32.
 * @param px16 GR88 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t GR88_to_ARGB32(uint16_t px16)
{
	// GR88:     GGGGGGGG RRRRRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	// FIXME: __swab16() is not constexpr on MSVC 2022.
	return 0xFF000000 | (static_cast<uint32_t>(__swab16(px16)) << 8);
}

/** GameCube-specific 16-bit RGB **/

/**
 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
 * @param px16 RGB5A3 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB5A3_to_ARGB32(uint16_t px16)
{
	// px16 high bit: if set, no alpha channel
	uint32_t px32 = (px16 & 0x8000U)
		? 0xFF000000U
		: 0;

	if (px16 & 0x8000) {
		// RGB555: xRRRRRGG GGGBBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32 |= ((px16 << 3) & 0x0000F8);	// Blue
		px32 |= ((px16 << 6) & 0x00F800);	// Green
		px32 |= ((px16 << 9) & 0xF80000);	// Red
		px32 |= ((px32 >> 5) & 0x070707);	// Expand from 5-bit to 8-bit
	} else {
		// RGB4A3: xAAARRRR GGGGBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  =  (px16 & 0x000F);	// Blue
		px32 |= ((px16 & 0x00F0) << 4);	// Green
		px32 |= ((px16 & 0x0F00) << 8);	// Red
		px32 |= (px32 << 4);		// Copy to the top nybble.

		// Calculate and apply the alpha channel.
		px32 |= a3_lookup[((px16 >> 12) & 0x07)];
	}

	return px32;
}

/**
 * Convert an IA8 pixel to ARGB32. (GameCube/Wii)
 * NOTE: Uses a grayscale palette.
 * @param px16 IA8 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t IA8_to_ARGB32(uint16_t px16)
{
	// FIXME: What's the component order of IA8?
	// Assuming I=MSB, A=LSB...
	// NOTE: This is the same as L8A8_to_ARGB32().

	// IA8:    IIIIIIII AAAAAAAA
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t i = (px16 & 0xFF00);
	i |= ((i << 8) | (i >> 8));
	i |= ((px16 & 0x00FF) << 24);
	return i;
}

/** Nintendo 3DS-specific 16-bit RGB **/

/**
 * Convert an RGB565+A4 pixel to ARGB32.
 * @param px16 RGB565 pixel.
 * @param a4 A4 value.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB565_A4_to_ARGB32(uint16_t px16, uint8_t a4)
{
	// RGB565: RRRRRGGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	a4 &= 0x0F;
	uint32_t px32 = (a4 << 24) | (a4 << 28);	// Alpha
	px32 |= ((px16 << 8) & 0xF80000) |	// Red
		((px16 << 3) & 0x0000F8);	// Blue
	px32 |=  (px32 >> 5) & 0x070007;	// Expand from 5-bit to 8-bit
	// Green
	px32 |= (((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300));
	return px32;
}

/** PlayStation 2-specific 16-bit RGB **/

/**
 * Convert a BGR5A3 pixel to ARGB32. (PlayStation 2)
 * Similar to GameCube RGB5A3, but the R and B channels are swapped.
 * @param px16 BGR5A3 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
static inline uint32_t BGR5A3_to_ARGB32(uint16_t px16)
{
	// px16 high bit: if set, no alpha channel
	uint32_t px32 = (px16 & 0x8000U)
		? 0xFF000000U
		: 0;

	if (px16 & 0x8000) {
		// BGR555: xBBBBBGG GGGRRRRR
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  = 0xFF000000U;	// no alpha channel
		px32 |= ((px16 >>  7) & 0x0000F8);	// Blue
		px32 |= ((px16 <<  6) & 0x00F800);	// Green
		px32 |= ((px16 << 19) & 0xF80000);	// Red
		px32 |= ((px32 >>  5) & 0x070707);	// Expand from 5-bit to 8-bit
	} else {
		// BGR4A3: xAAABBBB GGGGRRRR
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  = ((px16 & 0x0F00) >>  8);	// Blue
		px32 |= ((px16 & 0x00F0) <<  4);	// Green
		px32 |= ((px16 & 0x000F) << 16);	// Red
		px32 |= (px32 << 4);		// Copy to the top nybble.

		// Calculate and apply the alpha channel.
		px32 |= a3_lookup[((px16 >> 12) & 0x07)];
	}

	return px32;
}

/** 15-bit RGB **/

/**
 * Convert an RGB555 pixel to ARGB32.
 * @param px16 RGB555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB555_to_ARGB32(uint16_t px16)
{
	// RGB555: xRRRRRGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 << 9) & 0xF80000) |	// Red
		((px16 << 6) & 0x00F800) |	// Green
		((px16 << 3) & 0x0000F8);	// Blue
	px32 |= ((px32 >> 5) & 0x070707);	// Expand from 5-bit to 8-bit
	return px32;
}

/**
 * Convert a BGR555 pixel to ARGB32.
 * @param px16 BGR555 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t BGR555_to_ARGB32(uint16_t px16)
{
	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 << 19) & 0xF80000) |	// Red
		((px16 <<  6) & 0x00F800) |	// Green
		((px16 >>  7) & 0x0000F8);	// Blue
	px32 |= ((px32 >>  5) & 0x070707);	// Expand from 5-bit to 8-bit
	return px32;
}

/** 32-bit RGB **/

/**
 * Convert a G16R16 pixel to ARGB32.
 * @param px32 G16R16 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t G16R16_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// G16R16: GGGGGGGG gggggggg RRRRRRRR rrrrrrrr
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000U |
		((px32 <<  8) & 0x00FF0000) |
		((px32 >> 16) & 0x0000FF00);
}

/**
 * Convert an A2R10G10B10 pixel to ARGB32.
 * @param px32 A2R10G10B10 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t A2R10G10B10_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// A2R10G10B10: AARRRRRR RRrrGGGG GGGGggBB BBBBBBbb
	//      ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return ((px32 >> 6) & 0xFF0000) |	// Red
	       ((px32 >> 4) & 0x00FF00) |	// Green
	       ((px32 >> 2) & 0x0000FF) |	// Blue
	       a2_lookup[px32 >> 30];		// Alpha
}

/**
 * Convert an A2B10G10R10 pixel to ARGB32.
 * @param px32 A2B10G10R10 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t A2B10G10R10_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// A2B10G10R10: AABBBBBB BBbbGGGG GGGGggRR RRRRRRrr
	//      ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return ((px32 << 14) & 0xFF0000) |	// Red
	       ((px32 >>  4) & 0x00FF00) |	// Green
	       ((px32 >> 22) & 0x0000FF) |	// Blue
	       a2_lookup[px32 >> 30];		// Alpha
}

/**
 * Convert an RGB9_E5 pixel to ARGB32.
 * NOTE: RGB9_E5 is an HDR format; this converts to LDR.
 * @param px32 A2B10G10R10 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t RGB9_E5_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?
	// FIXME: constexpr isn't working with gcc-13.2.1 due to the union and argb32_t types.

	// References:
	// - https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_shared_exponent.txt
	// - https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/util/format_rgb9e5.h
	// RGB9_E5: EEEEEBBB BBBBBBGG GGGGGGGR RRRRRRRR
	//  ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
#define RGB9E5_EXP_BIAS 15
#define RGB9E5_MANTISSA_BITS 9
	union { float f; uint32_t u; } scale;

	const int e = static_cast<int>(px32 >> 27) - RGB9E5_EXP_BIAS - RGB9E5_MANTISSA_BITS;
	scale.u = (e + 127) << 23;

	// Process as float.
	const float rf = (float) (px32        & 0x1FF) * scale.f;
	const float gf = (float)((px32 >>  9) & 0x1FF) * scale.f;
	const float bf = (float)((px32 >> 18) & 0x1FF) * scale.f;

	// Convert to ARGB32, clamping to [0,255].
	argb32_t pxr;
	pxr.a = 0xFF;
	pxr.r = (rf <= 0.0f ? 0 : (rf >= 1.0f ? 255 : ((uint8_t)(rf * 256.0f))));
	pxr.g = (gf <= 0.0f ? 0 : (gf >= 1.0f ? 255 : ((uint8_t)(gf * 256.0f))));
	pxr.b = (bf <= 0.0f ? 0 : (bf >= 1.0f ? 255 : ((uint8_t)(bf * 256.0f))));
	return pxr.u32;
}

/** PlayStation 2-specific 32-bit RGB **/

/**
 * Convert a BGR888_ABGR7888 pixel to ARGB32. (PlayStation 2)
 * Similar to GameCube RGB5A3, but with 32-bit channels.
 * (Why would you do this... Just set alpha to 0xFF!)
 * @param px16 BGR888_ABGR7888 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
static inline uint32_t BGR888_ABGR7888_to_ARGB32(uint32_t px32)
{
	// px32 high bit: if set, no alpha channel
	uint32_t argb = (px32 & 0x80000000U)
		? 0xFF000000U
		: 0;

	if (px32 & 0x80000000U) {
		// BGR888: xxxxxxxx BBBBBBBB GGGGGGGG RRRRRRRR
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		argb |= (px32 >> 16) & 0xFF;	// Blue
		argb |= (px32 & 0x0000FF00U);	// Green
		argb |= (px32 & 0xFF) << 16;	// Red
	} else {
		// ABGR7888: xAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
		//   ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		argb  = (px32 & 0x7F000000U) << 1;	// Alpha
		argb |= (argb & 0x80000000U) >> 7;	// Alpha LSB
		argb |= (px32 >> 16) & 0xFF;		// Blue
		argb |= (px32 & 0x0000FF00U);		// Green
		argb |= (px32 & 0xFF) << 16;		// Red
	}

	return argb;
}

/** Luminance **/

/**
 * Convert an L8 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px8 L8 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t L8_to_ARGB32(uint8_t px8)
{
	//     L8: LLLLLLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000U |
		px8 | (px8 << 8) | (px8 << 16);
}

/**
 * Convert an A4L4 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px8 A4L4 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t A4L4_to_ARGB32(uint8_t px8)
{
	//   A4L4: AAAALLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb = ((px8 & 0xF0) << 20) | (px8 & 0x0F);	// Low nybble of A and B.
	argb |= (argb << 4);				// Copy to high nybble.
	argb |= (argb & 0xFF) <<  8;			// Copy B to G.
	argb |= (argb & 0xFF) << 16;			// Copy B to R.
	return argb;
}

/**
 * Convert an L16 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px16 L16 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t L16_to_ARGB32(uint16_t px16)
{
	// NOTE: This will truncate the luminance.
	// TODO: Add ARGB64 support?
	return L8_to_ARGB32(px16 >> 8);
}

/**
 * Convert an A8L8 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px16 A8L8 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t A8L8_to_ARGB32(uint16_t px16)
{
	//   A8L8: AAAAAAAA LLLLLLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t i = (px16 & 0x00FF);
	i |= ((i << 8) | (i << 16));
	i |= ((px16 & 0xFF00) << 16);
	return i;
}

/**
 * Convert an L8A8 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px16 L8A8 pixel.
 * @return ARGB32 pixel.
 */
static inline uint32_t L8A8_to_ARGB32(uint16_t px16)
{
	// FIXME: What's the component order of IA8?
	// Assuming I=MSB, A=LSB...
	// NOTE: This is the same as IA8_to_ARGB32().

	//   L8A8: LLLLLLLL AAAAAAAA
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t i = (px16 & 0xFF00);
	i |= ((i << 8) | (i >> 8));
	i |= ((px16 & 0x00FF) << 24);
	return i;
}

/** Alpha **/

/**
 * Convert an A8 pixel to ARGB32.
 * NOTE: Uses a black background.
 * @param px8 A8 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t A8_to_ARGB32(uint8_t px8)
{
	//     A8: AAAAAAAA
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return (px8 << 24);
}

/** Other **/

/**
 * Convert an R8 pixel to ARGB32.
 * @param px8 R8 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t R8_to_ARGB32(uint8_t px8)
{
	//     R8: RRRRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000U | (static_cast<uint32_t>(px8) << 16);
}

/**
 * Convert an RGB332 pixel to ARGB32.
 * @param px8 RGB332 pixel.
 * @return ARGB32 pixel.
 */
static inline constexpr uint32_t RGB332_to_ARGB32(uint8_t px8)
{
	// RGB332: RRRGGGBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000U |
		(c3_lookup[(px8 & 0xE0) >> 5] << 16) |	// R
		(c3_lookup[(px8 & 0x1C) >> 2] <<  8) |	// G
		 c2_lookup[(px8 & 0x03)];		// B
}

} }

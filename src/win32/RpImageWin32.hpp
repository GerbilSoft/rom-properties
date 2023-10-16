/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpImageWin32.hpp: rp_image to Win32 conversion functions.               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C includes
#include <stdint.h>

// C++ includes
#include <memory>

namespace LibRpTexture {
	class rp_image;
}

namespace RpImageWin32 {

// TODO: Do we really need all the wrapper functions,
// or should we get rid of the raw pointer versions?

/**
 * Convert an rp_image to HBITMAP.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP(const LibRpTexture::rp_image *image, uint32_t bgColor);

/**
 * Convert an rp_image to HBITMAP.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP(const LibRpTexture::rp_image_ptr &image, uint32_t bgColor)
{
	return toHBITMAP(image.get(), bgColor);
}

/**
 * Convert an rp_image to HBITMAP.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP(const LibRpTexture::rp_image_const_ptr &image, uint32_t bgColor)
{
	return toHBITMAP(image.get(), bgColor);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version resizes the image.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP(const LibRpTexture::rp_image *image, uint32_t bgColor, SIZE size, bool nearest);

/**
 * Convert an rp_image to HBITMAP.
 * This version resizes the image.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP(const LibRpTexture::rp_image_ptr &image, uint32_t bgColor, SIZE size, bool nearest)
{
	return toHBITMAP(image.get(), bgColor, size, nearest);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version resizes the image.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP(const LibRpTexture::rp_image_const_ptr &image, uint32_t bgColor, SIZE size, bool nearest)
{
	return toHBITMAP(image.get(), bgColor, size, nearest);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel.
 * @param image		[in] rp_image
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image *image);

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel.
 * @param image		[in] rp_image
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image_ptr &image)
{
	return toHBITMAP_alpha(image.get());
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel.
 * @param image		[in] rp_image
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image_const_ptr &image)
{
	return toHBITMAP_alpha(image.get());
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel and resizes the image.
 * @param image		[in] rp_image
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image *image, SIZE size, bool nearest);

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel and resizes the image.
 * @param image		[in] rp_image
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image_ptr &image, SIZE size, bool nearest)
{
	return toHBITMAP_alpha(image.get(), size, nearest);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel and resizes the image.
 * @param image		[in] rp_image
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
static inline HBITMAP toHBITMAP_alpha(const LibRpTexture::rp_image_const_ptr &image, SIZE size, bool nearest)
{
	return toHBITMAP_alpha(image.get(), size, nearest);
}

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
HICON toHICON(const LibRpTexture::rp_image *image);

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
static inline HICON toHICON(const LibRpTexture::rp_image_ptr &image)
{
	return toHICON(image.get());
}

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
static inline HICON toHICON(const LibRpTexture::rp_image_const_ptr &image)
{
	return toHICON(image.get());
}

/**
 * Convert an HBITMAP to rp_image.
 * @param hBitmap HBITMAP.
 * @return rp_image.
 */
LibRpTexture::rp_image_ptr fromHBITMAP(HBITMAP hBitmap);

/**
 * Convert an HBITMAP to HICON.
 * @param hBitmap HBITMAP.
 * @return HICON, or nullptr on error.
 */
HICON toHICON(HBITMAP hBitmap);

}

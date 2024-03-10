/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image_ops.cpp: Image class. (operations)                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C includes (C++ namespace)
#include <cmath>

// librptexture
#include "ImageSizeCalc.hpp"

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpTexture {

/** Image operations. **/

/**
 * Duplicate the rp_image.
 * @return New rp_image with a copy of the image data.
 */
rp_image_ptr rp_image::dup(void) const
{
	RP_D(const rp_image);
	const rp_image_backend *const backend = d->backend;

	const int width = backend->width;
	const int height = backend->height;
	const rp_image::Format format = backend->format;
	assert(width > 0);
	assert(height > 0);

	const rp_image_ptr img = std::make_shared<rp_image>(width, height, format);
	if (!img->isValid()) {
		// Image is invalid. Return it immediately.
		return img;
	}

	// Copy the image.
	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(backend->data());
	const int row_bytes = img->row_bytes();
	const int dest_stride = img->stride();
	const int src_stride = backend->stride;

	if (src_stride == dest_stride) {
		// Copy the entire image all at once.
		const size_t len = backend->data_len();
		memcpy(dest, src, len);
	} else {
		// Copy one line at a time.
		for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
			memcpy(dest, src, row_bytes);
			dest += dest_stride;
			src += src_stride;
		}
	}

	// If CI8, copy the palette.
	if (format == rp_image::Format::CI8) {
		const unsigned int entries = std::min(img->palette_len(), backend->palette_len());
		uint32_t *const dest_pal = img->palette();
		memcpy(dest_pal, backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	return img;
}

/**
 * Duplicate the rp_image, converting to ARGB32 if necessary.
 * @return New ARGB32 rp_image with a copy of the image data.
 */
rp_image_ptr rp_image::dup_ARGB32(void) const
{
	RP_D(const rp_image);
	const rp_image_backend *const backend = d->backend;

	if (backend->format == Format::ARGB32) {
		// Already in ARGB32.
		// Do a direct dup().
		return this->dup();
	} else if (backend->format != Format::CI8) {
		// Only CI8->ARGB32 is supported right now.
		return nullptr;
	}

	const int width = backend->width;
	const int height = backend->height;
	assert(width > 0);
	assert(height > 0);

	// TODO: Handle palette length smaller than 256.
	assert(backend->palette_len() == 256);
	if (backend->palette_len() != 256) {
		return nullptr;
	}

	rp_image_ptr img = std::make_shared<rp_image>(width, height, Format::ARGB32);
	if (!img->isValid()) {
		// Image is invalid. Something went wrong.
		return nullptr;
	}

	// Copy the image, converting from CI8 to ARGB32.
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(backend->data());
	const uint32_t *pal = backend->palette();
	const int dest_adj = (img->stride() / 4) - width;
	const int src_adj = backend->stride - width;

	for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
		// Convert up to 4 pixels per loop iteration.
		unsigned int x;
		for (x = static_cast<unsigned int>(width); x > 3; x -= 4) {
			dest[0] = pal[src[0]];
			dest[1] = pal[src[1]];
			dest[2] = pal[src[2]];
			dest[3] = pal[src[3]];
			dest += 4;
			src += 4;
		}
		// Remaining pixels.
		for (; x > 0; x--) {
			*dest = pal[*src];
			dest++;
			src++;
		}

		// Next line.
		dest += dest_adj;
		src += src_adj;
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	// Converted to ARGB32.
	return img;
}

/**
 * Square the rp_image.
 *
 * If the width and height don't match, transparent rows
 * and/or columns will be added to "square" the image.
 * Otherwise, this is the same as dup().
 *
 * @return New rp_image with a squared version of the original, or nullptr on error.
 */
rp_image_ptr rp_image::squared(void) const
{
	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	RP_D(const rp_image);
	const rp_image_backend *const backend = d->backend;

	const int width = backend->width;
	const int height = backend->height;
	assert(width > 0);
	assert(height > 0);
	if (width <= 0 || height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	if (width == height) {
		// Image is already square. dup() it.
		return this->dup();
	}

	// Image needs adjustment.
	// TODO: Native 8bpp support?
	rp_image_ptr tmp_rp_image;
	if (backend->format != rp_image::Format::ARGB32) {
		// Convert to ARGB32 first.
		tmp_rp_image = this->dup_ARGB32();
	}

	// Create the squared image.
	const int max_dim = std::max(width, height);
	rp_image_ptr sq_img = std::make_shared<rp_image>(max_dim, max_dim, rp_image::Format::ARGB32);
	if (!sq_img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(sq_img->bits());
	const int dest_stride = sq_img->stride();

	const uint8_t *src;
	int src_stride;
	int src_row_bytes;
	if (!tmp_rp_image) {
		// Not using a temporary image.
		src = static_cast<const uint8_t*>(backend->data());
		src_stride = backend->stride;
		src_row_bytes = this->row_bytes();
	} else {
		// Using a temporary image.
		const rp_image_backend *const tmp_backend = tmp_rp_image->d_ptr->backend;
		src = static_cast<const uint8_t*>(tmp_backend->data());
		src_stride = tmp_backend->stride;
		src_row_bytes = tmp_rp_image->row_bytes();
	}

	if (width > height) {
		// Image is wider. Add rows to the top and bottom.
		const int addToTop = (width-height)/2;
		const int addToBottom = addToTop + ((width-height)%2);

		// Clear the top rows.
		memset(dest, 0, ImageSizeCalc::T_calcImageSize(addToTop, dest_stride));
		dest += (addToTop * dest_stride);

		// Copy the image data.
		const int row_bytes = sq_img->row_bytes();
		for (unsigned int y = height; y > 0; y--) {
			memcpy(dest, src, row_bytes);
			dest += dest_stride;
			src += src_stride;
		}

		// Clear the bottom rows.
		// NOTE: Last row may not be the full stride.
		memset(dest, 0, ImageSizeCalc::T_calcImageSize(addToBottom-1, dest_stride) + sq_img->row_bytes());
	} else /*if (width < height)*/ {
		// Image is taller. Add columns to the left and right.

		// NOTE: Mega Man Gold amiibo is "shifting" by 1px when
		// refreshing in Win7. (switching from icon to thumbnail)
		// Not sure if this can be fixed easily.
		const int addToLeft = (height-width)/2;
		const int addToRight = addToLeft + ((height-width)%2);

		// "Blanking" area is right border, potential unused space from stride, then left border.
		const int dest_blanking = sq_img->stride() - src_row_bytes;

		// Clear the left part of the first row.
		memset(dest, 0, addToLeft * sizeof(uint32_t));
		dest += (addToLeft * sizeof(uint32_t));

		// Copy and clear all but the last line.
		for (int y = (height-2); y >= 0; y--) {
			memcpy(dest, src, src_row_bytes);
			memset(&dest[src_row_bytes], 0, dest_blanking);
			dest += dest_stride;
			src += src_stride;
		}

		// Copy the last line.
		// NOTE: Last row may not be the full stride.
		memcpy(dest, src, src_row_bytes);
		// Clear the end of the line.
		memset(&dest[src_row_bytes], 0, addToRight * sizeof(uint32_t));
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		sq_img->set_sBIT(&d->sBIT);
	}

	return sq_img;
}

/**
 * Resize the rp_image.
 *
 * A new rp_image will be created with the specified dimensions,
 * and the current image will be copied into the new image.
 *
 * If the new dimensions are smaller than the old dimensions,
 * the image will be cropped according to the specified alignment.
 *
 * If the new dimensions are larger, the original image will be
 * aligned to the top, center, or bottom, depending on alignment,
 * and if the image is ARGB32, the new space will be set to bgColor.
 *
 * @param width New width
 * @param height New height
 * @param alignment Alignment (Vertical only!)
 * @param bgColor Background color for empty space. (default is ARGB 0x00000000)
 * @return New rp_image with a resized version of the original, or nullptr on error.
 */
rp_image_ptr rp_image::resized(int width, int height, Alignment alignment, uint32_t bgColor) const
{
	assert(width > 0);
	assert(height > 0);
	if (width <= 0 || height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	RP_D(const rp_image);
	const rp_image_backend *const backend = d->backend;

	const int orig_width = backend->width;
	const int orig_height = backend->height;
	assert(orig_width > 0);
	assert(orig_height > 0);
	if (orig_width <= 0 || orig_height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	if (width == orig_width && height == orig_height) {
		// No resize is necessary.
		return this->dup();
	}

	const rp_image::Format format = backend->format;
	rp_image_ptr img = std::make_shared<rp_image>(width, height, format);
	if (!img->isValid()) {
		// Image is invalid.
		return nullptr;
	}

	// Copy the image.
	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(backend->data());
	const int dest_stride = img->stride();
	const int src_stride = backend->stride;

	// We want to copy the minimum of new vs. old width.
	int row_bytes = std::min(width, orig_width);
	if (format == rp_image::Format::ARGB32) {
		row_bytes *= sizeof(uint32_t);
	}

	// Are we copying to a taller or shorter image?
	unsigned int copy_height;
	if (height < orig_height) {
		// New image is shorter.
		switch (alignment & AlignVertical_Mask) {
			default:
			case AlignTop:
				// Top alignment.
				// No adjustment needed.
				break;
			case AlignVCenter:
				// Middle alignment.
				// Start at the middle of the original image.
				// If the original image is 64px tall,
				// and the new image is 32px tall,
				// we want to start at 16px: ((64 - 32) / 2)
				src += (src_stride * ((orig_height - height) / 2));
				break;
			case AlignBottom:
				// Bottom alignment.
				// Start at the bottom of the original image.
				src += (src_stride * (orig_height - height));
				break;
		}
		copy_height = height;
	} else if (height > orig_height) {
		// New image is taller.
		switch (alignment & AlignVertical_Mask) {
			default:
			case AlignTop:
				// Top alignment.
				// No adjustment needed.
				break;
			case AlignVCenter:
				// Center alignment.
				// Start at the middle of the new image.
				// If the original image is 32px tall,
				// and the new image is 64px tall,
				// we want to start at 16px: ((64 - 32) / 2)

				// If this is ARGB32, set the background color of the top half.
				// TODO: Optimize this.
				if (format == rp_image::Format::ARGB32 && bgColor != 0x00000000) {
					for (unsigned int y = (height - orig_height) / 2; y > 0; y--) {
						uint32_t *dest32 = reinterpret_cast<uint32_t*>(dest);
						for (unsigned int x = width; x > 0; x--) {
							*dest32++ = bgColor;
						}
						dest += dest_stride;
					}
				} else {
					// Just skip the blank area.
					dest += (dest_stride * ((height - orig_height) / 2));
				}
				break;
			case AlignBottom:
				// Bottom alignment.
				// Start at the bottom of new original image.

				// If this is ARGB32, set the background color of the blank area.
				// TODO: Optimize this.
				if (format == rp_image::Format::ARGB32 && bgColor != 0x00000000) {
					for (unsigned int y = (height - orig_height); y > 0; y--) {
						uint32_t *dest32 = reinterpret_cast<uint32_t*>(dest);
						for (unsigned int x = width; x > 0; x--) {
							*dest32++ = bgColor;
						}
						dest += dest_stride;
					}
				} else {
					// Just skip the blank area.
					dest += (dest_stride * (height - orig_height));
				}
				break;
		}
		copy_height = orig_height;
	} else {
		// New image has the same height.
		copy_height = orig_height;
	}

	for (; copy_height > 0; copy_height--) {
		memcpy(dest, src, row_bytes);
		dest += dest_stride;
		src += src_stride;
	}

	// If the image is taller, we may need to clear the bottom section.
	if (height > orig_height && format == rp_image::Format::ARGB32) {
		switch (alignment & AlignVertical_Mask) {
			default:
			case AlignTop:
				// Top alignment.
				// Set the background color of the blank area.
				for (unsigned int y = (height - orig_height); y > 0; y--) {
					uint32_t *dest32 = reinterpret_cast<uint32_t*>(dest);
					for (unsigned int x = width; x > 0; x--) {
						*dest32++ = bgColor;
					}
					dest += dest_stride;
				}
				break;

			case AlignVCenter: {
				// Center alignment.
				// Set the background color of the bottom half.
				unsigned int y = height - orig_height;
				if (y & 1) {
					// y is odd, so draw the extra odd line on the bottom.
					y = (y / 2) + 1;
				} else {
					// y is even.
					y /= 2;
				}

				for (; y > 0; y--) {
					uint32_t *dest32 = reinterpret_cast<uint32_t*>(dest);
					for (unsigned int x = width; x > 0; x--) {
						*dest32++ = bgColor;
					}
					dest += dest_stride;
				}
				break;
			}

			case AlignBottom:
				// Bottom alignment.
				// Nothing to do here...
				break;
		}
	}

	// If CI8, copy the palette.
	if (format == rp_image::Format::CI8) {
		const unsigned int entries = std::min(img->palette_len(), backend->palette_len());
		uint32_t *const dest_pal = img->palette();
		memcpy(dest_pal, backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// Copy sBIT if it's set.
	// TODO: Make sure alpha is at least 1?
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	// Image resized.
	return img;
}

/**
 * Convert a chroma-keyed image to standard ARGB32.
 * Standard version using regular C++ code.
 *
 * This operates on the image itself, and does not return
 * a duplicated image with the adjusted image.
 *
 * NOTE: The image *must* be ARGB32.
 *
 * @param key Chroma key color.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::apply_chroma_key_cpp(uint32_t key)
{
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;

	assert(backend->format == Format::ARGB32);
	if (backend->format != Format::ARGB32) {
		// ARGB32 only.
		return -EINVAL;
	}

	const unsigned int diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	uint32_t *img_buf = static_cast<uint32_t*>(backend->data());

	for (unsigned int y = static_cast<unsigned int>(backend->height); y > 0; y--) {
		unsigned int x = static_cast<unsigned int>(backend->width);
		for (; x > 1; x -= 2, img_buf += 2) {
			// Check for chroma key pixels.
			if (img_buf[0] == key) {
				img_buf[0] = 0;
			}
			if (img_buf[1] == key) {
				img_buf[1] = 0;
			}
		}

		if (x == 1) {
			if (*img_buf == key) {
				*img_buf = 0;
			}
			img_buf++;
		}

		// Next row.
		img_buf += diff;
	}

	// Adjust sBIT.
	// TODO: Only if transparent pixels were found.
	if (d->has_sBIT && d->sBIT.alpha == 0) {
		d->sBIT.alpha = 1;
	}

	// Chroma key applied.
	return 0;
}

/**
 * Flip the image.
 *
 * This function returns a *new* image and leaves the
 * original image unmodified.
 *
 * @param op Flip operation.
 * @return Flipped image, or nullptr on error.
 */
rp_image_ptr rp_image::flip(FlipOp op) const
{
	assert(op >= FLIP_V);
	assert(op <= FLIP_VH);
	if (op == 0) {
		// No-op...
		return this->dup();
	} else if (op < FLIP_V || op > FLIP_VH) {
		// Not supported.
		return nullptr;
	}

	RP_D(const rp_image);
	rp_image_backend *const backend = d->backend;

	const int width = backend->width;
	const int height = backend->height;
	assert(width > 0 && height > 0);
	if (width <= 0 || height <= 0) {
		return nullptr;
	}

	const int row_bytes = this->row_bytes();
	rp_image_ptr flipimg = std::make_shared<rp_image>(width, height, backend->format);
	const uint8_t *src = static_cast<const uint8_t*>(backend->data());
	uint8_t *dest;
	if (op & FLIP_V) {
		// Vertical flip: Destination starts at the bottom of the image.
		dest = static_cast<uint8_t*>(flipimg->scanLine(height - 1));
	} else {
		// Not a vertical flip: Destination starts at the top of the image.
		dest = static_cast<uint8_t*>(flipimg->bits());
	}

	int src_stride = backend->stride;
	int dest_stride = flipimg->stride();
	if (op & FLIP_H) {
		if (op & FLIP_V) {
			// Vertical flip: Subtract the destination stride.
			dest_stride = -dest_stride;
		}

		// Horizontal flip: Copy one pixel at a time.
		// TODO: Improve performance by using pointer arithmetic.
		// The algorithm gets ridiculously complicated and I couldn't
		// get it right, so I'm not doing that right now.
		switch (backend->format) {
			default:
				assert(!"rp_image format not supported for H-flip.");
				return nullptr;

			case rp_image::Format::CI8:
				// 8-bit copy.
				for (int y = height; y > 0; y--) {
					int dx = 0;
					for (int x = width-1; x >= 0; x--, dx++) {
						dest[dx] = src[x];
					}
					src += src_stride;
					dest += dest_stride;
				}
				break;

			case rp_image::Format::ARGB32: {
				// 32-bit copy.
				const uint32_t *src32 = reinterpret_cast<const uint32_t*>(src);
				uint32_t *dest32 = reinterpret_cast<uint32_t*>(dest);
				src_stride /= sizeof(uint32_t);
				dest_stride /= sizeof(uint32_t);

				for (int y = height; y > 0; y--) {
					int dx = 0;
					for (int x = width-1; x >= 0; x--, dx++) {
						dest32[dx] = src32[x];
					}
					src32 += src_stride;
					dest32 += dest_stride;
				}
				break;
			}
		}
	} else {
		if (op & FLIP_V) {
			// Vertical flip: Subtract the destination stride.
			dest_stride = -dest_stride;
		}

		// Not a horizontal flip. Copy one line at a time.
		for (int y = height; y > 0; y--) {
			memcpy(dest, src, row_bytes);
			src += src_stride;
			dest += dest_stride;
		}
	}

	// If CI8, copy the palette.
	if (backend->format == Format::CI8) {
		const int entries = std::min(flipimg->palette_len(), d->backend->palette_len());
		uint32_t *const dest_pal = flipimg->palette();
		memcpy(dest_pal, d->backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// If CI8, copy the palette.
	if (backend->format == rp_image::Format::CI8) {
		const unsigned int entries = std::min(flipimg->palette_len(), backend->palette_len());
		uint32_t *const dest_pal = flipimg->palette();
		memcpy(dest_pal, backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		flipimg->set_sBIT(&d->sBIT);
	}

	return flipimg;
}

/**
 * Shrink image dimensions.
 * @param width New width.
 * @param height New height.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::shrink(int width, int height)
{
	RP_D(rp_image);
	return d->backend->shrink(width, height);
}

/**
 * Swizzle the image channels.
 * Standard version using regular C++ code.
 *
 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::swizzle_cpp(const char *swz_spec)
{
	// TODO: Improve C performance.
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// ARGB32 is required.
		// TODO: Automatically convert the image?
		return -EINVAL;
	}

	// TODO: Verify swz_spec.
	typedef union _u8_32 {
		uint8_t u8[4];
		uint32_t u32;
	} u8_32;
	u8_32 swz_ch;
	memcpy(&swz_ch, swz_spec, sizeof(swz_ch));
	if (swz_ch.u32 == 'rgba') {
		// 'rgba' == NULL swizzle. Don't bother doing anything.
		return 0;
	}

	// NOTE: Texture uses ARGB format, but swizzle uses rgba.
	// Rotate swz_ch to convert it to argb.
	// The entire thing needs to be byteswapped to match the internal order, too.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// LE: Rotate 8-bits right
	swz_ch.u32 = (swz_ch.u32 >> 24) | (swz_ch.u32 <<  8);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	// BE: Rotate 8-bits left
	swz_ch.u32 = (swz_ch.u32 >>  8) | (swz_ch.u32 << 24);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	uint32_t *bits = static_cast<uint32_t*>(backend->data());
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	const int width = backend->width;
	for (int y = backend->height; y > 0; y--) {
		for (int x = width; x > 0; x--, bits++) {
			u8_32 cur, swz;
			cur.u32 = *bits;

#define SWZ_CH_B 3
#define SWZ_CH_G 2
#define SWZ_CH_R 1
#define SWZ_CH_A 0

#define SWIZZLE_CHANNEL(n) do { \
				switch (swz_ch.u8[n]) { \
					case 'b':	swz.u8[n] = cur.u8[SWZ_CH_B];	break; \
					case 'g':	swz.u8[n] = cur.u8[SWZ_CH_G];	break; \
					case 'r':	swz.u8[n] = cur.u8[SWZ_CH_R];	break; \
					case 'a':	swz.u8[n] = cur.u8[SWZ_CH_A];	break; \
					case '0':	swz.u8[n] = 0;			break; \
					case '1':	swz.u8[n] = 255;		break; \
					default: \
						assert(!"Invalid swizzle value."); \
						swz.u8[n] = 0; \
						break; \
				} \
			} while (0)

			SWIZZLE_CHANNEL(0);
			SWIZZLE_CHANNEL(1);
			SWIZZLE_CHANNEL(2);
			SWIZZLE_CHANNEL(3);

			*bits = swz.u32;
		}

		// Next row.
		bits += stride_diff;
	}

	// Swizzle the sBIT value, if set.
	if (d->has_sBIT) {
		// TODO: If gray is set, move its values to rgb?
		const rp_image::sBIT_t sBIT_old = d->sBIT;

#define SWIZZLE_sBIT(n, ch) do { \
				switch (swz_ch.u8[n]) { \
					case 'b':	d->sBIT.ch = sBIT_old.blue;	break; \
					case 'g':	d->sBIT.ch = sBIT_old.green;	break; \
					case 'r':	d->sBIT.ch = sBIT_old.red;	break; \
					case 'a':	d->sBIT.ch = sBIT_old.alpha;	break; \
					case '0': case '1': \
							d->sBIT.ch = 1;			break; \
				} \
			} while (0)

			SWIZZLE_sBIT(SWZ_CH_B, blue);
			SWIZZLE_sBIT(SWZ_CH_G, green);
			SWIZZLE_sBIT(SWZ_CH_R, red);
			SWIZZLE_sBIT(SWZ_CH_A, alpha);
	}

	return 0;
}

static inline float saturate(float a)
{
	if (a < 0) a = 0;
	if (a > 1) a = 1;
	return a;
}

/**
 * Unswizzle GIMP-DDS YCoCg.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::unswizzle_YCoCg(void)
{
	// TODO: SSE-optimized version.
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// ARGB32 is required.
		// TODO: Automatically convert the image?
		return -EINVAL;
	}

	const int width = backend->width;
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(argb32_t);
	argb32_t *bits = static_cast<argb32_t*>(backend->data());

	// Conversion offset (for YCoCg to RGB)
	static const float YCoCg_offset = 0.5f * 256.0f / 255.0f;

	for (unsigned int y = static_cast<unsigned int>(backend->height); y > 0; y--) {
		for (unsigned int x = static_cast<unsigned int>(width); x > 0; x--) {
			// References:
			// - https://en.wikipedia.org/wiki/YCoCg
			// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-dds/misc.c

			const float Y  = ((float)bits->YCoCg.y  / 255.0f);
			const float Co = ((float)bits->YCoCg.co / 255.0f) - YCoCg_offset;
			const float Cg = ((float)bits->YCoCg.cg / 255.0f) - YCoCg_offset;

			const float Y_minus_Cg = Y - Cg;
			const float R = saturate(Y_minus_Cg + Co);
			const float G = saturate(Y + Cg);
			const float B = saturate(Y_minus_Cg - Co);
			const uint8_t A = bits->YCoCg.a;

			bits->r = (uint8_t)lrintf(R * 255.0f);
			bits->g = (uint8_t)lrintf(G * 255.0f);
			bits->b = (uint8_t)lrintf(B * 255.0f);
			bits->a = A;

			bits++;
		}
		bits += stride_diff;
	}

	return 0;
}

/**
 * Unswizzle GIMP-DDS YCoCg (scaled).
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::unswizzle_YCoCg_scaled(void)
{
	// TODO: SSE-optimized version.
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// ARGB32 is required.
		// TODO: Automatically convert the image?
		return -EINVAL;
	}

	const int width = backend->width;
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(argb32_t);
	argb32_t *bits = static_cast<argb32_t*>(backend->data());

	// Conversion offset (for YCoCg to RGB)
	static const float YCoCg_offset = 0.5f * 256.0f / 255.0f;

	for (unsigned int y = static_cast<unsigned int>(backend->height); y > 0; y--) {
		for (unsigned int x = static_cast<unsigned int>(width); x > 0; x--) {
			// References:
			// - https://en.wikipedia.org/wiki/YCoCg
			// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-dds/misc.c

			const float Y  = ((float)bits->YCoCg.y  / 255.0f);
			float Co = ((float)bits->YCoCg.co / 255.0f) - YCoCg_offset;
			float Cg = ((float)bits->YCoCg.cg / 255.0f) - YCoCg_offset;

			// YCoCg (scaled) uses the alpha component as a scaling value.
			float S  = ((float)bits->YCoCg.a  / 255.0f);
			S = 1.0f / ((255.0f / 8.0f) * S + 1.0f);

			// Scale the Co and Cg components.
			Co *= S;
			Cg *= S;

			const float Y_minus_Cg = Y - Cg;
			const float R = saturate(Y_minus_Cg + Co);
			const float G = saturate(Y + Cg);
			const float B = saturate(Y_minus_Cg - Co);

			bits->r = (uint8_t)lrintf(R * 255.0f);
			bits->g = (uint8_t)lrintf(G * 255.0f);
			bits->b = (uint8_t)lrintf(B * 255.0f);
			bits->a = 255;	// no alpha channel

			bits++;
		}
		bits += stride_diff;
	}

	return 0;
}

/**
 * Unswizzle GIMP-DDS Alpha Exponent.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::unswizzle_AExp(void)
{
	// TODO: SSE-optimized version.
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// ARGB32 is required.
		// TODO: Automatically convert the image?
		return -EINVAL;
	}

	const int width = backend->width;
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(argb32_t);
	argb32_t *bits = static_cast<argb32_t*>(backend->data());

	for (unsigned int y = static_cast<unsigned int>(backend->height); y > 0; y--) {
		for (unsigned int x = static_cast<unsigned int>(width); x > 0; x--) {
			// References:
			// - https://en.wikipedia.org/wiki/YCoCg
			// - https://gitlab.gnome.org/GNOME/gimp/-/blob/master/plug-ins/file-dds/misc.c

			unsigned int R = bits->r;
			unsigned int G = bits->g;
			unsigned int B = bits->b;
			const unsigned int A = bits->a;

			// RGB values are scaled by the A value.
			// Alpha channel is then set to 255. (no alpha)
			R = (R * A + 1) >> 8;
			G = (G * A + 1) >> 8;
			B = (B * A + 1) >> 8;

			bits->r = (uint8_t)R;
			bits->g = (uint8_t)G;
			bits->b = (uint8_t)B;
			bits->a = 255;	// no alpha channel

			bits++;
		}
		bits += stride_diff;
	}

	// Zero out the alpha channel in the sBIT metadata.
	if (d->has_sBIT) {
		d->sBIT.alpha = 0;
	}

	return 0;
}

}

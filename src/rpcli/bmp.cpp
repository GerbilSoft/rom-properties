/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * bmp.cpp: BMP output for rp_image.                                       *
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

#include "bmp.hpp"
#include "bmp_structs.h"

#include "libromdata/byteswap.h"
#include "libromdata/common.h"
#include "libromdata/img/rp_image.hpp"
using LibRomData::rp_image;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <ostream>
#include <fstream>
using std::ostream;

int rpbmp(std::ostream& os, const rp_image *img)
{
	assert(img != nullptr);
	assert(img->isValid());
	if (!img || !img->isValid()) {
		return -1;
	}
	if (img->format() != rp_image::FORMAT_ARGB32 && img->format() != rp_image::FORMAT_CI8) {
		// Unsupported image format
		assert(img->format() == rp_image::FORMAT_NONE); // Should be none unless new format is added
		return -1;
	}

	BITMAPFILEHEADER fhead;
	fhead.bfType = cpu_to_le16(0x4D42);
	fhead.bfSize = cpu_to_le32(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
				img->palette_len()*4 + (uint32_t)img->data_len());
	fhead.bfReserved1 = 0;
	fhead.bfReserved2 = 0;
	fhead.bfOffBits	= cpu_to_le32(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
				img->palette_len()*4);
	os.write(reinterpret_cast<const char*>(&fhead), sizeof(fhead));

	BITMAPINFOHEADER ihead;
	memset(&ihead, 0, sizeof(ihead));
	ihead.biSize		= cpu_to_le32((uint32_t)sizeof(ihead));
	ihead.biWidth		= cpu_to_le32(img->width());
	// FIXME: Verify byteswapping of negative numbers.
	ihead.biHeight		= cpu_to_le32(-img->height());	// Negative for top-down.
	ihead.biPlanes		= cpu_to_le32(1);
	ihead.biBitCount	= cpu_to_le16(img->format() == rp_image::FORMAT_CI8 ? 8 : 32);
	ihead.biCompression	= cpu_to_le32(BI_RGB);
	ihead.biSizeImage	= cpu_to_le32(0);
	ihead.biXPelsPerMeter	= cpu_to_le32(0);
	ihead.biYPelsPerMeter	= cpu_to_le32(0);
	if (img->format() == rp_image::FORMAT_CI8) {
		ihead.biClrUsed		= cpu_to_le32(img->palette_len());
	} else {
		ihead.biClrUsed		= cpu_to_le32(0);
	}
	ihead.biClrImportant	= ihead.biClrUsed;
	os.write(reinterpret_cast<const char*>(&ihead), sizeof(ihead));

	if (img->format() == rp_image::FORMAT_CI8) {
		// Write the palette for CI8 images.
		os.write(reinterpret_cast<const char*>(img->palette()), img->palette_len()*4);
	}

	// Write the image data.
	os.write(static_cast<const char*>(img->bits()), img->data_len());
	return 0;
}

int rpbmp(const rp_char *filename, const LibRomData::rp_image *img)
{
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		return -1;
	}
	return rpbmp(file, img);
}

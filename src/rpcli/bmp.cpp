/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * bmp.cpp: BMP output for rp_image.                                       *
 *                                                                         *
 * Copyright (c) 2016 by Egor.                                             *
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
#include <ostream>
#include <libromdata/rp_image.hpp>
#include <libromdata/byteswap.h>
#include <libromdata/common.h>
using std::ostream;
using LibRomData::rp_image;


#pragma pack(1)
struct PACKED BITMAPFILEHEADER {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
	BITMAPFILEHEADER():
		bfType(0x4D42), // "BM"
		bfSize(0),
		bfReserved1(0),
		bfReserved2(0),
		bfOffBits(0){}
	inline void swap(){ // useful for big-endian only
		bfType		=cpu_to_le32(bfType);
		bfSize		=cpu_to_le32(bfSize);
		bfReserved1	=cpu_to_le16(bfReserved1);
		bfReserved2	=cpu_to_le16(bfReserved2);
		bfOffBits	=cpu_to_le32(bfOffBits);
	}
};
struct PACKED BITMAPINFOHEADER {
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	BITMAPINFOHEADER():
		biSize(40),
		biWidth(0),
		biHeight(0),
		biPlanes(1),
		biBitCount(0),
		biCompression(0),
		biSizeImage(0),
		biXPelsPerMeter(0),
		biYPelsPerMeter(0),
		biClrUsed(0),
		biClrImportant(0){}
	inline void swap(){ // useful for big-endian only
		biSize		=cpu_to_le32(biSize);
		biWidth		=cpu_to_le32(biWidth);
		biHeight	=cpu_to_le32(biHeight);
		biPlanes	=cpu_to_le16(biPlanes);
		biBitCount	=cpu_to_le16(biBitCount);
		biCompression	=cpu_to_le32(biCompression);
		biSizeImage	=cpu_to_le32(biSizeImage);
		biXPelsPerMeter	=cpu_to_le32(biXPelsPerMeter);
		biYPelsPerMeter	=cpu_to_le32(biYPelsPerMeter);
		biClrUsed	=cpu_to_le32(biClrUsed);
		biClrImportant	=cpu_to_le32(biClrImportant);
	}
};
#pragma pack()
int rpbmp(std::ostream& os, const rp_image* img){
	if(!img || !img->isValid() || img->format() == rp_image::FORMAT_NONE) return -1;
	
	BITMAPFILEHEADER fhead;
	fhead.bfSize	= 14+40+img->palette_len()*4+img->data_len();
	fhead.bfOffBits	= 14+40+img->palette_len()*4;
	fhead.swap();
	os.write((char*)&fhead,sizeof(fhead));
	
	BITMAPINFOHEADER ihead;
	ihead.biWidth		= img->width();
	ihead.biHeight		= -img->height();
	ihead.biBitCount	= img->format() == rp_image::FORMAT_CI8 ? 8 : 32;
	ihead.biClrUsed		= img->palette_len();
	ihead.swap();
	os.write((char*)&ihead,sizeof(ihead));
	
	if(img->format() == rp_image::FORMAT_CI8)
		os.write((const char*)img->palette(),img->palette_len()*4);
	
	os.write((const char*)img->bits(),img->data_len());
	return 0;
}

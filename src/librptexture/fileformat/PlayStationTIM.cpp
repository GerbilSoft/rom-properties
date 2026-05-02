/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PlayStationTIM.cpp: Sony PlayStation .tim image reader.                 *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://qhimm-modding.fandom.com/wiki/PSX/TIM_file
// - http://justsolve.archiveteam.org/wiki/TIM_(PlayStation_graphics)

#include "stdafx.h"
#include "PlayStationTIM.hpp"
#include "FileFormat_p.hpp"

#include "ps1_tim_structs.h"

// Other rom-properties libraries
using namespace LibRpFile;
using LibRpBase::RomFields;

// librptexture
//#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpTexture {

class PlayStationTIMPrivate final : public FileFormatPrivate
{
	public:
		PlayStationTIMPrivate(PlayStationTIM *q, const IRpFilePtr &file);

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(PlayStationTIMPrivate)

	public:
		/** TextureInfo **/
		static const array<const char*, 1+1> exts;
		static const array<const char*, 1+1> mimeTypes;
		static const TextureInfo textureInfo;

	public:
		// TIM headers
		PS1_TIM_Header_t timHeader;
		PS1_TIM_CLUT_Header_t clutHeader;
		PS1_TIM_Image_Header_t imageHeader;

		// CLUT and texture (image) data start addresses.
		uint32_t clutDataStartAddr;
		uint32_t texDataStartAddr;

		// Decoded image
		rp_image_ptr img;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(void);
};

FILEFORMAT_IMPL(PlayStationTIM)

/** PlayStationTIMPrivate **/

/* TextureInfo */
const array<const char*, 1+1> PlayStationTIMPrivate::exts = {{
	".tim",

	nullptr
}};
const array<const char*, 1+1> PlayStationTIMPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-ps1-tim",

	nullptr
}};
const TextureInfo PlayStationTIMPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

PlayStationTIMPrivate::PlayStationTIMPrivate(PlayStationTIM *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, clutDataStartAddr(0)
	, texDataStartAddr(0)
{
	// Clear the structs and arrays.
	memset(&timHeader, 0, sizeof(timHeader));
	memset(&clutHeader, 0, sizeof(clutHeader));
	memset(&imageHeader, 0, sizeof(imageHeader));
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr PlayStationTIMPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file) {
		// Can't load the image.
		return nullptr;
	}

	if (texDataStartAddr == 0) {
		// No texture data start address...
		return nullptr;
	}

	// CLUT buffer
	// TODO: Determine the size based on CLUT width/height?
	// For now, hard-coding 16/256.
	rp::uvector<uint16_t> clut_buf;
	if (timHeader.flags & PS1_TIM_FLAG_CLP) {
		// Load the CLUT.
		if (clutDataStartAddr == 0) {
			// CLUT is present, but no CLUT data start address...
			// TODO: Process it anyway, but with grayscale like no-CLUT images?
			return nullptr;
		}

		uint32_t clut_size;	// in uint16_t units
		switch (timHeader.flags & PS1_TIM_FLAG_BPP_MASK) {
			case PS1_TIM_FLAG_BPP_4BPP:
				clut_size = 16;
				break;
			case PS1_TIM_FLAG_BPP_8BPP:
				clut_size = 256;
				break;
			default:
				assert(!"CLUT is not applicable for this color depth! (15-bpp or 24-bpp)");
				return nullptr;
		}
		clut_buf.resize(clut_size);
		size_t size = file->seekAndRead(clutDataStartAddr, clut_buf.data(), clut_size * sizeof(uint16_t));
		if (size != (clut_size * sizeof(uint16_t))) {
			// Seek and/or read error.
			return nullptr;
		}
	} else {
		// No CLUT. If this is 4-bpp or 8-bpp, create a grayscale CLUT.
		switch (timHeader.flags & PS1_TIM_FLAG_BPP_MASK) {
			case PS1_TIM_FLAG_BPP_4BPP: {
				clut_buf.resize(16);
				uint8_t ch_8bpp = 0;
				for (unsigned int i = 0; i < 16; i++, ch_8bpp += 0x11) {
					// Need to convert to 5-bit BGR.
					const uint8_t ch_5bpp = (ch_8bpp >> 3);
					clut_buf[i] = 0x8000 | (ch_5bpp << 10) | (ch_5bpp << 5) | ch_5bpp;
				}
				break;
			}

			case PS1_TIM_FLAG_BPP_8BPP: {
				clut_buf.resize(256);
				uint8_t ch_8bpp = 0;
				for (unsigned int i = 0; i < 256; i++, ch_8bpp += 1) {
					// Need to convert to 5-bit BGR.
					const uint8_t ch_5bpp = (ch_8bpp >> 3);
					clut_buf[i] = 0x8000 | (ch_5bpp << 10) | (ch_5bpp << 5) | ch_5bpp;
				}
				break;
			}

			default:
				break;
		}
	}

	// TIM always uses units of 16-bit pixels for the image data.
	// For 15-bpp, we can use the width as-is.
	// For others, we have to round the rows to the nearest 16-bit unit.
	const int stride = le32_to_cpu(imageHeader.fb.width) * sizeof(uint16_t);
	const int width = this->dimensions[0];
	const int height = this->dimensions[1];

	// NOTE: fb.width is always in units of 16-bit pixels.
	// The actual image width depends on the format.
	uint32_t img_siz = static_cast<uint32_t>(stride) * static_cast<uint32_t>(height);
	unique_ptr<uint8_t[]> img_buf(new uint8_t[img_siz]);
	size_t size = file->seekAndRead(texDataStartAddr, img_buf.get(), img_siz);
	if (size != img_siz) {
		// Seek and/or read error.
		return nullptr;
	}

	// Process the image.
	rp_image_ptr imgtmp;
	switch (timHeader.flags & PS1_TIM_FLAG_BPP_MASK) {
		case PS1_TIM_FLAG_BPP_4BPP:
			imgtmp = ImageDecoder::fromLinearCI4(
				ImageDecoder::PixelFormat::BGR555_PS1, false, width, height,
				img_buf.get(), img_siz,
				clut_buf.data(), clut_buf.size() * sizeof(uint16_t),
				stride);
			break;

		case PS1_TIM_FLAG_BPP_8BPP:
			imgtmp = ImageDecoder::fromLinearCI8(
				ImageDecoder::PixelFormat::BGR555_PS1, width, height,
				img_buf.get(), img_siz,
				clut_buf.data(), clut_buf.size() * sizeof(uint16_t),
				stride);
			break;

		case PS1_TIM_FLAG_BPP_15BPP:
			imgtmp = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::BGR555_PS1, width, height,
				reinterpret_cast<const uint16_t*>(img_buf.get()), img_siz, stride);
			break;

		case PS1_TIM_FLAG_BPP_24BPP:
			imgtmp = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::BGR888, width, height,
				img_buf.get(), img_siz, stride);
			break;

		default:
			assert(!"Invalid BPP???");
			return nullptr;
	}

	img = imgtmp;
	return imgtmp;
}

/** PlayStationTIM **/

/**
 * Read a Sony PlayStation TIM image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
PlayStationTIM::PlayStationTIM(const IRpFilePtr &file)
	: super(new PlayStationTIMPrivate(this, file))
{
	RP_D(PlayStationTIM);
	d->mimeType = "image/x-ps1-tim";	// unofficial, not on fd.o
	d->textureFormatName = "Sony PlayStation TIM";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the TIM header.
	d->file->rewind();
	size_t size = d->file->read(&d->timHeader, sizeof(d->timHeader));
	if (size != sizeof(d->timHeader)) {
		d->file.reset();
		return;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the TIM flags field to host-endian,
	// since we use it quite often.
	d->timHeader.flags = le32_to_cpu(d->timHeader.flags);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Verify the TIM magic.
	// NOTE: File extension should also be checked, since the
	// magic number isn't unique enough.
	if (d->timHeader.nq_magic != cpu_to_le32(PS1_TIM_TAG) ||
	    (d->timHeader.flags & 0xFFFFFFF0) != 0)
	{
		// Incorrect magic.
		d->file.reset();
		return;
	}

	// If the CLUT header is present, read it.
	if (d->timHeader.flags & PS1_TIM_FLAG_CLP) {
		// We have a CLUT header.
		// Sanity check: bpp must be 4 or 8.
		uint32_t clut_size;
		switch (d->timHeader.flags & PS1_TIM_FLAG_BPP_MASK) {
			case PS1_TIM_FLAG_BPP_4BPP:
				clut_size = 16 * 2;
				break;
			case PS1_TIM_FLAG_BPP_8BPP:
				clut_size = 256 * 2;
				break;
			default:
				assert(!"CLUT is not applicable for this color depth! (15-bpp or 24-bpp)");
				d->file.reset();
				return;
		}

		// Read the CLUT header.
		size = d->file->read(&d->clutHeader, sizeof(d->clutHeader));
		if (size != sizeof(d->clutHeader)) {
			d->file.reset();
			return;
		}

		// Verify d->clutHeader.size.
		// Should be equal to sizeof(d->clutHeader), plus the 16-bit CLUT entries.
		// TODO: Determine the size based on CLUT width/height?
		// For now, hard-coding 16/256.
		const uint32_t clutHeader_size = le32_to_cpu(d->clutHeader.size);
		assert(clutHeader_size == sizeof(d->clutHeader) + clut_size);
		if (clutHeader_size != sizeof(d->clutHeader) + clut_size) {
			d->file.reset();
			return;
		}

		// Save the CLUT starting address for later.
		d->clutDataStartAddr = static_cast<uint32_t>(d->file->tell());

		// Skip over the CLUT to get to the image header.
		d->file->seek(clut_size, IRpFile::SeekWhence::Cur);
	}

	// Read the image header.
	size = d->file->read(&d->imageHeader, sizeof(d->imageHeader));
	if (size != sizeof(d->imageHeader)) {
		d->file.reset();
		return;
	}

	// NOTE: The framebuffer width is always in units of 16-bit pixels.
	// Convert it to an actual width here.
	int width = le32_to_cpu(d->imageHeader.fb.width);
	switch (d->timHeader.flags & PS1_TIM_FLAG_BPP_MASK) {
		case PS1_TIM_FLAG_BPP_4BPP:
			width *= 4;
			break;
		case PS1_TIM_FLAG_BPP_8BPP:
			width *= 2;
			break;
		case PS1_TIM_FLAG_BPP_15BPP:
			// stride == width
			break;
		case PS1_TIM_FLAG_BPP_24BPP:
			// Multiply by 2 to get bytes, then divide by 3 for 24-bit
			// and discard the remainder.
			width = ((width * 2) / 3);
			break;
		default:
			assert(!"Invalid BPP???");
			break;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	const int height = le32_to_cpu(d->imageHeader.fb.height);
	assert(width > 0);
	assert(height > 0);
	assert(width <= 32768);
	assert(height <= 32768);
	if (width <= 0 || height <= 0 || width > 32768 || height > 32768) {
		d->file.reset();
		return;
	}

	// TODO: Verify the image data size.

	// Texture data start address.
	d->texDataStartAddr = static_cast<uint32_t>(d->file->tell());

	// File is valid.
	d->isValid = true;

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = width;
	d->dimensions[1] = height;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *PlayStationTIM::pixelFormat(void) const
{
	RP_D(const PlayStationTIM);
	if (!d->isValid) {
		return nullptr;
	}

	// Using a simple string lookup table.
	// NOTE: Bit 2 isn't used, so splitting this into two tables
	// to avoid having 8 nullptr entries.
	static const char *const tim_format_tbl_noCLUT[] = {
		"CI4 (no CLUT)",
		"CI8 (no CLUT)",
		"BGR555_PS1",
		"BGR888",
	};

	static const char *const tim_format_tbl_CLUT[] = {
		"CI4",
		"CI8",
		nullptr,
		nullptr,
	};

	// TODO: Invalid pixel format for 15-bit/24-bit with CLUT?
	if (d->timHeader.flags & PS1_TIM_FLAG_CLP) {
		return tim_format_tbl_CLUT[d->timHeader.flags & PS1_TIM_FLAG_BPP_MASK];
	} else {
		return tim_format_tbl_noCLUT[d->timHeader.flags & PS1_TIM_FLAG_BPP_MASK];
	}
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int PlayStationTIM::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields) {
		return 0;
	}

	RP_D(const PlayStationTIM);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	//fields->reserve(initial_count + 1);	// Maximum of 1 field.

	// TODO: Add fields?

	// Finished reading the field data.
	return (fields->count() - initial_count);
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/** Image accessors **/

/**
 * Get the image.
 * For textures with mipmaps, this is the largest mipmap.
 * The image is owned by this object.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr PlayStationTIM::image(void) const
{
	RP_D(const PlayStationTIM);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<PlayStationTIMPrivate*>(d)->loadImage();
}

} // namespace LibRpTexture

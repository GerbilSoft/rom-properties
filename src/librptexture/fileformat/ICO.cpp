/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ICO.cpp: Windows icon and cursor image reader.                          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ICO.hpp"
#include "FileFormat_p.hpp"

#include "ico_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpbase/disc/DiscReader.hpp"
#include "librpbase/disc/PartitionFile.hpp"
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_Linear_Gray.hpp"

// C++ STL classes
using std::array;

// Uninitialized vector class
#include "uvector.h"

namespace LibRpTexture {

class ICOPrivate final : public FileFormatPrivate
{
public:
	ICOPrivate(ICO *q, const IRpFilePtr &file);
	ICOPrivate(ICO *q, const IResourceReaderPtr &resReader, uint16_t type, int id, int lang);
	~ICOPrivate();

private:
	typedef FileFormatPrivate super;
	RP_DISABLE_COPY(ICOPrivate)

public:
	/** TextureInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 9+1> mimeTypes;
	static const TextureInfo textureInfo;

public:
	// Icon type
	enum class IconType {
		Unknown	= -1,

		// Win1.x .ico/.cur
		Icon_Win1 = 0,
		Cursor_Win1 = 1,

		// Win3.x .ico/.cur
		Icon_Win3 = 2,
		Cursor_Win3 = 3,

		// Win1.x resources (RT_ICON, RT_CURSOR)
		IconRes_Win1 = 4,
		CursorRes_Win1 = 5,

		// Win3.x resources (RT_GROUP_ICON, RT_GROUP_CURSOR)
		IconRes_Win3 = 6,
		CursorRes_Win3 = 7,

		Max
	};
	IconType iconType;

	// ICO header
	union {
		ICO_Win1_Header win1;
		ICONDIR win3;		// ICONDIR and GRPICONDIR are essentially the same
	} icoHeader;

	/** Win3.x icon stuff **/

	/**
	 * Is this an icon resource from a .exe/.dll?
	 * @return True if it's a resource; false if it's a .ico file.
	 */
	inline bool isResource(void) const
	{
		return (iconType >= IconType::IconRes_Win1) && (iconType <= IconType::CursorRes_Win3);
	}

	/**
	 * Get the resource type for the individual icon or cursor.
	 * @return RT_ICON or RT_CURSOR, or 0 if this is a standalone .ico/.cur file.
	 */
	inline uint16_t imageResType(void) const
	{
		switch (iconType) {
			case IconType::IconRes_Win1:
			case IconType::IconRes_Win3:
				return RT_ICON;
			case IconType::CursorRes_Win1:
			case IconType::CursorRes_Win3:
				return RT_CURSOR;
			default:
				return 0;
		}
	}

	// NOTE: ICONDIRENTRY (for .ico files) and GRPICONDIRENTRY (for resources)
	// are different sizes. Hence, we have to use this union of struct pointers hack.
	struct icodir_ico {
		// Icon directory
		// NOTE: *Not* byteswapped.
		rp::uvector<ICONDIRENTRY> iconDirectory;
	};
	struct icodir_res {
		// Icon directory
		// NOTE: *Not* byteswapped.
		// NOTE: ICONDIRENTRY and GRPICONDIRENTRY are different sizes,
		// so this has to be interpreted based on IconType.
		rp::uvector<GRPICONDIRENTRY> iconDirectory;

		// IResourceReader for loading icons from Windows executables
		IResourceReaderPtr resReader;

		// Resource information
		uint16_t type;
		int id;
		int lang;

		icodir_res(const IResourceReaderPtr &resReader, uint16_t type, int id, int lang)
			: resReader(resReader)
			, type(type)
			, id(id)
			, lang(lang)
		{ }
	};

	struct dir_t {
		union {
			void *v;
			icodir_ico *ico;
			icodir_res *res;
		};

		// "Best" icon in the icon directory.
		// NOTE: This is an index into iconDirectory.
		// If -1, a "best" icon has not been determined.
		int bestIcon_idx;

		dir_t()
			: bestIcon_idx(-1)
		{ }
	};
	dir_t dir;

	// Icon bitmap header
	union IconBitmapHeader_t {
		uint32_t size;
		BITMAPCOREHEADER bch;
		BITMAPINFOHEADER bih;
		struct {
			uint8_t magic[8];
			PNG_IHDR_full_t ihdr;
		} png;
	};

	// All icon bitmap headers
	// These all have to be loaded in order to
	// determine which one is the "best" icon.
	rp::uvector<IconBitmapHeader_t> iconBitmapHeaders;

	// "Best" icon: Bitmap header
	const IconBitmapHeader_t *pIconHeader;

	// Decoded image
	rp_image_ptr img;

public:
	/**
	 * Load the icon directory. (Windows 3.x)
	 * This function will also select the "best" icon to use.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadIconDirectory_Win3(void);

private:
	/**
	 * Load the image. (Windows 1.0 icon format)
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_Win1(void);

	/**
	 * Load the image. (Windows 3.x icon format)
	 * @param idx Icon's bitmap index (-1 for "best")
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_Win3(int idx = -1);

	/**
	 * Load the image. (Windows Vista PNG format)
	 * @param idx Icon's bitmap index (-1 for "best")
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage_WinVista_PNG(int idx = -1);

public:
	/**
	 * Load the image.
	 * This loads the "best" bitmap from the icon.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);
};

FILEFORMAT_IMPL(ICO)

/** ICOPrivate **/

/* TextureInfo */
const array<const char*, 2+1> ICOPrivate::exts = {{
	".ico",
	".cur",

	nullptr
}};
const array<const char*, 9+1> ICOPrivate::mimeTypes = {{
	// Official MIME types.
	"image/vnd.microsoft.icon",

	// Unofficial MIME types.
	"application/ico",
	"image/ico",		// NOTE: Used by Microsoft
	"image/icon",
	"image/x-ico",
	"image/x-icon",		// NOTE: Used by Microsoft
	"text/ico",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/vnd.microsoft.cursor",
	"image/x-cursor",

	nullptr
}};
const TextureInfo ICOPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

ICOPrivate::ICOPrivate(ICO *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, iconType(IconType::Unknown)
	, pIconHeader(nullptr)
{
	// Clear the ICO header union.
	memset(&icoHeader, 0, sizeof(icoHeader));

	// Initialize the icon directory union.
	dir.ico = new icodir_ico();
}

ICOPrivate::ICOPrivate(ICO *q, const IResourceReaderPtr &resReader, uint16_t type, int id, int lang)
	: super(q, resReader, &textureInfo)
	, iconType(IconType::Unknown)
	, pIconHeader(nullptr)
{
	// Clear the ICO header union.
	memset(&icoHeader, 0, sizeof(icoHeader));

	// Determine the icon type here.
	switch (type) {
		default:
			assert(!"Unsupported resource type");
			dir.v = nullptr;
			return;

		// NOTE: Assuming individual icon/cursor is Windows 1.x/2.x format.
		// TODO: Check the header to verify?
		case RT_ICON:
			iconType = IconType::IconRes_Win1;
			break;
		case RT_CURSOR:
			iconType = IconType::CursorRes_Win1;
			break;

		case RT_GROUP_ICON:
			iconType = IconType::IconRes_Win3;
			break;
		case RT_GROUP_CURSOR:
			iconType = IconType::CursorRes_Win3;
			break;
	}

	// Initialize the icon directory union.
	dir.res = new icodir_res(resReader, type, id, lang);
}

ICOPrivate::~ICOPrivate()
{
	if (dir.v) {
		if (isResource()) {
			delete dir.res;
		} else {
			delete dir.ico;
		}
	}
}

/**
 * Load the icon directory. (Windows 3.x)
 * This function will also select the "best" icon to use.
 * @return 0 on success; negative POSIX error code on error.
 */
int ICOPrivate::loadIconDirectory_Win3(void)
{
	// TODO: Windows Vista uses BITMAPINFOHEADER to select an icon.
	// Don't remember the reference for this, probably The Old New Thing...

	// Load the icon directory.
	const unsigned int count = le16_to_cpu(icoHeader.win3.idCount);
	if (count == 0) {
		// No icons???
		return -ENOENT;
	}

	const uint16_t rt = imageResType();
	if (rt != 0) {
		// Icon/cursor resource from a Windows executable.

		// Open the RT_GROUP_ICON / RT_GROUP_CURSOR resource.
		auto &res = *(dir.res);
		IRpFilePtr f_icondir = res.resReader->open(res.type, res.id, res.lang);
		if (!f_icondir) {
			// Unable to open the RT_GROUP_ICON / RT_GROUP_CURSOR.
			return -ENOENT;
		}

		const size_t fullsize = count * sizeof(GRPICONDIRENTRY);
		auto &iconDirectory = res.iconDirectory;
		iconDirectory.resize(count);
		size_t size = f_icondir->seekAndRead(sizeof(GRPICONDIR), iconDirectory.data(), fullsize);
		if (size != fullsize) {
			// Seek and/or read error.
			iconDirectory.clear();
			return -EIO;
		}

		// Load all of the icon image headers.
		iconBitmapHeaders.resize(count);
		IconBitmapHeader_t *p = iconBitmapHeaders.data();
		for (auto iter = iconDirectory.cbegin(); iter != iconDirectory.cend(); ++iter, p++) {
			IRpFilePtr f_icon = res.resReader->open(rt, le16_to_cpu(iter->nID), res.lang);
			if (!f_icon) {
				// Unable to open the resource.
				iconDirectory.clear();
				iconBitmapHeaders.clear();
				return -ENOENT;
			}

			size_t size = f_icon->read(p, sizeof(*p));
			if (size != sizeof(*p)) {
				// Short read.
				iconDirectory.clear();
				iconBitmapHeaders.clear();
				return -EIO;
			}
		}
	} else {
		// Standalone .ico/.cur file.
		const size_t fullsize = count * sizeof(ICONDIRENTRY);
		auto &iconDirectory = dir.ico->iconDirectory;
		iconDirectory.resize(count);
		size_t size = file->seekAndRead(sizeof(ICONDIR), iconDirectory.data(), fullsize);
		if (size != fullsize) {
			// Seek and/or read error.
			iconDirectory.clear();
			return -EIO;
		}

		// Load all of the icon image headers.
		iconBitmapHeaders.resize(count);
		IconBitmapHeader_t *p = iconBitmapHeaders.data();
		for (auto iter = iconDirectory.cbegin(); iter != iconDirectory.cend(); ++iter, p++) {
			unsigned int addr = le32_to_cpu(iter->dwImageOffset);
			size_t size = file->seekAndRead(addr, p, sizeof(*p));
			if (size != sizeof(*p)) {
				// Seek and/or read error.
				iconDirectory.clear();
				iconBitmapHeaders.clear();
				return -EIO;
			}
		}
	}

	// Go through the icon bitmap headers and figure out the "best" one.
	unsigned int width_best = 0, height_best = 0, bitcount_best = 0;
	int bestIcon_idx = -1;
	for (unsigned int i = 0; i < count; i++) {
		// Get the width, height, and color depth from this bitmap header.
		const IconBitmapHeader_t *const p = &iconBitmapHeaders[i];
		unsigned int width, height, bitcount;

		switch (p->size) {
			default:
				// Not supported...
				continue;

			case BITMAPCOREHEADER_SIZE:
				if (le32_to_cpu(p->bch.bcPlanes) > 1) {
					// Cannot handle planar bitmaps.
					continue;
				}
				width = le16_to_cpu(p->bch.bcWidth);
				height = le16_to_cpu(p->bch.bcHeight) / 2;
				bitcount = le16_to_cpu(p->bch.bcBitCount);
				break;

			case BITMAPINFOHEADER_SIZE:
			case BITMAPV2INFOHEADER_SIZE:
			case BITMAPV3INFOHEADER_SIZE:
			case BITMAPV4HEADER_SIZE:
			case BITMAPV5HEADER_SIZE:
				if (le32_to_cpu(p->bih.biPlanes) > 1) {
					// Cannot handle planar bitmaps.
					continue;
				}
				width = le32_to_cpu(p->bih.biWidth);
				height = le32_to_cpu(p->bih.biHeight) / 2;
				bitcount = le16_to_cpu(p->bih.biBitCount);
				break;

			case 0x474E5089:	// "\x89PNG"
				switch (p->png.ihdr.data.color_type) {
					default:
						// Not supported...
						continue;

					case PNG_COLOR_TYPE_PALETTE:
						bitcount = p->png.ihdr.data.bit_depth;
						break;

					case PNG_COLOR_TYPE_GRAY:
					case PNG_COLOR_TYPE_RGB:
						// Handling as if it's RGB.
						bitcount = p->png.ihdr.data.bit_depth * 3;
						break;

					case PNG_COLOR_TYPE_GRAY_ALPHA:
					case PNG_COLOR_TYPE_RGB_ALPHA:
						// Handling as if it's ARGB.
						bitcount = p->png.ihdr.data.bit_depth * 4;
						break;
				}

				width = be32_to_cpu(p->png.ihdr.data.width);
				height = be32_to_cpu(p->png.ihdr.data.height);
				break;
		}

		// Check if the image is larger.
		// TODO: Non-square icon handling.
		bool icon_is_better = false;
		if (width > width_best || height > height_best) {
			// Image is larger.
			icon_is_better = true;
		} else if (width == width_best && height == height_best) {
			// Image is the same size.
			if (bitcount > bitcount_best) {
				// Color depth is higher.
				icon_is_better = true;
			}
		}

		if (icon_is_better) {
			// This icon is better.
			bestIcon_idx = static_cast<int>(i);
			pIconHeader = p;
			width_best = width;
			height_best = height;
			bitcount_best = bitcount;
		}
	}

	if (bestIcon_idx >= 0) {
		dir.bestIcon_idx = bestIcon_idx;
		return 0;
	}

	// No icons???
	return -ENOENT;
}

/**
 * Load the image. (Windows 1.0 icon format)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage_Win1(void)
{
	// Icon data is located immediately after the header.
	// Each icon is actually two icons: a 1bpp mask, then a 1bpp icon.

	// NOTE: If the file has *both* DIB and DDB, then the DIB is first,
	// followed by the DDB, with its own icon header. Not handling this
	// for now; only reading the DIB.

	const int width = le16_to_cpu(icoHeader.win1.width);
	const int height = le16_to_cpu(icoHeader.win1.height);
	const unsigned int stride = le16_to_cpu(icoHeader.win1.stride);

	// Single icon size
	const size_t icon_size = static_cast<size_t>(height) * stride;

	// Load the icon data.
	rp::uvector<uint8_t> icon_data;
	icon_data.resize(icon_size * 2);

	// Is this from a file or a resource?
	size_t size;
	const uint16_t rt = imageResType();
	if (rt != 0) {
		// Open the resource.
		const auto &res = *(dir.res);
		IRpFilePtr f_icon = res.resReader->open(rt, res.id, res.lang);
		if (!f_icon) {
			// Unable to open the resource.
			return {};
		}

		// Read from the resource.
		size = f_icon->seekAndRead(sizeof(icoHeader.win1), icon_data.data(), icon_size * 2);
	} else {
		// Read from the file.
		size = file->seekAndRead(sizeof(icoHeader.win1), icon_data.data(), icon_size * 2);
	}

	if (size != icon_size * 2) {
		// Seek and/or read error.
		return {};
	}

	// Convert the icon.
	const uint8_t *const p_mask_data = icon_data.data();
	const uint8_t *const p_icon_data = p_mask_data + icon_size;
	img = ImageDecoder::fromLinearMono_WinIcon(width, height,
		p_icon_data, icon_size, p_mask_data, icon_size, stride);
	return img;
}

/**
 * Load the image. (Windows 3.x icon format)
 * @param idx Icon's bitmap index (-1 for "best")
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage_Win3(int idx)
{
	// Icon image header was already loaded by loadIconDirectory_Win3().
	// TODO: Verify dwBytesInRes.

	// Check the header size.
	const unsigned int header_size = le32_to_cpu(pIconHeader->size);
	switch (header_size) {
		default:
			// Not supported...
			return {};

		case BITMAPCOREHEADER_SIZE:
			// TODO: Convert to BITMAPINFOHEADER.
			return {};

		case BITMAPINFOHEADER_SIZE:
		case BITMAPV2INFOHEADER_SIZE:
		case BITMAPV3INFOHEADER_SIZE:
		case BITMAPV4HEADER_SIZE:
		case BITMAPV5HEADER_SIZE:
			break;

		case 0x474E5089:	// "\x89PNG"
			// Load it as a PNG image.
			return loadImage_WinVista_PNG();
	}

	// NOTE: For standard icons (non-alpha, not PNG), the height is
	// actually doubled. The top of the bitmap is the icon image,
	// and the bottom is the monochrome mask.
	// NOTE 2: If height > 0, the entire bitmap is upside-down.

	const BITMAPINFOHEADER *bih = &pIconHeader->bih;

	// Make sure width and height are valid.
	// Height cannot be 0 or an odd number.
	// NOTE: Negative height is allowed for "right-side up".
	const unsigned int width = le32_to_cpu((unsigned int)(bih->biWidth));
	const int orig_height = static_cast<int>(le32_to_cpu(bih->biHeight));
	const unsigned int height = abs(orig_height);
	if (width <= 0 || height == 0 || (height & 1)) {
		// Invalid bitmap size.
		return {};
	}

	const bool is_upside_down = (orig_height > 0);
	const unsigned int half_height = height / 2;

	// Only supporting 16-color images for now.
	// TODO: Handle BI_BITFIELDS?
	if (le32_to_cpu(bih->biPlanes) > 1) {
		// Cannot handle planar bitmaps.
		return {};
	}

	// Row must be 32-bit aligned.
	// FIXME: Including for 24-bit images?
	const unsigned int bitcount = le32_to_cpu(bih->biBitCount);
	unsigned int stride = width;
	switch (bitcount) {
		default:
			// Unsupported bitcount.
			return {};
		case 1:
			stride /= 8;
			break;
		case 2:
			stride /= 4;
			break;
		case 4:
			stride /= 2;
			break;
		case 8:
			break;
		case 16:
			stride *= 2;
			break;
		case 24:
			stride *= 3;
			break;
		case 32:
			stride *= 4;
			break;
	}
	stride = ALIGN_BYTES(4, stride);

	// Mask row is 1bpp and must also be 32-bit aligned.
	unsigned int mask_stride = ALIGN_BYTES(4, width / 8);

	// Icon file (this->file for .ico; IResourceReader::open() for .exe/.dll)
	IRpFilePtr f_icon;
	unsigned int addr;

	const uint16_t rt = imageResType();
	if (rt != 0) {
		// Load the icon from a resource.
		const auto &res = *(dir.res);
		const GRPICONDIRENTRY *pBestIcon;
		if (idx < 0 && dir.bestIcon_idx >= 0) {
			pBestIcon = &res.iconDirectory[dir.bestIcon_idx];
		} else if (idx < static_cast<int>(res.iconDirectory.size())) {
			pBestIcon = &res.iconDirectory[idx];
		} else {
			// Invalid index.
			return {};
		}

		f_icon = res.resReader->open(rt, le16_to_cpu(pBestIcon->nID), res.lang);
		if (!f_icon) {
			// Unable to open the resource.
			return {};
		}
		addr = header_size;
	} else {
		// Get the icon's starting address within the .ico file.
		const auto &ico = *(dir.ico);
		const ICONDIRENTRY *pBestIcon;
		if (idx < 0 && dir.bestIcon_idx >= 0) {
			pBestIcon = &ico.iconDirectory[dir.bestIcon_idx];
		} else if (idx < static_cast<int>(ico.iconDirectory.size())) {
			pBestIcon = &ico.iconDirectory[idx];
		} else {
			// Invalid index.
			return {};
		}
		
		f_icon = this->file;
		addr = le32_to_cpu(pBestIcon->dwImageOffset) + header_size;
	}

	// For 8bpp or less, a color table is present.
	// NOTE: Need to manually set the alpha channel to 0xFF.
	rp::uvector<uint32_t> pal_data;
	if (bitcount <= 8) {
		const unsigned int palette_count = (1U << bitcount);
		const unsigned int palette_size = palette_count * static_cast<unsigned int>(sizeof(uint32_t));
		pal_data.resize(palette_count);
		size_t size = f_icon->seekAndRead(addr, pal_data.data(), palette_size);
		if (size != palette_size) {
			// Seek and/or read error.
			return {};
		}
		// TODO: 32-bit alignment?
		addr += palette_size;

		// Convert to host-endian and set the A channel to 0xFF.
		for (uint32_t &p : pal_data) {
			p = le32_to_cpu(p) | 0xFF000000U;
		}
	}

	// Calculate the icon, mask, and total image size.
	unsigned int icon_size = stride * half_height;
	unsigned int mask_size = mask_stride * half_height;

	size_t biSizeImage = icon_size + mask_size;
	rp::uvector<uint8_t> img_data;
	img_data.resize(biSizeImage);
	size_t size = f_icon->seekAndRead(addr, img_data.data(), biSizeImage);
	if (size != biSizeImage) {
		// Seek and/or read error.
		return {};
	}

	// Convert the main image first.
	// TODO: Apply the mask.
	const uint8_t *icon_data, *mask_data;
	if (is_upside_down) {
		icon_data = img_data.data();
		mask_data = icon_data + icon_size;
	} else {
		// TODO: Need to test this. Might not be correct.
		// I don't have any right-side up icons...
		mask_data = img_data.data();
		icon_data = mask_data + mask_size;
	}

	switch (bitcount) {
		default:
			// Not supported yet...
			assert(!"This Win3.x icon format is not supported yet!");
			return {};

		case 1:
			// 1bpp (monochrome)
			// NOTE: ImageDecoder::fromLinearMono_WinIcon() handles the mask.
			img = ImageDecoder::fromLinearMono_WinIcon(width, half_height,
				icon_data, icon_size,
				mask_data, mask_size, stride);
			break;

		case 4:
			// 16-color
			// NOTE: fromLinearCI4() doesn't support Host_xRGB32,
			// and we're setting the alpha channel to 0xFF ourselves.
			img = ImageDecoder::fromLinearCI4(ImageDecoder::PixelFormat::Host_ARGB32, true,
				width, half_height,
				icon_data, icon_size,
				pal_data.data(), pal_data.size() * sizeof(uint32_t),
				stride);
			break;

		case 8:
			// 256-color
			img = ImageDecoder::fromLinearCI8(ImageDecoder::PixelFormat::Host_xRGB32,
				width, half_height,
				icon_data, icon_size,
				pal_data.data(), pal_data.size() * sizeof(uint32_t),
				stride);
			break;

		case 32: {
			// 32-bit ARGB
			const uint32_t biCompression = le32_to_cpu(bih->biCompression);
			if (biCompression != BI_RGB) {
				// FIXME: BI_BITFIELDS is not supported right now.
				return {};
			}
			img = ImageDecoder::fromLinear32(ImageDecoder::PixelFormat::ARGB8888,
				width, half_height,
				reinterpret_cast<const uint32_t*>(icon_data), icon_size,
				stride);
			break;
		}
	}

	// Apply the icon mask.
	rp_image::sBIT_t sBIT;
	img->get_sBIT(&sBIT);
	if (bitcount == 1) {
		// Monochrome icons are handled by ImageDecoder::fromLinearMono_WinIcon().
	} else if (bitcount < 8) {
		// Keep the icon as CI8 and add a transparency color.
		// TODO: Set sBIT.
		assert(img->format() == rp_image::Format::CI8);
		uint8_t tr_idx = (1U << bitcount);
		img->palette()[tr_idx] = 0;
		img->set_tr_idx(tr_idx);

		uint8_t *bits = static_cast<uint8_t*>(img->bits());
		const uint8_t *mask = mask_data;
		int dest_stride_adj = img->stride() - width;
		int mask_stride_adj = mask_stride - ((width / 8) + (width % 8 > 0));
		for (unsigned int y = half_height; y > 0; y--) {
			// TODO: Mask stride adjustment?
			unsigned int mask_bits_remain = 8;
			uint8_t mask_byte = *mask++;

			for (unsigned int x = width; x > 0; x--, bits++, mask_byte <<= 1, mask_bits_remain--) {
				if (mask_bits_remain == 0) {
					// Get the next mask byte.
					mask_byte = *mask++;
					mask_bits_remain = 8;
				}

				if (mask_byte & 0x80) {
					// Mask the pixel.
					// TODO: For 1bpp, if the destination pixel is white, don't mask it.
					// This would be "invert" mode.
					*bits = tr_idx;
				}
			}

			// Next row.
			bits += dest_stride_adj;
			mask += mask_stride_adj;
		}

		// Update the sBIT metadata.
		if (sBIT.alpha == 0) {
			sBIT.alpha = 1;
			img->set_sBIT(&sBIT);
		}
	} else {
		// CI8 needs to be converted to ARGB32.
		if (img->format() != rp_image::Format::ARGB32) {
			rp_image_ptr convimg = img->dup_ARGB32();
			assert((bool)convimg);
			if (!convimg) {
				// Cannot convert the image for some reason...
				// Flip it if necessary and then give up.
				if (is_upside_down) {
					rp_image_ptr flipimg = img->flip(rp_image::FLIP_V);
					if (flipimg) {
						img = std::move(flipimg);
					}
				}
				return img;
			}
			img = std::move(convimg);
		}

		assert(img->format() == rp_image::Format::ARGB32);

		uint32_t *bits = static_cast<uint32_t*>(img->bits());
		const uint8_t *mask = mask_data;
		int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - width;
		int mask_stride_adj = mask_stride - ((width / 8) + (width % 8 > 0));
		for (unsigned int y = half_height; y > 0; y--) {
			// TODO: Mask stride adjustment?
			unsigned int mask_bits_remain = 8;
			uint8_t mask_byte = *mask++;

			for (unsigned int x = width; x > 0; x--, bits++, mask_byte <<= 1, mask_bits_remain--) {
				if (mask_bits_remain == 0) {
					// Get the next mask byte.
					mask_byte = *mask++;
					mask_bits_remain = 8;
				}

				if (mask_byte & 0x80) {
					// Make the pixel transparent.
					// NOTE: Complete transparency, without keeping the RGB.
					*bits = 0;
				}
			}

			// Next row.
			bits += dest_stride_adj;
			mask += mask_stride_adj;
		}

		// Update the sBIT metadata.
		if (sBIT.alpha == 0) {
			sBIT.alpha = 1;
			img->set_sBIT(&sBIT);
		}
	}

	// Flip the icon after the mask is processed.
	if (is_upside_down) {
		rp_image_ptr flipimg = img->flip(rp_image::FLIP_V);
		if (flipimg) {
			img = std::move(flipimg);
		}
	}

	return img;
}

/**
 * Load the image. (Windows Vista PNG format)
 * @param idx Icon's bitmap index (-1 for "best")
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage_WinVista_PNG(int idx)
{
	// Use RpPng to load a PNG image.
	IRpFilePtr f_png;

	const uint16_t rt = imageResType();
	if (rt != 0) {
		// Load the PNG from a resource.
		const auto &res = *(dir.res);
		const GRPICONDIRENTRY *pBestIcon;
		if (idx < 0 && dir.bestIcon_idx >= 0) {
			pBestIcon = &res.iconDirectory[dir.bestIcon_idx];
		} else if (idx < static_cast<int>(res.iconDirectory.size())) {
			pBestIcon = &res.iconDirectory[idx];
		} else {
			// Invalid index.
			return {};
		}

		f_png = res.resReader->open(rt, le16_to_cpu(pBestIcon->nID), res.lang);
	} else {
		// Get the PNG's starting address within the .ico file.
		const auto &ico = *(dir.ico);
		const ICONDIRENTRY *pBestIcon;
		if (idx < 0 && dir.bestIcon_idx >= 0) {
			pBestIcon = &ico.iconDirectory[dir.bestIcon_idx];
		} else if (idx < static_cast<int>(ico.iconDirectory.size())) {
			pBestIcon = &ico.iconDirectory[idx];
		} else {
			// Invalid index.
			return {};
		}

		// NOTE: PartitionFile only supports IDiscReader, so we'll need to
		// create a dummy DiscReader object.
		IDiscReaderPtr discReader = std::make_shared<DiscReader>(file, 0, file->size());
		f_png = std::make_shared<PartitionFile>(discReader, pBestIcon->dwImageOffset, pBestIcon->dwBytesInRes);
	}

	img = RpPng::load(f_png);
	return img;
}

/**
 * Load the image.
 * This loads the "best" bitmap from the icon.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ICOPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->isValid || !this->file) {
		// Can't load the image.
		return {};
	}

	switch (iconType) {
		default:
			// Not supported...
			return {};

		case IconType::Icon_Win1:
		case IconType::Cursor_Win1:
		case IconType::IconRes_Win1:
		case IconType::CursorRes_Win1:
			// Windows 1.0 icon or cursor
			return loadImage_Win1();

		case IconType::Icon_Win3:
		case IconType::Cursor_Win3:
		case IconType::IconRes_Win3:
		case IconType::CursorRes_Win3:
			// Windows 3.x icon or cursor
			return loadImage_Win3();
	}
}

/** ICO **/

/**
 * Read a Windows icon or cursor image file.
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
ICO::ICO(const IRpFilePtr &file)
	: super(new ICOPrivate(this, file))
{
	init(false);
}

/**
 * Read an icon or cursor from a Windows executable.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param resReader	[in] IResourceReader
 * @param type		[in] Resource type ID
 * @param id		[in] Resource ID (-1 for "first entry")
 * @param lang		[in] Language ID (-1 for "first entry")
 */
ICO::ICO(const LibRpBase::IResourceReaderPtr &resReader, uint16_t type, int id, int lang)
	: super(new ICOPrivate(this, resReader, type, id, lang))
{
	init(true);
}

/**
 * Common initialization function.
 * @param res True if the icon is in a Windows executable; false if not.
 */
void ICO::init(bool res)
{
	RP_D(ICO);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ICONDIR.
	if (res) {
		const auto &res = *(d->dir.res);
		IRpFilePtr f_icondir = res.resReader->open(res.type, res.id, res.lang);
		if (!f_icondir) {
			// Unable to open the icon or cursor.
			d->file.reset();
			delete d->dir.res;
			d->dir.res = nullptr;
			return;
		}

		size_t size = f_icondir->read(&d->icoHeader, sizeof(d->icoHeader));
		if (size != sizeof(d->icoHeader)) {
			d->file.reset();
			delete d->dir.res;
			d->dir.res = nullptr;
			return;
		}
	} else {
		d->file->rewind();
		size_t size = d->file->read(&d->icoHeader, sizeof(d->icoHeader));
		if (size != sizeof(d->icoHeader)) {
			d->file.reset();
			return;
		}
	}

	// Determine the icon type.
	// NOTE: d->iconType is already set if loading from a Windows resource.
	// Only set it if it's still ICOPrivate::IconType::Unknown.
	switch (le16_to_cpu(d->icoHeader.win1.format)) {
		default:
			// Not recognized...
			d->file.reset();
			if (res) {
				delete d->dir.res;
				d->dir.res = nullptr;
			}
			return;

		case ICO_WIN1_FORMAT_MAYBE_WIN3: {
			switch (le16_to_cpu(d->icoHeader.win3.idType)) {
				default:
					// Not recognized...
					d->file.reset();
					if (res) {
						delete d->dir.res;
						d->dir.res = nullptr;
					}
					return;
				case ICO_WIN3_TYPE_ICON:
					if (d->iconType == ICOPrivate::IconType::Unknown) {
						d->iconType = ICOPrivate::IconType::Icon_Win3;
					}
					d->mimeType = "image/vnd.microsoft.icon";
					d->textureFormatName = "Windows 3.x Icon";
					break;
				case ICO_WIN3_TYPE_CURSOR:
					if (d->iconType == ICOPrivate::IconType::Unknown) {
						d->iconType = ICOPrivate::IconType::Cursor_Win3;
					}
					d->mimeType = "image/vnd.microsoft.cursor";
					d->textureFormatName = "Windows 3.x Icon";
					break;
			}

			// Load the icon directory and select the best image.
			int ret = d->loadIconDirectory_Win3();
			if (ret != 0) {
				// Failed to load the icon directory.
				d->file.reset();
				if (res) {
					delete d->dir.res;
					d->dir.res = nullptr;
				}
				return;
			}
			break;
		}

		case ICO_WIN1_FORMAT_ICON_DIB:
		case ICO_WIN1_FORMAT_ICON_DDB:
		case ICO_WIN1_FORMAT_ICON_BOTH:
			if (d->iconType == ICOPrivate::IconType::Unknown) {
				d->iconType = ICOPrivate::IconType::Icon_Win1;
			}
			// TODO: Different MIME type for Windows 1.x?
			d->mimeType = "image/vnd.microsoft.icon";
			d->textureFormatName = "Windows 1.x Icon";
			break;

		case ICO_WIN1_FORMAT_CURSOR_DIB:
		case ICO_WIN1_FORMAT_CURSOR_DDB:
		case ICO_WIN1_FORMAT_CURSOR_BOTH:
			if (d->iconType == ICOPrivate::IconType::Unknown) {
				d->iconType = ICOPrivate::IconType::Cursor_Win1;
			}
			// TODO: Different MIME type for Windows 1.x?
			d->mimeType = "image/vnd.microsoft.cursor";
			d->textureFormatName = "Windows 1.x Cursor";
			break;
	}

	// Cache the dimensions for the FileFormat base class.
	switch (d->iconType) {
		default:
			// Shouldn't get here...
			assert(!"Invalid case!");
			d->file.reset();
			if (res) {
				delete d->dir.res;
				d->dir.res = nullptr;
			}
			return;

		case ICOPrivate::IconType::Icon_Win1:
		case ICOPrivate::IconType::Cursor_Win1:
		case ICOPrivate::IconType::IconRes_Win1:
		case ICOPrivate::IconType::CursorRes_Win1:
			d->dimensions[0] = le16_to_cpu(d->icoHeader.win1.width);
			d->dimensions[1] = le16_to_cpu(d->icoHeader.win1.height);
			break;

		case ICOPrivate::IconType::Icon_Win3:
		case ICOPrivate::IconType::Cursor_Win3:
		case ICOPrivate::IconType::IconRes_Win3:
		case ICOPrivate::IconType::CursorRes_Win3:
			if (d->dir.bestIcon_idx < 0) {
				// No "best" icon...
				d->file.reset();
				if (res) {
					delete d->dir.res;
					d->dir.res = nullptr;
				}
				return;
			}

			const ICOPrivate::IconBitmapHeader_t *const pIconHeader = d->pIconHeader;
			switch (pIconHeader->size) {
				default:
					// Not supported...
					d->file.reset();
					if (res) {
						delete d->dir.res;
						d->dir.res = nullptr;
					}
					return;

				case BITMAPCOREHEADER_SIZE:
					d->dimensions[0] = le16_to_cpu(pIconHeader->bch.bcWidth);
					d->dimensions[1] = le16_to_cpu(pIconHeader->bch.bcHeight) / 2;
					break;

				case BITMAPINFOHEADER_SIZE:
				case BITMAPV2INFOHEADER_SIZE:
				case BITMAPV3INFOHEADER_SIZE:
				case BITMAPV4HEADER_SIZE:
				case BITMAPV5HEADER_SIZE:
					d->dimensions[0] = le32_to_cpu(pIconHeader->bih.biWidth);
					d->dimensions[1] = le32_to_cpu(pIconHeader->bih.biHeight) / 2;
					break;

				case 0x474E5089:	// "\x89PNG"
					// TODO: Verify more IHDR fields?
					d->dimensions[0] = be32_to_cpu(pIconHeader->png.ihdr.data.width);
					d->dimensions[1] = be32_to_cpu(pIconHeader->png.ihdr.data.height);
					break;
			}
			break;
	}

	// File is valid.
	d->isValid = true;
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *ICO::pixelFormat(void) const
{
	RP_D(const ICO);
	if (!d->isValid) {
		return nullptr;
	}

	switch (d->iconType) {
		default:
			assert(!"Invalid icon type?");
			return nullptr;

		case ICOPrivate::IconType::Icon_Win1:
		case ICOPrivate::IconType::Cursor_Win1:
			// Windows 1.x only supports monochrome.
			return "1bpp";

		case ICOPrivate::IconType::Icon_Win3:
		case ICOPrivate::IconType::Cursor_Win3:
			// Check what the "best" icon is.
			// TODO: Check BITMAPCOREHEADER, BITMAPINFOHEADER, or the PNG header.
			return "Win3.x (TODO)";
	}
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int ICO::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const ICO);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 1);	// Maximum of 1 field.

	// TODO: ICO/CUR fields?
	// and "color" for Win1.x cursors

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
rp_image_const_ptr ICO::image(void) const
{
	RP_D(const ICO);
	if (!d->isValid) {
		// Unknown file type.
		return {};
	}

	// Load the image.
	return const_cast<ICOPrivate*>(d)->loadImage();
}

} // namespace LibRpTexture

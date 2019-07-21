/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iQueN64.cpp: iQue Nintendo 64 .cmd reader.                              *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"

#include "iQueN64.hpp"
#include "librpbase/RomData_p.hpp"

#include "ique_n64_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"

#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/ImageDecoder.hpp"

#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::vector;

// zlib
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
# include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

ROMDATA_IMPL(iQueN64)
ROMDATA_IMPL_IMG(iQueN64)

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(zlibVersion);
#endif /* _MSC_VER */

class iQueN64Private : public RomDataPrivate
{
	public:
		iQueN64Private(iQueN64 *q, IRpFile *file);
		virtual ~iQueN64Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(iQueN64Private)

	public:
		// .cmd structs.
		iQueN64_contentDesc contentDesc;
		iQueN64_BbContentMetaDataHead bbContentMetaDataHead;

		// Internal images.
		rp_image *img_thumbnail;

	public:
		/**
		 * Get the ROM title and ISBN.
		 * @param title Output string for title.
		 * @param isbn Output string for ISBN.
		 * @return 0 on success; non-zero on error.
		 */
		int getTitleAndISBN(string &title, string &isbn);

	public:
		/**
		 * Load the thumbnail image.
		 * @return Thumbnail image, or nullptr on error.
		 */
		const rp_image *loadThumbnailImage(void);
};

/** iQueN64Private **/

iQueN64Private::iQueN64Private(iQueN64 *q, IRpFile *file)
	: super(q, file)
	, img_thumbnail(nullptr)
{
	// Clear the .cmd structs.
	memset(&contentDesc, 0, sizeof(contentDesc));
	memset(&bbContentMetaDataHead, 0, sizeof(bbContentMetaDataHead));
}

iQueN64Private::~iQueN64Private()
{
	delete img_thumbnail;
}

/**
 * Get the ROM title and ISBN.
 * @param title Output string for title.
 * @param isbn Output string for ISBN.
 * @return 0 on success; non-zero on error.
 */
int iQueN64Private::getTitleAndISBN(string &title, string &isbn)
{
	// Stored immediately after the thumbnail and title images,
	// and NULL-terminated.
	static const size_t title_buf_sz = IQUEN64_BBCONTENTMETADATAHEAD_ADDRESS - sizeof(contentDesc);
	std::unique_ptr<char[]> title_buf(new char[title_buf_sz]);

	const int64_t title_addr = sizeof(contentDesc) +
		be16_to_cpu(contentDesc.thumb_image_size) +
		be16_to_cpu(contentDesc.title_image_size);
	if (title_addr >= (int64_t)title_buf_sz) {
		// Out of range.
		return 1;
	}

	const size_t title_sz = title_buf_sz - title_addr;
	size_t size = file->seekAndRead(title_addr, title_buf.get(), title_sz);
	if (size != title_sz) {
		// Seek and/or read error.
		return 2;
	}

	// Data read.
	title.clear();
	isbn.clear();

	// Find the first NULL.
	// This will give us the title.
	const char *p = title_buf.get();
	const char *p_end = static_cast<const char*>(memchr(p, '\0', title_buf_sz));
	if (p_end && p_end > p) {
		// Convert from GB2312 to UTF-8.
		title = cpN_to_utf8(CP_GB2312, p, p_end - p);
	}

	// Find the second NULL.
	// This will give us the ISBN.
	// NOTE: May be ASCII, but we'll decode as GB2312 just in case.
	const size_t isbn_buf_sz = title_buf_sz - (p_end - p) - 1;
	p = p_end + 1;
	p_end = static_cast<const char*>(memchr(p, '\0', isbn_buf_sz));
	if (p_end && p_end > p) {
		// Convert from GB2312 to UTF-8.
		isbn = cpN_to_utf8(CP_GB2312, p, p_end - p);
	}

	return 0;
}

/**
 * Load the thumbnail image.
 * @return Thumbnail image, or nullptr on error.
 */
const rp_image *iQueN64Private::loadThumbnailImage(void)
{
	if (img_thumbnail) {
		// Thumbnail is already loaded.
		return img_thumbnail;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Get the thumbnail address and size.
	static const int64_t thumb_addr = sizeof(contentDesc);
	const size_t z_thumb_size = be16_to_cpu(contentDesc.thumb_image_size);
	if (z_thumb_size > 0x4000) {
		// Out of range.
		return nullptr;
	}

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlibVersion() != 0) {
		// Delay load failed.
		// Can't decompress the thumbnail image.
		return nullptr;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Read the compressed thumbnail image.
	std::unique_ptr<uint8_t[]> z_thumb_buf(new uint8_t[z_thumb_size]);
	size_t size = file->seekAndRead(thumb_addr, z_thumb_buf.get(), z_thumb_size);
	if (size != z_thumb_size) {
		// Seek and/or read error.
		return nullptr;
	}

	// Decompress the thumbnail image.
	// Decompressed size must be 0x1880 bytes. (56*56*2)
	auto thumb_buf = aligned_uptr<uint16_t>(16, IQUEN64_THUMB_SIZE/2);

	// Initialize zlib.
	// Reference: https://zlib.net/zlib_how.html
	// NOTE: Raw deflate is used, so we need to specify -15.
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = 0;
	int ret = inflateInit2(&strm, -15);
	if (ret != Z_OK) {
		// Error initializing zlib.
		return nullptr;
	}

	strm.avail_in = static_cast<uInt>(z_thumb_size);
	strm.next_in = z_thumb_buf.get();

	strm.avail_out = IQUEN64_THUMB_SIZE;
	strm.next_out = reinterpret_cast<Bytef*>(thumb_buf.get());

	ret = inflate(&strm, Z_FINISH);
	inflateEnd(&strm);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		// Error decompressing.
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the image first.
	// TODO: Integrate this into image decoding?
	__byte_swap_16_array(thumb_buf.get(), IQUEN64_THUMB_SIZE);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Convert the thumbnail image from RGBA5551.
	img_thumbnail = ImageDecoder::fromLinear16(ImageDecoder::PXF_RGBA5551,
		IQUEN64_THUMB_W, IQUEN64_THUMB_H,
		thumb_buf.get(), IQUEN64_THUMB_SIZE);
	return img_thumbnail;
}

/** iQueN64 **/

/**
 * Read an iQue N64 .cmd file.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
iQueN64::iQueN64(IRpFile *file)
	: super(new iQueN64Private(this, file))
{
	RP_D(iQueN64);
	d->className = "iQueN64";
	d->fileType = FTYPE_METADATA_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Check the filesize.
	if (file->size() != IQUEN64_CMD_FILESIZE) {
		// Incorrect filesize.
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Read the content description.
	d->file->rewind();
	size_t size = d->file->read(&d->contentDesc, sizeof(d->contentDesc));
	if (size != sizeof(d->contentDesc)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this .cmd file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->contentDesc);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->contentDesc);
	info.ext = nullptr;	// Not needed for iQueN64.
	info.szFile = IQUEN64_CMD_FILESIZE;
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Read the bbContentMetaDataHead.
	size = d->file->seekAndRead(IQUEN64_BBCONTENTMETADATAHEAD_ADDRESS,
		&d->bbContentMetaDataHead, sizeof(d->bbContentMetaDataHead));
	if (size != sizeof(d->bbContentMetaDataHead)) {
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int iQueN64::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(iQueN64_contentDesc))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	if (info->szFile != IQUEN64_CMD_FILESIZE) {
		// Incorrect filesize.
		return -1;
	}

	const iQueN64_contentDesc *const contentDesc =
		reinterpret_cast<const iQueN64_contentDesc*>(info->header.pData);

	// Check the magic number.
	// NOTE: This technically isn't a "magic number",
	// but it appears to be the same for all iQue .cmd files.
	if (!memcmp(contentDesc->magic, IQUEN64_MAGIC, sizeof(contentDesc->magic))) {
		// Magic number matches.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *iQueN64::systemName(unsigned int type) const
{
	RP_D(const iQueN64);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// iQue was only released in China, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"iQueN64::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"iQue", "iQue", "iQue", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *iQueN64::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".cmd",		// NOTE: Conflicts with Windows NT batch files.
		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *iQueN64::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-ique-cmd",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t iQueN64::supportedImageTypes_static(void)
{
	// TODO: IMGBF_INT_TITLE?
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 *
 * The first item in the returned vector is the "default" size.
 * If the width/height is 0, then an image exists, but the size is unknown.
 *
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> iQueN64::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return vector<ImageSizeDef>();
	}

	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, IQUEN64_THUMB_W, IQUEN64_THUMB_H, 0},
	};
	return vector<ImageSizeDef>(sz_INT_ICON,
		sz_INT_ICON + ARRAY_SIZE(sz_INT_ICON));
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t iQueN64::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	if (imageType == IMG_INT_ICON) {
		// Use nearest-neighbor scaling.
		return IMGPF_RESCALE_NEAREST;
	}

	// Nothing else is supported.
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int iQueN64::loadFieldData(void)
{
	RP_D(iQueN64);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	const iQueN64_BbContentMetaDataHead *const bbContentMetaDataHead = &d->bbContentMetaDataHead;
	d->fields->reserve(4);	// Maximum of 4 fields. (TODO: Add more.)

	// Get the title and ISBN.
	string rom_title, rom_isbn;
	int ret = d->getTitleAndISBN(rom_title, rom_isbn);
	if (ret == 0) {
		// Title.
		d->fields->addField_string(C_("RomData", "Title"), rom_title);

		// ISBN.
		d->fields->addField_string(C_("RomData", "ISBN"), rom_isbn);
	}

	// Content ID.
	// NOTE: We don't want the "0x" prefix.
	// This is sort of like Wii title IDs, but only the
	// title ID low portion.
	d->fields->addField_string(C_("iQueN64", "Content ID"),
		rp_sprintf("%08X", be32_to_cpu(bbContentMetaDataHead->content_id)),
		RomFields::STRF_MONOSPACE);

	// Hardware access rights.
	// TODO: Localization?
	static const char *const hw_access_names[] = {
		"PI Buffer", "NAND Flash", "Memory Mapper",
		"AES Engine", "New PI DMA", "GPIO",
		"External I/O", "New PI Errors", "USB",
		"SK Stack RAM"
	};
	vector<string> *const v_hw_access_names = RomFields::strArrayToVector(
		hw_access_names, ARRAY_SIZE(hw_access_names));
	
	d->fields->addField_bitfield(C_("iQueN64", "HW Access"),
		v_hw_access_names, 3, be32_to_cpu(bbContentMetaDataHead->hwAccessRights));

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int iQueN64::loadMetaData(void)
{
	RP_D(iQueN64);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Get the title and ISBN.
	string rom_title, rom_isbn;
	int ret = d->getTitleAndISBN(rom_title, rom_isbn);
	if (ret == 0) {
		// Title.
		d->metaData->addMetaData_string(Property::Title, rom_title);

		// TODO: ISBN.
		//d->metaData->addMetaData_string(Property::ISBN, rom_isbn);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int iQueN64::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(iQueN64);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by iQueN64.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->img_thumbnail) {
		// Image has already been loaded.
		*pImage = d->img_thumbnail;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the icon.
	*pImage = d->loadThumbnailImage();
	return (*pImage != nullptr ? 0 : -EIO);
}

}

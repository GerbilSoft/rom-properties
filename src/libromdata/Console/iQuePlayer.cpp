/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iQuePlayer.cpp: iQue Player .cmd reader.                                *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "iQuePlayer.hpp"
#include "ique_player_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// for memmem() if it's not available in <string.h>
#include "librptext/libc.h"

// C++ STL classes
using std::string;
using std::vector;

// zlib
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
# include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class iQuePlayerPrivate final : public RomDataPrivate
{
public:
	iQuePlayerPrivate(const IRpFilePtr &file);
	~iQuePlayerPrivate() final = default;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(iQuePlayerPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// File type
	enum class IQueFileType {
		Unknown	= -1,	// Unknown ROM type.

		CMD	= 0,	// .cmd file (content metadata)
		DAT	= 1,	// .dat file (ticket)

		Max
	};
	IQueFileType iQueFileType;

	// .cmd structs
	iQuePlayer_contentDesc contentDesc;
	iQuePlayer_BbContentMetaDataHead bbContentMetaDataHead;
	iQuePlayer_BbTicketHead bbTicketHead;

	// Internal images
	rp_image_ptr img_thumbnail;	// handled as icon
	rp_image_ptr img_title;		// handled as banner

public:
	/**
	 * Get the ROM title and ISBN.
	 * @param title Output string for title.
	 * @param isbn Output string for ISBN.
	 * @return 0 on success; non-zero on error.
	 */
	int getTitleAndISBN(string &title, string &isbn);

private:
	/**
	 * Load an image. (internal function)
	 * @param address	[in] Starting address.
	 * @param z_size	[in] Compressed size.
	 * @param unz_size	[in] Expected decompressed size.
	 * @param px_format	[in] 16-bit pixel format.
	 * @param w		[in] Image width.
	 * @param h		[in] Image height.
	 * @param byteswap	[in] If true, byteswap before decoding if needed.
	 * @return Image, or nullptr on error.
	 */
	rp_image_ptr loadImage(off64_t address, size_t z_size, size_t unz_size,
		ImageDecoder::PixelFormat px_format, int w, int h, bool byteswap);

public:
	/**
	 * Load the thumbnail image.
	 * @return Thumbnail image, or nullptr on error.
	 */
	rp_image_const_ptr loadThumbnailImage(void);

	/**
	 * Load the title image.
	 * This is the game title in Chinese.
	 * @return Title image, or nullptr on error.
	 */
	rp_image_const_ptr loadTitleImage(void);
};

ROMDATA_IMPL(iQuePlayer)
ROMDATA_IMPL_IMG(iQuePlayer)

/** iQuePlayerPrivate **/

/* RomDataInfo */
const char *const iQuePlayerPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.

	".cmd",		// NOTE: Conflicts with Windows NT batch files.
	".dat",		// NOTE: Conflicts with lots of files.

	nullptr
};
const char *const iQuePlayerPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-ique-cmd",
	"application/x-ique-dat",

	nullptr
};
const RomDataInfo iQuePlayerPrivate::romDataInfo = {
	"iQuePlayer", exts, mimeTypes
};

iQuePlayerPrivate::iQuePlayerPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, iQueFileType(IQueFileType::Unknown)
	, img_thumbnail(nullptr)
	, img_title(nullptr)
{
	// Clear the .cmd structs.
	memset(&contentDesc, 0, sizeof(contentDesc));
	memset(&bbContentMetaDataHead, 0, sizeof(bbContentMetaDataHead));
	memset(&bbTicketHead, 0, sizeof(bbTicketHead));
}

/**
 * Get the ROM title and ISBN.
 * @param title Output string for title.
 * @param isbn Output string for ISBN.
 * @return 0 on success; non-zero on error.
 */
int iQuePlayerPrivate::getTitleAndISBN(string &title, string &isbn)
{
	// Stored immediately after the thumbnail and title images,
	// and NULL-terminated.
	static const size_t title_buf_sz = IQUE_PLAYER_BBCONTENTMETADATAHEAD_ADDRESS - sizeof(contentDesc);
	std::unique_ptr<char[]> title_buf(new char[title_buf_sz]);

	const unsigned int title_addr = sizeof(contentDesc) +
		be16_to_cpu(contentDesc.thumb_image_size) +
		be16_to_cpu(contentDesc.title_image_size);
	if (title_addr >= title_buf_sz) {
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

	/// Find the title. (first string)

	// Check for "\xEF\xBB\xBF" (UTF-8 BOM).
	// Title 00201b2c (Dongwu Senlin) uses this separator instead
	// of a NULL character for some reason.
	static const char utf8bom[] = "\xEF\xBB\xBF";
	const char *p = title_buf.get();
	const char *p_end = static_cast<const char*>(memmem(p, title_buf_sz, utf8bom, sizeof(utf8bom)-1));
	if (p_end && p_end > p) {
		// Found the UTF-8 BOM.
		// Convert from GB2312 to UTF-8.
		title = cpN_to_utf8(CP_GB2312, p, static_cast<int>(p_end - p));

		// Adjust p to point to the next string.
		p = p_end + 3;
	} else {
		// No UTF-8 BOM. Check for NULL instead.
		p_end = static_cast<const char*>(memchr(p, '\0', title_buf_sz));
		if (p_end && p_end > p) {
			// Convert from GB2312 to UTF-8.
			title = cpN_to_utf8(CP_GB2312, p, static_cast<int>(p_end - p));

			// Adjust p to point to the next string.
			p = p_end + 1;
		}
	}

	if (!p_end) {
		// No NULL found.
		// The description is invalid.
		return 3;
	}

	// Find the second NULL.
	// This will give us the ISBN. (ASCII)
	const size_t isbn_buf_sz = title_buf_sz - (p_end - p) - 1;
	p_end = static_cast<const char*>(memchr(p, '\0', isbn_buf_sz));
	if (p_end && p_end > p) {
		// Convert from ASCII (well, Latin-1) to UTF-8.
		isbn = latin1_to_utf8(p, static_cast<int>(p_end - p));
	}

	// TODO: There might be other fields with NULL or UTF-8 BOM separators.
	// Check 00201b2c.cmd for more information.
	return 0;
}

/**
 * Load an image. (internal function)
 * @param address	[in] Starting address.
 * @param z_size	[in] Compressed size.
 * @param unz_size	[in] Expected decompressed size.
 * @param px_format	[in] 16-bit pixel format.
 * @param w		[in] Image width.
 * @param h		[in] Image height.
 * @param byteswap	[in] If true, byteswap before decoding if needed.
 * @return Image, or nullptr on error.
 */
rp_image_ptr iQuePlayerPrivate::loadImage(off64_t address, size_t z_size, size_t unz_size,
	ImageDecoder::PixelFormat px_format, int w, int h, bool byteswap)
{
	assert(address >= static_cast<off64_t>(sizeof(contentDesc)));
	assert(z_size != 0);
	assert(unz_size > z_size);
	assert(unz_size == static_cast<size_t>(w * h * 2));

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Can't decompress the thumbnail image.
		return nullptr;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Read the compressed thumbnail image.
	std::unique_ptr<uint8_t[]> z_buf(new uint8_t[z_size]);
	size_t size = file->seekAndRead(address, z_buf.get(), z_size);
	if (size != z_size) {
		// Seek and/or read error.
		return nullptr;
	}

	// Decompress the thumbnail image.
	// Decompressed size must be 0x1880 bytes. (56*56*2)
	auto img_buf = aligned_uptr<uint16_t>(16, unz_size/2);

	// Initialize zlib.
	// Reference: https://zlib.net/zlib_how.html
	// NOTE: Raw deflate is used, so we need to specify -15.
	z_stream strm = { };
	int ret = inflateInit2(&strm, -15);
	if (ret != Z_OK) {
		// Error initializing zlib.
		return nullptr;
	}

	strm.avail_in = static_cast<uInt>(z_size);
	strm.next_in = z_buf.get();

	strm.avail_out = static_cast<uInt>(unz_size);
	strm.next_out = reinterpret_cast<Bytef*>(img_buf.get());

	ret = inflate(&strm, Z_FINISH);
	inflateEnd(&strm);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		// Error decompressing.
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	if (byteswap) {
		// Byteswap the image first.
		// TODO: Integrate this into image decoding?
		rp_byte_swap_16_array(img_buf.get(), unz_size);
	}
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Convert the image.
	return ImageDecoder::fromLinear16(px_format,
		w, h, img_buf.get(), unz_size);
}

/**
 * Load the thumbnail image.
 * @return Thumbnail image, or nullptr on error.
 */
rp_image_const_ptr iQuePlayerPrivate::loadThumbnailImage(void)
{
	if (img_thumbnail) {
		// Thumbnail is already loaded.
		return img_thumbnail;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Get the thumbnail address and size.
	static const off64_t thumb_addr = sizeof(contentDesc);
	const size_t z_thumb_size = be16_to_cpu(contentDesc.thumb_image_size);
	if (z_thumb_size > 0x4000) {
		// Out of range.
		return nullptr;
	}

	// Load the image.
	img_thumbnail = loadImage(thumb_addr, z_thumb_size, IQUE_PLAYER_THUMB_SIZE,
		ImageDecoder::PixelFormat::RGBA5551,
		IQUE_PLAYER_THUMB_W, IQUE_PLAYER_THUMB_H, true);
	return img_thumbnail;
}

/**
 * Load the title image.
 * This is the game title in Chinese.
 * @return Title image, or nullptr on error.
 */
rp_image_const_ptr iQuePlayerPrivate::loadTitleImage(void)
{
	if (img_title) {
		// Title is already loaded.
		return img_title;
	} else if (!this->file || !this->isValid) {
		// Can't load the banner.
		return nullptr;
	}

	// Get the thumbnail address and size.
	const off64_t title_addr = sizeof(contentDesc) + be16_to_cpu(contentDesc.thumb_image_size);
	const size_t z_title_size = be16_to_cpu(contentDesc.title_image_size);
	if (z_title_size > 0x10000) {
		// Out of range.
		return nullptr;
	}

	// Load the image.
	// NOTE: Using A8L8 format, not IA8, which is GameCube-specific.
	// TODO: Add ImageDecoder::fromLinear16() support for IA8 later.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	img_title = loadImage(title_addr, z_title_size, IQUE_PLAYER_TITLE_SIZE,
		ImageDecoder::PixelFormat::A8L8,
		IQUE_PLAYER_TITLE_W, IQUE_PLAYER_TITLE_H, false);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	img_title = loadImage(title_addr, z_title_size, IQUE_PLAYER_TITLE_SIZE,
		ImageDecoder::PixelFormat::L8A8,
		IQUE_PLAYER_TITLE_W, IQUE_PLAYER_TITLE_H, false);
#endif
	return img_title;
}

/** iQuePlayer **/

/**
 * Read an iQue Player .cmd file.
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
iQuePlayer::iQuePlayer(const IRpFilePtr &file)
	: super(new iQuePlayerPrivate(file))
{
	RP_D(iQuePlayer);
	d->fileType = FileType::MetadataFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Check the filesize.
	// TODO: Identify CMD vs. Ticket and display ticket-specific information?
	const off64_t fileSize = file->size();
	if (fileSize != IQUE_PLAYER_CMD_FILESIZE &&
	    fileSize != IQUE_PLAYER_DAT_FILESIZE)
	{
		// Incorrect filesize.
		d->file.reset();
		return;
	}

	// Read the content description.
	d->file->rewind();
	size_t size = d->file->read(&d->contentDesc, sizeof(d->contentDesc));
	if (size != sizeof(d->contentDesc)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->contentDesc), reinterpret_cast<const uint8_t*>(&d->contentDesc)},
		nullptr,	// ext (not needed for iQuePlayer)
		fileSize	// szFile
	};
	d->iQueFileType = static_cast<iQuePlayerPrivate::IQueFileType>(isRomSupported_static(&info));
	d->isValid = ((int)d->iQueFileType >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Read the BBContentMetaDataHead.
	size = d->file->seekAndRead(IQUE_PLAYER_BBCONTENTMETADATAHEAD_ADDRESS,
		&d->bbContentMetaDataHead, sizeof(d->bbContentMetaDataHead));
	if (size != sizeof(d->bbContentMetaDataHead)) {
		d->iQueFileType = iQuePlayerPrivate::IQueFileType::Unknown;
		d->isValid = false;
		d->file.reset();
		return;
	}

	// If this is a ticket, read the BBTicketHead.
	if (d->iQueFileType == iQuePlayerPrivate::IQueFileType::DAT) {
		d->mimeType = "application/x-ique-dat";		// unofficial, not on fd.o
		size = d->file->seekAndRead(IQUE_PLAYER_BBTICKETHEAD_ADDRESS,
			&d->bbTicketHead, sizeof(d->bbTicketHead));
		if (size != sizeof(d->bbTicketHead)) {
			// Unable to read the ticket header.
			// Handle it as a content metadata file.
			d->iQueFileType = iQuePlayerPrivate::IQueFileType::CMD;
		}
	} else {
		d->mimeType = "application/x-ique-cmd";		// unofficial, not on fd.o
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int iQuePlayer::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(iQuePlayer_contentDesc))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(iQuePlayerPrivate::IQueFileType::Unknown);
	}

	if (info->szFile != IQUE_PLAYER_CMD_FILESIZE &&
	    info->szFile != IQUE_PLAYER_DAT_FILESIZE)
	{
		// Incorrect filesize.
		return static_cast<int>(iQuePlayerPrivate::IQueFileType::Unknown);
	}

	const iQuePlayer_contentDesc *const contentDesc =
		reinterpret_cast<const iQuePlayer_contentDesc*>(info->header.pData);

	// Check the magic number.
	// NOTE: This technically isn't a "magic number",
	// but it appears to be the same for all iQue .cmd files.
	iQuePlayerPrivate::IQueFileType iQueFileType = iQuePlayerPrivate::IQueFileType::Unknown;
	if (!memcmp(contentDesc->magic, IQUE_PLAYER_MAGIC, sizeof(contentDesc->magic))) {
		// Magic number matches.
		if (info->szFile == IQUE_PLAYER_DAT_FILESIZE) {
			iQueFileType = iQuePlayerPrivate::IQueFileType::DAT;
		} else /*if (info->szFile == IQUE_PLAYER_CMD_FILESIZE)*/ {
			iQueFileType = iQuePlayerPrivate::IQueFileType::CMD;
		}
	}

	return (int)iQueFileType;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *iQuePlayer::systemName(unsigned int type) const
{
	RP_D(const iQuePlayer);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// iQue was only released in China, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"iQuePlayer::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"iQue Player", "iQue Player", "iQue", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t iQuePlayer::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON | IMGBF_INT_BANNER;
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
vector<RomData::ImageSizeDef> iQuePlayer::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_INT_ICON:
			// Icon (thumbnail)
			return {{nullptr, IQUE_PLAYER_THUMB_W, IQUE_PLAYER_THUMB_H, 0}};
		case IMG_INT_BANNER:
			// Banner (title)
			return {{nullptr, IQUE_PLAYER_TITLE_W, IQUE_PLAYER_TITLE_H, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
uint32_t iQuePlayer::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
		case IMG_INT_BANNER:
			// Use nearest-neighbor scaling.
			ret = IMGPF_RESCALE_NEAREST;
			break;

		default:
			// Nothing else is supported.
			break;
	}

	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int iQuePlayer::loadFieldData(void)
{
	RP_D(iQuePlayer);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->iQueFileType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	const iQuePlayer_BbContentMetaDataHead *const bbContentMetaDataHead = &d->bbContentMetaDataHead;
	d->fields.reserve(5);	// Maximum of 5 fields. (TODO: Add more.)

	// Get the title and ISBN.
	// TODO: Trim trailing newlines?
	string rom_title, rom_isbn;
	int ret = d->getTitleAndISBN(rom_title, rom_isbn);
	if (ret == 0) {
		// Title.
		if (!rom_title.empty()) {
			d->fields.addField_string(C_("RomData", "Title"), rom_title);
		}

		// ISBN.
		if (!rom_isbn.empty()) {
			d->fields.addField_string(C_("RomData", "ISBN"), rom_isbn);
		}
	}

	// Content ID.
	// NOTE: We don't want the "0x" prefix.
	// This is sort of like Wii title IDs, but only the
	// title ID low portion.
	d->fields.addField_string(C_("iQuePlayer", "Content ID"),
		rp_sprintf("%08X", be32_to_cpu(bbContentMetaDataHead->content_id)),
		RomFields::STRF_MONOSPACE);

	if (d->iQueFileType == iQuePlayerPrivate::IQueFileType::DAT) {
		// Ticket-specific fields.
		const iQuePlayer_BbTicketHead *const bbTicketHead = &d->bbTicketHead;

		// Console ID.
		// TODO: Hide the "0x" prefix?
		d->fields.addField_string_numeric(C_("iQuePlayer", "Console ID"),
			be32_to_cpu(bbTicketHead->bbId), RomFields::Base::Hex, 8,
			RomFields::STRF_MONOSPACE);
	}

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

	d->fields.addField_bitfield(C_("iQuePlayer", "HW Access"),
		v_hw_access_names, 3, be32_to_cpu(bbContentMetaDataHead->hwAccessRights));

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int iQuePlayer::loadMetaData(void)
{
	RP_D(iQuePlayer);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->iQueFileType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// Get the title and ISBN.
	// TODO: Trim trailing newlines?
	string rom_title, rom_isbn;
	int ret = d->getTitleAndISBN(rom_title, rom_isbn);
	if (ret == 0) {
		// Title.
		if (!rom_title.empty()) {
			d->metaData->addMetaData_string(Property::Title, rom_title);
		}

		// TODO: ISBN.
		/*if (!rom_isbn.empty()) {
			d->metaData->addMetaData_string(Property::ISBN, rom_isbn);
		}*/
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int iQuePlayer::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(iQuePlayer);
	switch (imageType) {
		case IMG_INT_ICON:
			if (d->img_thumbnail) {
				// Icon (thumbnail) is loaded.
				pImage = d->img_thumbnail;
				return 0;
			}
			break;
		case IMG_INT_BANNER:
			if (d->img_title) {
				// Banner (title) is loaded.
				pImage = d->img_title;
				return 0;
			}
			break;
		default:
			// Unsupported image type.
			pImage.reset();
			return 0;
	}

	if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->iQueFileType < 0) {
		// Save file isn't valid.
		return -EIO;
	}

	// Load the image.
	switch (imageType) {
		case IMG_INT_ICON:
			pImage = d->loadThumbnailImage();
			break;
		case IMG_INT_BANNER:
			pImage = d->loadTitleImage();
			break;
		default:
			// Unsupported.
			pImage.reset();
			return -ENOENT;
	}

	// TODO: -ENOENT if the file doesn't actually have an icon/banner.
	return ((bool)pImage ? 0 : -EIO);
}

}

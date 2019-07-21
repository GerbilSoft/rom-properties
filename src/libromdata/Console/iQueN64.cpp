/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * iQueN64.cpp: iQue Nintendo 64 .cmd reader.                              *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "iQueN64.hpp"
#include "librpbase/RomData_p.hpp"

#include "ique_n64_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
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

namespace LibRomData {

ROMDATA_IMPL(iQueN64)

class iQueN64Private : public RomDataPrivate
{
	public:
		iQueN64Private(iQueN64 *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(iQueN64Private)

	public:
		// .cmd structs.
		iQueN64_contentDesc contentDesc;
		iQueN64_BbContentMetaDataHead bbContentMetaDataHead;

	public:
		/**
		 * Get the ROM title and ISBN.
		 * @param title Output string for title.
		 * @param isbn Output string for ISBN.
		 * @return 0 on success; non-zero on error.
		 */
		int getTitleAndISBN(string &title, string &isbn);
};

/** iQueN64Private **/

iQueN64Private::iQueN64Private(iQueN64 *q, IRpFile *file)
	: super(q, file)
{
	// Clear the .cmd structs.
	memset(&contentDesc, 0, sizeof(contentDesc));
	memset(&bbContentMetaDataHead, 0, sizeof(bbContentMetaDataHead));
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
		printf("A\n");
		return 1;
	}

	const size_t title_sz = title_buf_sz - title_addr;
	size_t size = file->seekAndRead(title_addr, title_buf.get(), title_sz);
	if (size != title_sz) {
		printf("B\n");
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
	d->fileType = FTYPE_ICON_FILE;	// TODO: Metadata file?

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

}

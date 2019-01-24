/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XDBF.cpp: Microsoft Xbox 360 game resource reader.              *
 * Handles XDBF files and sections.                                        *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

#include "Xbox360_XDBF.hpp"
#include "librpbase/RomData_p.hpp"

#include "xbox360_xdbf_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/file/RpMemFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

ROMDATA_IMPL(Xbox360_XDBF)
ROMDATA_IMPL_IMG_TYPES(Xbox360_XDBF)
ROMDATA_IMPL_IMG_SIZES(Xbox360_XDBF)

// Workaround for RP_D() expecting the no-underscore naming convention.
#define Xbox360_XDBFPrivate Xbox360_XDBF_Private

class Xbox360_XDBF_Private : public RomDataPrivate
{
	public:
		Xbox360_XDBF_Private(Xbox360_XDBF *q, IRpFile *file);
		virtual ~Xbox360_XDBF_Private();

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Xbox360_XDBF_Private)

	public:
		// Internal icon.
		rp_image *img_icon;

	public:
		// XDBF header.
		XDBF_Header xdbfHeader;

		// Entry table.
		// Size is indicated by xdbfHeader.entry_table_length * sizeof(XDBF_Entry).
		// NOTE: Data is *not* byteswapped on load.
		XDBF_Entry *entryTable;

		// Data start offset within the file.
		uint32_t data_offset;

		// String tables.
		// NOTE: These are *pointers* to ao::uvector<>.
		ao::uvector<char> *strTbl[XDBF_LANGUAGE_MAX];

		/**
		 * Find a resource in the entry table.
		 * @param namespace_id Namespace ID.
		 * @param resource_id Resource ID.
		 * @return XDBF_Entry*, or nullptr if not found.
		 */
		const XDBF_Entry *findResource(uint16_t namespace_id, uint64_t resource_id) const;

		/**
		 * Load a string table.
		 * @param language_id Language ID.
		 * @return Pointer to string table on success; nullptr on error.
		 */
		const ao::uvector<char> *loadStringTable(XDBF_Language_e language_id);

		/**
		 * Get a string from a string table.
		 * @param language_id Language ID.
		 * @param string_id String ID.
		 * @return String, or empty string on error.
		 */
		string loadString(XDBF_Language_e language_id, uint16_t string_id);

		/**
		 * Get the language ID to use for the title fields.
		 * @return XDBF language ID.
		 */
		XDBF_Language_e getLangID(void) const;

		/**
		 * Load the main title icon.
		 * @return Icon, or nullptr on error.
		 */
		const rp_image *loadIcon(void);
};

/** Xbox360_XDBF_Private **/

Xbox360_XDBF_Private::Xbox360_XDBF_Private(Xbox360_XDBF *q, IRpFile *file)
	: super(q, file)
	, img_icon(nullptr)
	, entryTable(nullptr)
	, data_offset(0)
{
	// Clear the header.
	memset(&xdbfHeader, 0, sizeof(xdbfHeader));

	// Clear the string table pointers.
	memset(strTbl, 0, sizeof(strTbl));
}

Xbox360_XDBF_Private::~Xbox360_XDBF_Private()
{
	delete img_icon;
	delete[] entryTable;

	// Delete any allocated string tables.
	for (int i = ARRAY_SIZE(strTbl)-1; i >= 0; i--) {
		if (strTbl[i]) {
			delete strTbl[i];
		}
	}
}

/**
 * Find a resource in the entry table.
 * @param namespace_id Namespace ID.
 * @param resource_id Resource ID.
 * @return XDBF_Entry*, or nullptr if not found.
 */
const XDBF_Entry *Xbox360_XDBF_Private::findResource(uint16_t namespace_id, uint64_t resource_id) const
{
	if (!entryTable) {
		// Entry table isn't loaded...
		return nullptr;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the IDs to make it easier to find things.
	namespace_id = cpu_to_be16(namespace_id);
	resource_id  = cpu_to_be64(resource_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	const XDBF_Entry *p = entryTable;
	const XDBF_Entry *const p_end = p + xdbfHeader.entry_table_length;
	for (; p < p_end; p++) {
		if (p->namespace_id == namespace_id &&
		    p->resource_id == resource_id)
		{
			// Found a match!
			return p;
		}
	}

	// No match.
	return nullptr;
}

/**
 * Load a string table.
 * @param language_id Language ID.
 * @return Pointer to string table on success; nullptr on error.
 */
const ao::uvector<char> *Xbox360_XDBF_Private::loadStringTable(XDBF_Language_e language_id)
{
	assert(language_id >= 0);
	assert(language_id < XDBF_LANGUAGE_MAX);
	if (language_id < 0 || language_id >= XDBF_LANGUAGE_MAX)
		return nullptr;

	// Is the string table already loaded?
	if (this->strTbl[language_id]) {
		return this->strTbl[language_id];
	}

	// Can we load the string table?
	if (!file || !isValid) {
		// Can't load the string table.
		return nullptr;
	}

	// Find the string table entry.
	const XDBF_Entry *const entry = findResource(
		XDBF_NAMESPACE_STRING_TABLE, static_cast<uint64_t>(language_id));
	if (!entry) {
		// Not found.
		return nullptr;
	}

	// Allocate memory and load the string table.
	const unsigned int str_tbl_sz = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be larger than sizeof(XDBF_String_Table_Header)
	// - Size must be a maximum of 1 MB.
	assert(str_tbl_sz > sizeof(XDBF_String_Table_Header));
	assert(str_tbl_sz <= 1024*1024);
	if (str_tbl_sz <= sizeof(XDBF_String_Table_Header) || str_tbl_sz > 1024*1024) {
		// Size is out of range.
		return nullptr;
	}
	ao::uvector<char> *vec = new ao::uvector<char>(str_tbl_sz);

	const unsigned int str_tbl_addr = be32_to_cpu(entry->offset) + this->data_offset;
	size_t size = file->seekAndRead(str_tbl_addr, vec->data(), str_tbl_sz);
	if (size != str_tbl_sz) {
		// Seek and/or read error.
		delete vec;
		return nullptr;
	}

	// Validate the string table magic.
	const XDBF_String_Table_Header *const tblHdr =
		reinterpret_cast<const XDBF_String_Table_Header*>(vec->data());
	if (tblHdr->magic != cpu_to_be32(XDBF_XSTR_MAGIC) ||
	    tblHdr->version != cpu_to_be32(XDBF_XSTR_VERSION))
	{
		// Magic is invalid.
		// TODO: Report an error?
		delete vec;
		return nullptr;
	}

	// String table loaded successfully.
	this->strTbl[language_id] = vec;
	return vec;
}

/**
 * Get a string from a string table.
 * @param language_id Language ID.
 * @param string_id String ID.
 * @return String, or empty string on error.
 */
string Xbox360_XDBF_Private::loadString(XDBF_Language_e language_id, uint16_t string_id)
{
	string ret;

	assert(language_id >= 0);
	assert(language_id < XDBF_LANGUAGE_MAX);
	if (language_id < 0 || language_id >= XDBF_LANGUAGE_MAX)
		return ret;

	// Get the string table.
	const ao::uvector<char> *vec = strTbl[language_id];
	if (!vec) {
		vec = loadStringTable(language_id);
		if (!vec) {
			// Unable to load the string table.
			return ret;
		}
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the ID to make it easier to find things.
	string_id = cpu_to_be16(string_id);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// TODO: Optimize by creating an unordered_map of IDs to strings?
	// Might not be a good optimization if we don't have that many strings...

	// Search for the specified string.
	const char *p = vec->data() + sizeof(XDBF_String_Table_Header);
	const char *const p_end = p + vec->size() - sizeof(XDBF_String_Table_Header);
	while (p < p_end) {
		// TODO: Verify alignment.
		const XDBF_String_Table_Entry_Header *const hdr =
			reinterpret_cast<const XDBF_String_Table_Entry_Header*>(p);
		const uint16_t length = be16_to_cpu(hdr->length);
		if (hdr->string_id == string_id) {
			// Found the string.
			// Verify that it doesn't go out of bounds.
			const char *const p_str = p + sizeof(XDBF_String_Table_Entry_Header);
			const char *const p_str_end = p_str + length;
			if (p_str_end <= p_end) {
				// Bounds are OK.
				// Copy the string as-is, since the string table
				// uses UTF-8 encoding.
				ret.assign(p_str, length);
			}
			break;
		} else {
			// Not the requested string.
			// Go to the next string.
			p += sizeof(XDBF_String_Table_Entry_Header) + length;
		}
	}

	return ret;
}

/**
 * Get the language ID to use for the title fields.
 * @return XDBF language ID.
 */
XDBF_Language_e Xbox360_XDBF_Private::getLangID(void) const
{
	// TODO: Get the system language.
	// Assuming English for now.

	// TODO: Also get the fallback language from XSTC.

	return XDBF_LANGUAGE_ENGLISH;
}

/**
 * Load the main title icon.
 * @return Icon, or nullptr on error.
 */
const rp_image *Xbox360_XDBF_Private::loadIcon(void)
{
	if (img_icon) {
		// Icon has already been loaded.
		return img_icon;
	} else if (!file || !isValid) {
		// Can't load the icon.
		return nullptr;
	}

	// Make sure the entry table is loaded.
	if (!entryTable) {
		// Not loaded. Cannot load an icon.
		return nullptr;
	}

	// Icons are stored in PNG format.
	// TODO: Achievement icons?

	// Get the icon resource.
	const XDBF_Entry *entry = findResource(XDBF_NAMESPACE_IMAGE, XDBF_ID_TITLE);
	if (!entry) {
		// Not found...
		return nullptr;
	}

	// Load the icon.
	const unsigned int addr = be32_to_cpu(entry->offset) + this->data_offset;
	const unsigned int length = be32_to_cpu(entry->length);
	// Sanity check:
	// - Size must be at least 16 bytes. [TODO: Smallest PNG?]
	// - Size must be a maximum of 1 MB.
	assert(length >= 16);
	assert(length <= 1024*1024);
	if (length < 16 || length > 1024*1024) {
		// Size is out of range.
		return nullptr;
	}

	unique_ptr<uint8_t[]> png_buf(new uint8_t[length]);
	size_t size = file->seekAndRead(addr, png_buf.get(), length);
	if (size != length) {
		// Seek and/or read error.
		return nullptr;
	}

	// Create an RpMemFile.
	// TODO: For rpcli, shortcut to extract the PNG directly.
	RpMemFile *const f_mem = new RpMemFile(png_buf.get(), length);
	img_icon = RpPng::load(f_mem);
	delete f_mem;

	return img_icon;
}

/** Xbox360_XDBF **/

/**
 * Read an Xbox 360 XDBF file and/or section.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open XDBF file and/or section.
 */
Xbox360_XDBF::Xbox360_XDBF(IRpFile *file)
	: super(new Xbox360_XDBF_Private(this, file))
{
	// This class handles XDBF files and/or sections only.
	RP_D(Xbox360_XDBF);
	d->className = "Xbox360_XEX";	// Using same image settings as Xbox360_XEX.
	d->fileType = FTYPE_RESOURCE_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the Xbox360_XDBF header.
	d->file->rewind();
	size_t size = d->file->read(&d->xdbfHeader, sizeof(d->xdbfHeader));
	if (size != sizeof(d->xdbfHeader)) {
		d->xdbfHeader.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->xdbfHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->xdbfHeader);
	info.ext = nullptr;	// Not needed for XDBF.
	info.szFile = 0;	// Not needed for XDBF.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->xdbfHeader.magic = 0;
		delete d->file;
		d->file = nullptr;
		return;
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the header for little-endian systems.
	// NOTE: The magic number is *not* byteswapped here.
	d->xdbfHeader.version			= be32_to_cpu(d->xdbfHeader.version);
	d->xdbfHeader.entry_table_length	= be32_to_cpu(d->xdbfHeader.entry_table_length);
	d->xdbfHeader.entry_count		= be32_to_cpu(d->xdbfHeader.entry_count);
	d->xdbfHeader.free_space_table_length	= be32_to_cpu(d->xdbfHeader.free_space_table_length);
	d->xdbfHeader.free_space_table_count	= be32_to_cpu(d->xdbfHeader.free_space_table_count);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

	// Calculate the data start offset.
	d->data_offset = sizeof(d->xdbfHeader) +
		(d->xdbfHeader.entry_table_length * sizeof(XDBF_Entry)) +
		(d->xdbfHeader.free_space_table_length * sizeof(XDBF_Free_Space_Entry));

	// Read the entry table.
	const size_t entry_table_sz = d->xdbfHeader.entry_table_length * sizeof(XDBF_Entry);
	d->entryTable = new XDBF_Entry[d->xdbfHeader.entry_table_length];
	size = d->file->read(d->entryTable, entry_table_sz);
	if (size != entry_table_sz) {
		// Read error.
		delete[] d->entryTable;
		d->entryTable = nullptr;
		d->xdbfHeader.magic = 0;
		delete d->file;
		d->file = nullptr;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Xbox360_XDBF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(XDBF_Entry))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for XDBF.
	const XDBF_Header *const xdbfHeader =
		reinterpret_cast<const XDBF_Header*>(info->header.pData);
	if (xdbfHeader->magic == cpu_to_be32(XDBF_MAGIC) &&
	    xdbfHeader->version == cpu_to_be32(XDBF_VERSION))
	{
		// We have an XDBF file.
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
const char *Xbox360_XDBF::systemName(unsigned int type) const
{
	RP_D(const Xbox360_XDBF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (long, short, abbreviation)
	// TODO: XDBF-specific, or just use Xbox 360?
	static const char *const sysNames[4] = {
		"Microsoft Xbox 360", "Xbox 360", "X360", nullptr
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
const char *const *Xbox360_XDBF::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".xdbf",
		//".gpd",	// Gamer Profile Data

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
const char *const *Xbox360_XDBF::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-xbox360-xdbf",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t Xbox360_XDBF::supportedImageTypes_static(void)
{
	return IMGBF_INT_ICON;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> Xbox360_XDBF::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_ICON) {
		// Only icons are supported.
		return vector<ImageSizeDef>();
	}

	// FIXME: Get the actual icon size from the PNG image.
	// For now, assuming all games use 64x64.
	static const ImageSizeDef sz_INT_ICON[] = {
		{nullptr, 64, 64, 0},
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
uint32_t Xbox360_XDBF::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_INT_ICON:
			// Use nearest-neighbor scaling.
			ret = IMGPF_RESCALE_NEAREST;
			break;
		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Xbox360_XDBF::loadFieldData(void)
{
	RP_D(Xbox360_XDBF);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// XDBF file isn't valid.
		return -EIO;
	}

	// NOTE: Using "XEX" as the localization context.

	// Parse the XDBF file.
	// NOTE: The magic number is NOT byteswapped in the constructor.
	// COMMIT NOTE: Find other classes that leave magic unswapped.
	const XDBF_Header *const xdbfHeader = &d->xdbfHeader;
	if (xdbfHeader->magic != cpu_to_be32(XDBF_MAGIC)) {
		// Invalid magic number.
		return 0;
	}

	// Maximum of 1 field.
	d->fields->reserve(1);
	d->fields->setTabName(0, "XDBF");

	// TODO: XSTR string table handling class.
	// For now, just reading it directly.

	// TODO: Convenience function to look up a resource
	// given a namespace ID and resource ID.

	// Language ID.
	const XDBF_Language_e langID = d->getLangID();

	// Find the English string table.
	string title = d->loadString(langID, XDBF_ID_TITLE);
	d->fields->addField_string(C_("RomData", "Title"),
		!title.empty() ? title : C_("RomData", "Unknown"));

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Pointer to const rp_image* to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int Xbox360_XDBF::loadInternalImage(ImageType imageType, const rp_image **pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);

	RP_D(Xbox360_XDBF);
	if (imageType != IMG_INT_ICON) {
		// Only IMG_INT_ICON is supported by 3DS.
		*pImage = nullptr;
		return -ENOENT;
	} else if (d->img_icon) {
		// Image has already been loaded.
		*pImage = d->img_icon;
		return 0;
	} else if (!d->file) {
		// File isn't open.
		*pImage = nullptr;
		return -EBADF;
	} else if (!d->isValid) {
		// SMDH file isn't valid.
		*pImage = nullptr;
		return -EIO;
	}

	// Load the icon.
	*pImage = d->loadIcon();
	return (*pImage != nullptr ? 0 : -EIO);
}

}

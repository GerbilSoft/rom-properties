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
#include "librpbase/img/rp_image.hpp"
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

namespace LibRomData {

ROMDATA_IMPL(Xbox360_XDBF)

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
		// XDBF header.
		XDBF_Header xdbfHeader;

		// Entry table.
		// Size is indicated by xdbfHeader.entry_table_length * sizeof(XDBF_Entry).
		// NOTE: Data is *not* byteswapped on load.
		XDBF_Entry *entryTable;

		// Data start offset within the file.
		uint32_t data_offset;

		/**
		 * Get the language ID to use for the title fields.
		 * @return XDBF language ID.
		 */
		XDBF_Language_e getLangID(void) const;
};

/** Xbox360_XDBF_Private **/

Xbox360_XDBF_Private::Xbox360_XDBF_Private(Xbox360_XDBF *q, IRpFile *file)
	: super(q, file)
	, entryTable(nullptr)
	, data_offset(0)
{
	// Clear the header.
	memset(&xdbfHeader, 0, sizeof(xdbfHeader));
}

Xbox360_XDBF_Private::~Xbox360_XDBF_Private()
{
	delete[] entryTable;
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

	// Find the English string table.
	// TODO: Get the system and/or default locale.
	const XDBF_Entry *entry = d->entryTable;
	const XDBF_Entry *const end = d->entryTable + d->xdbfHeader.entry_table_length;
	for (; entry < end; entry++) {
		if (entry->namespace_id != cpu_to_be16(XDBF_NAMESPACE_STRING_TABLE))
			continue;
		if (entry->id == cpu_to_be64(XDBF_LANGUAGE_ENGLISH))
			break;
	}

	string title;
	if (entry < end) {
		// Found the English string table.
		// Load it into memory.
		const unsigned int str_tbl_addr = be32_to_cpu(entry->offset) + d->data_offset;
		const unsigned int str_tbl_sz = be32_to_cpu(entry->length);
		unique_ptr<char[]> str_tbl(new char[str_tbl_sz]);
		size_t size = d->file->seekAndRead(str_tbl_addr, str_tbl.get(), str_tbl_sz);
		if (size == str_tbl_sz) {
			// Search for the title string.
			// ID: 0x8001
			const XDBF_String_Table_Header *const tblHdr =
				reinterpret_cast<const XDBF_String_Table_Header*>(str_tbl.get());
			if (tblHdr->magic == cpu_to_be32(XDBF_XSTR_MAGIC) &&
			    tblHdr->version == cpu_to_be32(XDBF_XSTR_VERSION))
			{
				// Valid string table.
				// TODO: Validate tblHdr->size and tblHdr->string_count?
				const char *p = str_tbl.get() + sizeof(XDBF_String_Table_Header);
				const char *const p_end = p + str_tbl_sz - sizeof(XDBF_String_Table_Header);
				while (p < p_end) {
					// TODO: Verify alignment.
					const XDBF_String_Table_Entry_Header *const hdr =
						reinterpret_cast<const XDBF_String_Table_Entry_Header*>(p);
					const uint16_t length = be16_to_cpu(hdr->length);
					if (hdr->id == cpu_to_be16(XDBF_ID_TITLE)) {
						// Found the title string.
						// Verify that it doesn't go out of bounds.
						const char *const p_str = p + sizeof(XDBF_String_Table_Entry_Header);
						const char *const p_str_end = p_str + length;
						if (p_str_end <= p_end) {
							// Bounds are OK.
							// Copy the string as-is, since the string table
							// uses UTF-8 encoding.
							title.assign(p_str, length);
						}
						break;
					} else {
						// Not the title string.
						// Go to the next string.
						p += sizeof(XDBF_String_Table_Entry_Header) + length;
					}
				}
			}
		}
	}

	// TODO: XDBF or RomData context?
	d->fields->addField_string(C_("XDBF", "Title"),
		!title.empty() ? title : C_("RomData", "Unknown"));

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}

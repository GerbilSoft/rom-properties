/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GBS.hpp: GBS audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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

#include "GBS.hpp"
#include "librpbase/RomData_p.hpp"

#include "gbs_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(GBS)

class GBSPrivate : public RomDataPrivate
{
	public:
		GBSPrivate(GBS *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GBSPrivate)

	public:
		// GBS header.
		// NOTE: **NOT** byteswapped in memory.
		GBS_Header gbsHeader;
};

/** GBSPrivate **/

GBSPrivate::GBSPrivate(GBS *q, IRpFile *file)
	: super(q, file)
{
	// Clear the GBS header struct.
	memset(&gbsHeader, 0, sizeof(gbsHeader));
}

/** GBS **/

/**
 * Read a GBS audio file.
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
GBS::GBS(IRpFile *file)
	: super(new GBSPrivate(this, file))
{
	RP_D(GBS);
	d->className = "GBS";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the GBS header.
	d->file->rewind();
	size_t size = d->file->read(&d->gbsHeader, sizeof(d->gbsHeader));
	if (size != sizeof(d->gbsHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->gbsHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->gbsHeader);
	info.ext = nullptr;	// Not needed for GBS.
	info.szFile = 0;	// Not needed for GBS.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GBS::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GBS_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const GBS_Header *const gbsHeader =
		reinterpret_cast<const GBS_Header*>(info->header.pData);

	// Check the GBS magic number.
	if (gbsHeader->magic == cpu_to_be32(GBS_MAGIC)) {
		// Found the GBS magic number.
		return 0;
	}

	// Not suported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GBS::systemName(unsigned int type) const
{
	RP_D(const GBS);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBS has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GBS::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Game Boy Sound System", "GBS", "GBS", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *GBS::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gbs",

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
const char *const *GBS::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-gbs",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GBS::loadFieldData(void)
{
	RP_D(GBS);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// GBS header.
	const GBS_Header *const gbsHeader = &d->gbsHeader;
	d->fields->reserve(9);	// Maximum of 9 fields.

	// NOTE: The GBS specification says ASCII, but I'm assuming
	// the text is cp1252 and/or Shift-JIS.

	// Title.
	if (gbsHeader->title[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Title"),
			cp1252_sjis_to_utf8(gbsHeader->title, sizeof(gbsHeader->title)));
	}

	// Composer.
	if (gbsHeader->composer[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Composer"),
			cp1252_sjis_to_utf8(gbsHeader->composer, sizeof(gbsHeader->composer)));
	}

	// Copyright.
	if (gbsHeader->copyright[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Copyright"),
			cp1252_sjis_to_utf8(gbsHeader->copyright, sizeof(gbsHeader->copyright)));
	}

	// Number of tracks.
	d->fields->addField_string_numeric(C_("RomData|Audio", "Track Count"),
		gbsHeader->track_count);

	// Default track number.
	d->fields->addField_string_numeric(C_("RomData|Audio", "Default Track #"),
		gbsHeader->default_track);

	// Load address.
	d->fields->addField_string_numeric(C_("GBS", "Load Address"),
		le16_to_cpu(gbsHeader->load_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// Init address.
	d->fields->addField_string_numeric(C_("GBS", "Init Address"),
		le16_to_cpu(gbsHeader->init_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// Play address.
	d->fields->addField_string_numeric(C_("GBS", "Play Address"),
		le16_to_cpu(gbsHeader->play_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// Play address.
	d->fields->addField_string_numeric(C_("GBS", "Stack Pointer"),
		le16_to_cpu(gbsHeader->stack_pointer),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// TODO: Timer modulo and control?

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GBS::loadMetaData(void)
{
	RP_D(GBS);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(3);	// Maximum of 3 metadata properties.

	// GBS header.
	const GBS_Header *const gbsHeader = &d->gbsHeader;

	// Title.
	if (gbsHeader->title[0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_sjis_to_utf8(gbsHeader->title, sizeof(gbsHeader->title)));
	}

	// Composer.
	if (gbsHeader->composer[0] != 0) {
		d->metaData->addMetaData_string(Property::Composer,
			cp1252_sjis_to_utf8(gbsHeader->composer, sizeof(gbsHeader->composer)));
	}

	// Copyright.
	if (gbsHeader->copyright[0] != 0) {
		d->metaData->addMetaData_string(Property::Copyright,
			cp1252_sjis_to_utf8(gbsHeader->copyright, sizeof(gbsHeader->copyright)));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->fields->count());
}

}

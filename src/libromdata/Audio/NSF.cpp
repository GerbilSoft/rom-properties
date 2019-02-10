/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NSF.hpp: NSF audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
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

#include "NSF.hpp"
#include "librpbase/RomData_p.hpp"

#include "nsf_structs.h"

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

ROMDATA_IMPL(NSF)

class NSFPrivate : public RomDataPrivate
{
	public:
		NSFPrivate(NSF *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NSFPrivate)

	public:
		// NSF header.
		// NOTE: **NOT** byteswapped in memory.
		NSF_Header nsfHeader;
};

/** NSFPrivate **/

NSFPrivate::NSFPrivate(NSF *q, IRpFile *file)
	: super(q, file)
{
	// Clear the NSF header struct.
	memset(&nsfHeader, 0, sizeof(nsfHeader));
}

/** NSF **/

/**
 * Read an NSF audio file.
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
NSF::NSF(IRpFile *file)
	: super(new NSFPrivate(this, file))
{
	RP_D(NSF);
	d->className = "NSF";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the NSF header.
	d->file->rewind();
	size_t size = d->file->read(&d->nsfHeader, sizeof(d->nsfHeader));
	if (size != sizeof(d->nsfHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->nsfHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->nsfHeader);
	info.ext = nullptr;	// Not needed for NSF.
	info.szFile = 0;	// Not needed for NSF.
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
int NSF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NSF_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const NSF_Header *const nsfHeader =
		reinterpret_cast<const NSF_Header*>(info->header.pData);

	// Check the NSF magic number.
	if (!memcmp(nsfHeader->magic, NSF_MAGIC, sizeof(nsfHeader->magic))) {
		// Found the NSF magic number.
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
const char *NSF::systemName(unsigned int type) const
{
	RP_D(const NSF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NSF has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NSF::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Nintendo Sound Format", "NSF", "NSF", nullptr
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
const char *const *NSF::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".nsf",

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
const char *const *NSF::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-nsf",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NSF::loadFieldData(void)
{
	RP_D(NSF);
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

	// NSF header.
	const NSF_Header *const nsfHeader = &d->nsfHeader;
	d->fields->reserve(10);	// Maximum of 10 fields.

	// NOTE: The NSF specification says ASCII, but I'm assuming
	// the text is cp1252 and/or Shift-JIS.

	// Title.
	if (nsfHeader->title[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Title"),
			cp1252_sjis_to_utf8(nsfHeader->title, sizeof(nsfHeader->title)));
	}

	// Composer.
	if (nsfHeader->composer[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Composer"),
			cp1252_sjis_to_utf8(nsfHeader->composer, sizeof(nsfHeader->composer)));
	}

	// Copyright.
	if (nsfHeader->copyright[0] != 0) {
		d->fields->addField_string(C_("RomData|Audio", "Copyright"),
			cp1252_sjis_to_utf8(nsfHeader->copyright, sizeof(nsfHeader->copyright)));
	}

	// Number of tracks.
	d->fields->addField_string_numeric(C_("RomData|Audio", "Track Count"),
		nsfHeader->track_count);

	// Default track number.
	d->fields->addField_string_numeric(C_("RomData|Audio", "Default Track #"),
		nsfHeader->default_track);

	// Load address.
	d->fields->addField_string_numeric(C_("NSF", "Load Address"),
		le16_to_cpu(nsfHeader->load_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// Init address.
	d->fields->addField_string_numeric(C_("NSF", "Init Address"),
		le16_to_cpu(nsfHeader->init_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// Play address.
	d->fields->addField_string_numeric(C_("NSF", "Play Address"),
		le16_to_cpu(nsfHeader->play_address),
		RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);

	// TV System.
	// TODO: NTSC/PAL framerates?
	// NOTE: NSF uses an enum, not a bitfield.
	static const char *const tv_system_bitfield_names[] = {
		NOP_C_("NSF|TVSystem", "NTSC"),
		NOP_C_("NSF|TVSystem", "PAL"),
	};
	uint32_t bfval = nsfHeader->tv_system;
	if (bfval < NSF_TV_MAX) {
		bfval++;
	} else {
		bfval = 0;
	}
	vector<string> *const v_tv_system_bitfield_names = RomFields::strArrayToVector_i18n(
		"NSF|TVSystem", tv_system_bitfield_names, ARRAY_SIZE(tv_system_bitfield_names));
	d->fields->addField_bitfield(C_("NSF", "TV System"),
		v_tv_system_bitfield_names, 0, bfval);

	// Expansion audio.
	static const char *const expansion_bitfield_names[] = {
		NOP_C_("NSF|Expansion", "Konami VRC6"),
		NOP_C_("NSF|Expansion", "Konami VRC7"),
		NOP_C_("NSF|Expansion", "2C33 (FDS)"),
		NOP_C_("NSF|Expansion", "MMC5"),
		NOP_C_("NSF|Expansion", "Namco N163"),
		NOP_C_("NSF|Expansion", "Sunsoft 5B"),
	};
	vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector_i18n(
		"NSF|Expansion", expansion_bitfield_names, ARRAY_SIZE(expansion_bitfield_names));
	d->fields->addField_bitfield(C_("NSF", "Expansion"),
		v_expansion_bitfield_names, 3, nsfHeader->expansion_audio);

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NSF::loadMetaData(void)
{
	RP_D(NSF);
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

	// NSF header.
	const NSF_Header *const nsfHeader = &d->nsfHeader;

	// Title.
	if (nsfHeader->title[0] != 0) {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_sjis_to_utf8(nsfHeader->title, sizeof(nsfHeader->title)));
	}

	// Composer.
	if (nsfHeader->composer[0] != 0) {
		d->metaData->addMetaData_string(Property::Composer,
			cp1252_sjis_to_utf8(nsfHeader->composer, sizeof(nsfHeader->composer)));
	}

	// Copyright.
	if (nsfHeader->copyright[0] != 0) {
		d->metaData->addMetaData_string(Property::Copyright,
			cp1252_sjis_to_utf8(nsfHeader->copyright, sizeof(nsfHeader->copyright)));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->fields->count());
}

}

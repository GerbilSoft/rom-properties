/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NSF.hpp: NSF audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NSF.hpp"
#include "nsf_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class NSFPrivate final : public RomDataPrivate
{
	public:
		NSFPrivate(IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NSFPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// NSF header.
		// NOTE: **NOT** byteswapped in memory.
		NSF_Header nsfHeader;
};

ROMDATA_IMPL(NSF)

/** NSFPrivate **/

/* RomDataInfo */
const char *const NSFPrivate::exts[] = {
	".nsf",

	nullptr
};
const char *const NSFPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"audio/x-nsf",

	nullptr
};
const RomDataInfo NSFPrivate::romDataInfo = {
	"NSF", exts, mimeTypes
};

NSFPrivate::NSFPrivate(IRpFile *file)
	: super(file, &romDataInfo)
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
	: super(new NSFPrivate(file))
{
	RP_D(NSF);
	d->mimeType = "audio/x-nsf";	// unofficial
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the NSF header.
	d->file->rewind();
	size_t size = d->file->read(&d->nsfHeader, sizeof(d->nsfHeader));
	if (size != sizeof(d->nsfHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->nsfHeader), reinterpret_cast<const uint8_t*>(&d->nsfHeader)},
		nullptr,	// ext (not needed for NSF)
		0		// szFile (not needed for NSF)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
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
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NSF::loadFieldData(void)
{
	RP_D(NSF);
	if (!d->fields.empty()) {
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
	d->fields.reserve(10);	// Maximum of 10 fields.

	// NOTE: The NSF specification says ASCII, but I'm assuming
	// the text is cp1252 and/or Shift-JIS.

	// Title.
	if (nsfHeader->title[0] != 0) {
		d->fields.addField_string(C_("RomData|Audio", "Title"),
			cp1252_sjis_to_utf8(nsfHeader->title, sizeof(nsfHeader->title)));
	}

	// Composer.
	if (nsfHeader->composer[0] != 0) {
		d->fields.addField_string(C_("RomData|Audio", "Composer"),
			cp1252_sjis_to_utf8(nsfHeader->composer, sizeof(nsfHeader->composer)));
	}

	// Copyright.
	if (nsfHeader->copyright[0] != 0) {
		d->fields.addField_string(C_("RomData|Audio", "Copyright"),
			cp1252_sjis_to_utf8(nsfHeader->copyright, sizeof(nsfHeader->copyright)));
	}

	// Number of tracks.
	d->fields.addField_string_numeric(C_("RomData|Audio", "Track Count"),
		nsfHeader->track_count);

	// Default track number.
	d->fields.addField_string_numeric(C_("RomData|Audio", "Default Track #"),
		nsfHeader->default_track);

	// Load address.
	d->fields.addField_string_numeric(C_("NSF", "Load Address"),
		le16_to_cpu(nsfHeader->load_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Init address.
	d->fields.addField_string_numeric(C_("NSF", "Init Address"),
		le16_to_cpu(nsfHeader->init_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// Play address.
	d->fields.addField_string_numeric(C_("NSF", "Play Address"),
		le16_to_cpu(nsfHeader->play_address),
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// TV System.
	// TODO: NTSC/PAL framerates?
	// NOTE: NSF uses an enum, not a bitfield.
	static const char *const tv_system_bitfield_names[] = {
		"NTSC", "PAL",
	};
	uint32_t bfval = nsfHeader->tv_system;
	if (bfval < NSF_TV_MAX) {
		bfval++;
	} else {
		bfval = 0;
	}
	vector<string> *const v_tv_system_bitfield_names = RomFields::strArrayToVector(
		tv_system_bitfield_names, ARRAY_SIZE(tv_system_bitfield_names));
	d->fields.addField_bitfield(C_("NSF", "TV System"),
		v_tv_system_bitfield_names, 0, bfval);

	// Expansion audio.
	static const char *const expansion_bitfield_names[] = {
		"Konami VRC6", "Konami VRC7", "2C33 (FDS)",
		"MMC5", "Namco N163", "Sunsoft 5B",
	};
	vector<string> *const v_expansion_bitfield_names = RomFields::strArrayToVector(
		expansion_bitfield_names, ARRAY_SIZE(expansion_bitfield_names));
	d->fields.addField_bitfield(C_("NSF", "Expansion"),
		v_expansion_bitfield_names, 3, nsfHeader->expansion_audio);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
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
	return static_cast<int>(d->metaData->count());
}

}

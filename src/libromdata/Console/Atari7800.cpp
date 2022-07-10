/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Atari7800.cpp: Atari 7800 ROM reader.                                   *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Atari7800.hpp"
#include "atari_7800_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class Atari7800Private final : public RomDataPrivate
{
	public:
		Atari7800Private(Atari7800 *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Atari7800Private)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// ROM header
		Atari_A78Header romHeader;
};

ROMDATA_IMPL(Atari7800)

/** Atari7800Private **/

/* RomDataInfo */
const char8_t *const Atari7800Private::exts[] = {
	U8(".a78"),

	nullptr
};
const char *const Atari7800Private::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-atari-7800-rom",

	nullptr
};
const RomDataInfo Atari7800Private::romDataInfo = {
	"Atari7800", exts, mimeTypes
};

Atari7800Private::Atari7800Private(Atari7800 *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** Atari7800 **/

/**
 * Read an Atari 7800 ROM image.
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
Atari7800::Atari7800(IRpFile *file)
	: super(new Atari7800Private(this, file))
{
	RP_D(Atari7800);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		// Seek and/or read error.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for Atari7800)
		0		// szFile (not needed for Atari7800)
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
int Atari7800::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Atari_A78Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const Atari_A78Header *const romHeader =
		reinterpret_cast<const Atari_A78Header*>(info->header.pData);

	// Check the magic strings.
	if (!memcmp(romHeader->magic, ATARI_7800_A78_MAGIC, sizeof(romHeader->magic)) &&
	    !memcmp(romHeader->end_magic, ATARI_7800_A78_END_MAGIC, sizeof(romHeader->end_magic)))
	{
		// Found the magic strings.
		// TODO: If v3, verify padding?
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
const char *Atari7800::systemName(unsigned int type) const
{
	RP_D(const Atari7800);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Atari 7800 has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"N64::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Atari 7800", "Atari 7800", "7800", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Atari7800::loadFieldData(void)
{
	RP_D(Atari7800);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	const Atari_A78Header *const romHeader = &d->romHeader;
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Title
	// NOTE: Should be ASCII, but allowing cp1252.
	if (romHeader->title[0] != '\0') {
		d->fields->addField_string(C_("RomData", "Title"),
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomFields::STRF_TRIM_END);
	}

	// TV type
	// NOTE: Two components, and values are mutually-exclusive.
	// NOTE 2: Not indicating "composite" since that's the default.
	const char *tv_type_title = C_("Atari7800", "TV Type");
	if (!(romHeader->tv_type & ATARI_A78_TVType_Artifacts_Mask)) {
		// Composite artifacting
		d->fields->addField_string(tv_type_title,
			(romHeader->tv_type & ATARI_A78_TVType_Format_Mask) ? "PAL" : "NTSC");
	} else {
		// Component: no artifacting
		char s_tv_type[32];
		snprintf(s_tv_type, sizeof(s_tv_type), "%s, %s",
			(romHeader->tv_type & ATARI_A78_TVType_Format_Mask) ? "PAL" : "NTSC",
			(romHeader->tv_type & ATARI_A78_TVType_Artifacts_Mask) ? "component" : "composite");
		d->fields->addField_string(tv_type_title, s_tv_type);
	}

	// Controllers
	static const char *const controller_tbl[] = {
		// 0
		NOP_C_("Atari7800|ControllerType", "None"),
		NOP_C_("Atari7800|ControllerType", "Joystick (7800)"),
		NOP_C_("Atari7800|ControllerType", "Light Gun"),
		NOP_C_("Atari7800|ControllerType", "Paddle"),
		NOP_C_("Atari7800|ControllerType", "Trak-Ball"),
		NOP_C_("Atari7800|ControllerType", "Joystick (2600)"),
		NOP_C_("Atari7800|ControllerType", "Driving (2600)"),
		NOP_C_("Atari7800|ControllerType", "Keyboard (2600)"),
		NOP_C_("Atari7800|ControllerType", "Mouse (Atari ST)"),
		NOP_C_("Atari7800|ControllerType", "Mouse (Amiga)"),

		// 10
		"AtariVox / SaveKey",
		"SNES2Atari"
	};
	for (unsigned int i = 0; i < 2; i++) {
		const string control_title = rp_sprintf(C_("Atari7800", "Controller %u"), i+1);
		const uint8_t control_type = romHeader->control_types[i];

		if (control_type < ARRAY_SIZE(controller_tbl)) {
			d->fields->addField_string(control_title.c_str(),
				dpgettext_expr(RP_I18N_DOMAIN, "Atari7800|ControllerType",
					controller_tbl[control_type]));
		} else {
			d->fields->addField_string(control_title.c_str(),
				rp_sprintf(C_("RomData", "Unknown (%u)"), control_type));
		}
	}

	// TODO: Other fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Atari7800::loadMetaData(void)
{
	RP_D(Atari7800);
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

	const Atari_A78Header *const romHeader = &d->romHeader;

	// Title
	// NOTE: Should be ASCII, but allowing cp1252.
	if (romHeader->title[0] != '\0') {
		d->metaData->addMetaData_string(Property::Title,
			cp1252_to_utf8(romHeader->title, sizeof(romHeader->title)),
			RomMetaData::STRF_TRIM_END);
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : -ENOENT);
}

}

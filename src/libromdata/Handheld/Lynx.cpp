/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Lynx.hpp: Atari Lynx ROM reader.                                        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Lynx.hpp"
#include "lnx_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

namespace LibRomData {

class LynxPrivate final : public RomDataPrivate
{
	public:
		LynxPrivate(Lynx *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(LynxPrivate)

	public:
		/** RomDataInfo **/
		static const char8_t *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// ROM header.
		Lynx_RomHeader romHeader;
};

ROMDATA_IMPL(Lynx)

/** LynxPrivate **/

/* RomDataInfo */
const char8_t *const LynxPrivate::exts[] = {
	U8(".lnx"),

	nullptr
};
const char *const LynxPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-atari-lynx-rom",

	nullptr
};
const RomDataInfo LynxPrivate::romDataInfo = {
	"Lynx", exts, mimeTypes
};

LynxPrivate::LynxPrivate(Lynx *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** Lynx **/

/**
 * Read an Atari Lynx ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
Lynx::Lynx(IRpFile *file)
	: super(new LynxPrivate(this, file))
{
	RP_D(Lynx);
	d->mimeType = "application/x-atari-lynx-rom";	// unofficial

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header. [0x40 bytes]
	uint8_t header[0x40];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM is supported.
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for Lynx)
		0		// szFile (not needed for Lynx)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (d->isValid) {
		// Save the header for later.
		memcpy(&d->romHeader, header, sizeof(d->romHeader));
	} else {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Lynx::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 0x40)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the magic number.
	const Lynx_RomHeader *const romHeader =
		reinterpret_cast<const Lynx_RomHeader*>(info->header.pData);
	if (romHeader->magic == cpu_to_be32(LYNX_MAGIC)) {
		// Found a Lynx ROM.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type (See the SystemName enum)
 * @return System name, or nullptr if type is invalid.
 */
const char8_t *Lynx::systemName(unsigned int type) const
{
	RP_D(const Lynx);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Lynx has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Lynx::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char8_t *const sysNames[4] = {
		U8("Atari Lynx"), U8("Lynx"), U8("LNX"), nullptr,
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Lynx::loadFieldData(void)
{
	RP_D(Lynx);
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

	// Lynx ROM header.
	const Lynx_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(5);	// Maximum of 5 fields.

	d->fields->addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->cartname, sizeof(romHeader->cartname)));

	d->fields->addField_string(C_("Lynx", "Manufacturer"),
		latin1_to_utf8(romHeader->manufname, sizeof(romHeader->manufname)));

	static const char8_t rotation_names[][8] = {
		NOP_C_("Lynx|Rotation", "None"),
		NOP_C_("Lynx|Rotation", "Left"),
		NOP_C_("Lynx|Rotation", "Right"),
	};
	const char8_t *const rotation_title = C_("Lynx", "Rotation");
	if (romHeader->rotation < ARRAY_SIZE(rotation_names)) {
		// FIXME: U8STRFIX - dpgettext_expr()
		d->fields->addField_string(rotation_title,
			dpgettext_expr(RP_I18N_DOMAIN, "Lynx|Rotation",
				reinterpret_cast<const char*>(rotation_names[romHeader->rotation])));
	} else {
		d->fields->addField_string(rotation_title, C_("RomData", "Unknown"));
	}

	d->fields->addField_string(C_("Lynx", "Bank 0 Size"),
		LibRpBase::formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	d->fields->addField_string(C_("Lynx", "Bank 1 Size"),
		LibRpBase::formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	return static_cast<int>(d->fields->count());
}

}

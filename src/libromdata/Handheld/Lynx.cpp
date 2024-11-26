/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Lynx.hpp: Atari Lynx ROM reader.                                        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2017-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Lynx.hpp"
#include "lnx_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;

namespace LibRomData {

class LynxPrivate final : public RomDataPrivate
{
public:
	explicit LynxPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(LynxPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 2+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	Lynx_RomHeader romHeader;
};

ROMDATA_IMPL(Lynx)

/** LynxPrivate **/

/* RomDataInfo */
const array<const char*, 2+1> LynxPrivate::exts = {{
	".lnx",
	".lyx",

	nullptr
}};
const array<const char*, 1+1> LynxPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-atari-lynx-rom",

	nullptr
}};
const RomDataInfo LynxPrivate::romDataInfo = {
	"Lynx", exts.data(), mimeTypes.data()
};

LynxPrivate::LynxPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
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
Lynx::Lynx(const IRpFilePtr &file)
	: super(new LynxPrivate(file))
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
		d->file.reset();
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
		d->file.reset();
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
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Lynx::systemName(unsigned int type) const
{
	RP_D(const Lynx);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Lynx has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Lynx::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Atari Lynx", "Lynx", "LNX", nullptr,
	}};

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
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the header.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Lynx ROM header
	const Lynx_RomHeader *const romHeader = &d->romHeader;
	d->fields.reserve(5);	// Maximum of 5 fields.

	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->cartname, sizeof(romHeader->cartname)));

	d->fields.addField_string(C_("Lynx", "Manufacturer"),
		latin1_to_utf8(romHeader->manufname, sizeof(romHeader->manufname)));

	static constexpr char rotation_names[][8] = {
		NOP_C_("Lynx|Rotation", "None"),
		NOP_C_("Lynx|Rotation", "Left"),
		NOP_C_("Lynx|Rotation", "Right"),
	};
	d->fields.addField_string(C_("Lynx", "Rotation"),
		(romHeader->rotation < ARRAY_SIZE(rotation_names)
			? pgettext_expr("Lynx|Rotation", rotation_names[romHeader->rotation])
			: C_("RomData", "Unknown")));

	d->fields.addField_string(C_("Lynx", "Bank 0 Size"),
		LibRpText::formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	d->fields.addField_string(C_("Lynx", "Bank 1 Size"),
		LibRpText::formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Lynx::loadMetaData(void)
{
	RP_D(Lynx);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// No file.
		// A closed file is OK, since we already loaded the header.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown save banner file type.
		return -EIO;
	}

	// Lynx ROM header
	const Lynx_RomHeader *const romHeader = &d->romHeader;
	d->metaData.reserve(2);	// Maximum of 2 metadata properties.

	// Title
	d->metaData.addMetaData_string(Property::Title,
		latin1_to_utf8(romHeader->cartname, sizeof(romHeader->cartname)));

	// Publisher (aka manufacturer)
	d->metaData.addMetaData_string(Property::Publisher,
		latin1_to_utf8(romHeader->manufname, sizeof(romHeader->manufname)));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData

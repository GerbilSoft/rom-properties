/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Lynx.hpp: Atari Lynx ROM reader.                                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2017-2018 by Egor.                                        *
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

#include "Lynx.hpp"
#include "librpbase/RomData_p.hpp"

#include "lnx_structs.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRomData {

ROMDATA_IMPL(Lynx)

class LynxPrivate : public RomDataPrivate
{
	public:
		LynxPrivate(Lynx *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(LynxPrivate)

	public:
		enum Lynx_RomType {
			ROM_UNKNOWN	= -1,	// Unknown ROM type.
			ROM_LYNX = 0,	// Atari Lynx
		};

		// ROM type.
		int romType;

	public:
		// ROM header.
		Lynx_RomHeader romHeader;
};

/** LynxPrivate **/

LynxPrivate::LynxPrivate(Lynx *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** Lynx **/

/**
 * Read an Atari Lynx ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
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
	d->className = "Lynx";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header. [0x40 bytes]
	uint8_t header[0x40];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for Lynx.
	info.szFile = 0;	// Not needed for Lynx.
	d->romType = isRomSupported_static(&info);

	d->isValid = (d->romType >= 0);
	if (d->isValid) {
		// Save the header for later.
		memcpy(&d->romHeader, header, sizeof(d->romHeader));
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

	// Check the system name.
	const Lynx_RomHeader *const romHeader =
		reinterpret_cast<const Lynx_RomHeader*>(info->header.pData);

	static const char lynxMagic[4] = {'L','Y','N','X'};
	if (!memcmp(romHeader->magic, lynxMagic, sizeof(lynxMagic))) {
		// Found a Lynx ROM.
		return LynxPrivate::ROM_LYNX;
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

	static_assert(SYSNAME_TYPE_MASK == 3,
		"Lynx::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	static const char *const sysNames[4] = {
		"Atari Lynx", "Lynx", "LNX", nullptr,
	};

	unsigned int idx = (type & SYSNAME_TYPE_MASK);
	if (idx >= ARRAY_SIZE(sysNames)) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
	}

	return sysNames[idx];
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
const char *const *Lynx::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".lnx",
		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Lynx::loadFieldData(void)
{
	RP_D(Lynx);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Lynx ROM header.
	const Lynx_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(5);	// Maximum of 5 fields.

	d->fields->addField_string(C_("Lynx", "Title"),
		latin1_to_utf8(romHeader->cartname, sizeof(romHeader->cartname)));

	d->fields->addField_string(C_("Lynx", "Manufacturer"),
		latin1_to_utf8(romHeader->manufname, sizeof(romHeader->manufname)));

	static const char *const rotation_names[] = {
		NOP_C_("Lynx|Rotation", "None"),
		NOP_C_("Lynx|Rotation", "Left"),
		NOP_C_("Lynx|Rotation", "Right"),
	};
	d->fields->addField_string("Rotation",
		(romHeader->rotation < ARRAY_SIZE(rotation_names)
			? dpgettext_expr(RP_I18N_DOMAIN, "Lynx|Rotation", rotation_names[romHeader->rotation])
			: C_("Lynx", "Unknown")));

	d->fields->addField_string(C_("Lynx", "Bank 0 Size"),
		d->formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	d->fields->addField_string(C_("Lynx", "Bank 1 Size"),
		d->formatFileSize(le16_to_cpu(romHeader->page_size_bank0) * 256));

	return (int)d->fields->count();
}

}

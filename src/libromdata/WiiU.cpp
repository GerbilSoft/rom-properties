/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiU.cpp: Nintendo Wii U disc image reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.libromdata.h"

#include "WiiU.hpp"
#include "RomData_p.hpp"

#include "wiiu_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"
#include "img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class WiiUPrivate : public RomDataPrivate
{
	public:
		WiiUPrivate(WiiU *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WiiUPrivate)

	public:
		// Disc header.
		WiiU_DiscHeader discHeader;
};

/** WiiUPrivate **/

WiiUPrivate::WiiUPrivate(WiiU *q, IRpFile *file)
	: super(q, file)
{
	// Clear the discHeader struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/** WiiU **/

/**
 * Read a Nintendo Wii U disc image.
 *
 * A disc image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the disc image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open disc image.
 */
WiiU::WiiU(IRpFile *file)
	: super(new WiiUPrivate(this, file))
{
	// This class handles disc images.
	RP_D(WiiU);
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	d->file->rewind();
	size_t size = d->file->read(&d->discHeader, sizeof(d->discHeader));
	if (size != sizeof(d->discHeader))
		return;

	// Check if this disc image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->discHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->discHeader);
	info.ext = nullptr;	// Not needed for Wii U.
	info.szFile = d->file->size();
	bool isValid = (isRomSupported_static(&info) >= 0);
	if (!isValid)
		return;

	// Verify the secondary magic number at 0x10000.
	static const uint8_t wiiu_magic[4] = {0xCC, 0x54, 0x9E, 0xB9};
	int ret = d->file->seek(0x10000);
	if (ret != 0) {
		// Seek error.
		return;
	}

	uint8_t disc_magic[4];
	size = d->file->read(disc_magic, sizeof(disc_magic));
	if (size != sizeof(disc_magic)) {
		// Read error.
		return;
	}

	if (!memcmp(disc_magic, wiiu_magic, sizeof(wiiu_magic))) {
		// Secondary magic matches.
		d->isValid = true;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiU::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(WiiU_DiscHeader) ||
	    info->szFile < 0x20000)
	{
		// Either no detection information was specified,
		// or the header is too small.
		// szFile: Partition table is at 0x18000, so we
		// need to have at least 0x20000.
		return -1;
	}

	// Game ID must start with "WUP-".
	// TODO: Make sure GCN/Wii magic numbers aren't present.
	// NOTE: There's also a secondary magic number at 0x10000,
	// but we can't check it here.
	const WiiU_DiscHeader *wiiu_header = reinterpret_cast<const WiiU_DiscHeader*>(info->header.pData);
	if (memcmp(wiiu_header->id, "WUP-", 4) != 0) {
		// Not Wii U.
		return -1;
	}

	// Check hyphens.
	// TODO: Verify version numbers and region code.
	if (wiiu_header->hyphen1 != '-' ||
	    wiiu_header->hyphen2 != '-' ||
	    wiiu_header->hyphen3 != '-')
	{
		// Missing hyphen.
		return -1;
	}

	// Disc header is valid.
	return 0;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WiiU::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *WiiU::systemName(uint32_t type) const
{
	RP_D(const WiiU);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Wii U"), _RP("Wii U"), _RP("Wii U"), nullptr
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
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> WiiU::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".wud"),

		// NOTE: May cause conflicts on Windows
		// if fallback handling isn't working.
		_RP(".iso"),
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> WiiU::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WiiU::loadFieldData(void)
{
	RP_D(WiiU);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Disc image isn't valid.
		return -EIO;
	}

	// Disc header is read in the constructor.
	d->fields->reserve(4);	// Maximum of 4 fields.

	// Temporary buffer for snprintf().
	char buf[64];
	int len;

	// Game ID.
	d->fields->addField_string(_RP("Game ID"),
		latin1_to_rp_string(d->discHeader.id, sizeof(d->discHeader.id)));

	// Game version.
	// TODO: Validate the version characters.
	len = snprintf(buf, sizeof(buf), "%.2s", d->discHeader.version);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	d->fields->addField_string(_RP("Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// OS version.
	// TODO: Validate the version characters.
	len = snprintf(buf, sizeof(buf), "%c.%c.%c",
		d->discHeader.os_version[0],
		d->discHeader.os_version[1],
		d->discHeader.os_version[2]);
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	d->fields->addField_string(_RP("OS Version"),
		len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

	// Region.
	// TODO: Compare against list of regions and show the fancy name.
	d->fields->addField_string(_RP("Region"),
		latin1_to_rp_string(d->discHeader.region, sizeof(d->discHeader.region)));

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCom.hpp: Tiger game.com ROM reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "GameCom.hpp"
#include "librpbase/RomData_p.hpp"
#include "gcom_structs.h"

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

ROMDATA_IMPL(GameCom)

class GameComPrivate : public RomDataPrivate
{
	public:
		GameComPrivate(GameCom *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameComPrivate)

	public:
		// ROM header.
		Gcom_RomHeader romHeader;
};

/** GameComPrivate **/

GameComPrivate::GameComPrivate(GameCom *q, IRpFile *file)
	: super(q, file)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** GameCom **/

/**
 * Read a Tiger game.com ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
GameCom::GameCom(IRpFile *file)
	: super(new GameComPrivate(this, file))
{
	RP_D(GameCom);
	d->className = "GameCom";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header at the standard address.
	size_t size = d->file->seekAndRead(GCOM_HEADER_ADDRESS, &d->romHeader, sizeof(d->romHeader));
	if (size == sizeof(d->romHeader)) {
		// Check if this ROM image is supported.
		DetectInfo info;
		info.header.addr = GCOM_HEADER_ADDRESS;
		info.header.size = sizeof(d->romHeader);
		info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
		info.ext = nullptr;	// Not needed for Gcom.
		info.szFile = 0;	// Not needed for Gcom.
		d->isValid = (isRomSupported_static(&info) >= 0);
	}

	if (!d->isValid) {
		// Try again at the alternate address.
		size_t size = d->file->seekAndRead(GCOM_HEADER_ADDRESS_ALT, &d->romHeader, sizeof(d->romHeader));
		if (size == sizeof(d->romHeader)) {
			// Check if this ROM image is supported.
			DetectInfo info;
			info.header.addr = GCOM_HEADER_ADDRESS_ALT;
			info.header.size = sizeof(d->romHeader);
			info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
			info.ext = nullptr;	// Not needed for Gcom.
			info.szFile = 0;	// Not needed for Gcom.
			d->isValid = (isRomSupported_static(&info) >= 0);
		}
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameCom::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: The official game.com emulator requires the header to be at 0x40000.
	// Some ROMs have the header at 0, though.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	// TODO: Proper address handling to ensure that 0x40000 is within the buffer.
	// (See SNES for more information.)
	assert(info->header.addr == GCOM_HEADER_ADDRESS || info->header.addr == GCOM_HEADER_ADDRESS_ALT);
	if (!info || !info->header.pData ||
	    (info->header.addr != GCOM_HEADER_ADDRESS && info->header.addr != GCOM_HEADER_ADDRESS_ALT) ||
	    info->header.size < sizeof(Gcom_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the system ID.
	const Gcom_RomHeader *const gcom_header =
		reinterpret_cast<const Gcom_RomHeader*>(info->header.pData);
	if (!memcmp(gcom_header->sys_id, GCOM_SYS_ID, sizeof(GCOM_SYS_ID)-1)) {
		// System ID is correct.
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
const char *GameCom::systemName(unsigned int type) const
{
	RP_D(const GameCom);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameCom::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Tiger game.com",
		"game.com",
		"game.com",
		nullptr
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
const char *const *GameCom::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".bin",	// Most common (only one supported by the official emulator)
		".tgc",	// Less common

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameCom::loadFieldData(void)
{
	RP_D(GameCom);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Add more fields?

	// game.com ROM header.
	const Gcom_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Game title.
	// TODO: Trim spaces?
	d->fields->addField_string(C_("GameCom", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)));

	// Game ID.
	d->fields->addField_string_numeric(C_("GameCom", "Game ID"),
		le16_to_cpu(romHeader->game_id), RomFields::FB_HEX, 4);

	// Entry point..
	d->fields->addField_string_numeric(C_("GameCom", "Entry Point"),
		le16_to_cpu(romHeader->entry_point), RomFields::FB_HEX, 4);

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

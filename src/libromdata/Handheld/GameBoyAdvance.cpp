/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameBoyAdvance.hpp: Nintendo Game Boy Advance ROM reader.               *
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

#include "GameBoyAdvance.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "gba_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(GameBoyAdvance)

class GameBoyAdvancePrivate : public RomDataPrivate
{
	public:
		GameBoyAdvancePrivate(GameBoyAdvance *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameBoyAdvancePrivate)

	public:
		enum RomType {
			ROM_UNKNOWN		= -1,	// Unknown ROM type.
			ROM_GBA			= 0,	// Standard GBA ROM.
			ROM_GBA_PASSTHRU	= 1,	// Unlicensed GBA pass-through cartridge.
			ROM_NDS_EXP		= 2,	// Non-bootable NDS expansion ROM.
		};

		// ROM type.
		int romType;

		// ROM header.
		GBA_RomHeader romHeader;
};

/** GameBoyAdvancePrivate **/

GameBoyAdvancePrivate::GameBoyAdvancePrivate(GameBoyAdvance *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** GameBoyAdvance **/

/**
 * Read a Nintendo Game Boy Advance ROM image.
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
GameBoyAdvance::GameBoyAdvance(IRpFile *file)
	: super(new GameBoyAdvancePrivate(this, file))
{
	RP_D(GameBoyAdvance);
	d->className = "GameBoyAdvance";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for GBA.
	info.szFile = 0;	// Not needed for GBA.
	d->romType = isRomSupported_static(&info);

	d->isValid = (d->romType >= 0);
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameBoyAdvance::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GBA_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the first 16 bytes of the Nintendo logo.
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};

	const GBA_RomHeader *const gba_header =
		reinterpret_cast<const GBA_RomHeader*>(info->header.pData);
	if (!memcmp(gba_header->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
		// Nintendo logo is present at the correct location.
		return GameBoyAdvancePrivate::ROM_GBA;
	} else if (gba_header->fixed_96h == 0x96 && gba_header->device_type == 0x00) {
		// This may be an expansion cartridge for a DS game.
		// These cartridges don't have the logo data, so they
		// aren't bootable as a GBA game.

		// Verify the header checksum.
		uint8_t chk = 0;
		for (int i = 0xA0; i <= 0xBC; i++) {
			chk -= info->header.pData[i];
		}
		chk -= 0x19;
		if (chk == gba_header->checksum) {
			// Header checksum is correct.
			// This is either a Nintendo DS expansion cartridge
			// or an unlicensed pass-through cartridge, e.g.
			// "Action Replay".

			// The entry point for expansion cartridges is 0xFFFFFFFF.
			if (gba_header->entry_point == cpu_to_le32(0xFFFFFFFF)) {
				// This is a Nintendo DS expansion cartridge.
				return GameBoyAdvancePrivate::ROM_NDS_EXP;
			} else {
				// This is an unlicensed pass-through cartridge.
				return GameBoyAdvancePrivate::ROM_GBA_PASSTHRU;
			}
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameBoyAdvance::systemName(unsigned int type) const
{
	RP_D(const GameBoyAdvance);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameBoyAdvance::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Game Boy Advance",
		"Game Boy Advance",
		"GBA",
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
const char *const *GameBoyAdvance::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gba",	// Most common
		".agb",	// Less common
		".mb",	// Multiboot (may conflict with AutoDesk Maya)
		".srl",	// Official SDK extension

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameBoyAdvance::loadFieldData(void)
{
	RP_D(GameBoyAdvance);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// GBA ROM header.
	const GBA_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(5);	// Maximum of 5 fields.

	// Game title.
	d->fields->addField_string(C_("GameBoyAdvance", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)));

	// Game ID.
	// Replace any non-printable characters with underscores.
	// (Action Replay has ID6 "\0\0\0\001".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (isprint(romHeader->id6[i])
			? romHeader->id6[i]
			: '_');
	}
	id6[6] = 0;
	d->fields->addField_string(C_("GameBoyAdvance", "Game ID"), latin1_to_utf8(id6, 6));

	// Look up the publisher.
	const char *const publisher = NintendoPublishers::lookup(romHeader->company);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (isalnum(romHeader->company[0]) &&
		    isalnum(romHeader->company[1]))
		{
			s_publisher = rp_sprintf(C_("GameBoyAdvance", "Unknown (%.2s)"),
				romHeader->company);
		} else {
			s_publisher = rp_sprintf(C_("GameBoyAdvance", "Unknown (%02X %02X)"),
				(uint8_t)romHeader->company[0],
				(uint8_t)romHeader->company[1]);
		}
	}
	d->fields->addField_string(C_("GameBoyAdvance", "Publisher"), s_publisher);

	// ROM version.
	d->fields->addField_string_numeric(C_("GameBoyAdvance", "Revision"),
		romHeader->rom_version, RomFields::FB_DEC, 2);

	// Entry point.
	switch (d->romType) {
		case GameBoyAdvancePrivate::ROM_GBA:
		case GameBoyAdvancePrivate::ROM_GBA_PASSTHRU:
			if (romHeader->entry_point_bytes[3] == 0xEA) {
				// Unconditional branch instruction.
				// NOTE: Due to pipelining, the actual branch is 2 words
				// after the specified branch offset.
				uint32_t entry_point = ((le32_to_cpu(romHeader->entry_point) + 2) & 0xFFFFFF) << 2;
				// Handle signed values.
				if (entry_point & 0x02000000) {
					entry_point |= 0xFC000000;
				}
				d->fields->addField_string_numeric(C_("GameBoyAdvance", "Entry Point"),
					entry_point, RomFields::FB_HEX, 8,
					RomFields::STRF_MONOSPACE);
			} else {
				// Non-standard entry point instruction.
				d->fields->addField_string_hexdump(C_("GameBoyAdvance", "Entry Point"),
					romHeader->entry_point_bytes, 4,
					RomFields::STRF_MONOSPACE);
			}
			break;

		case GameBoyAdvancePrivate::ROM_NDS_EXP:
			// Not bootable.
			d->fields->addField_string(C_("GameBoyAdvance", "Entry Point"),
				C_("GameBoyAdvance", "Not bootable (Nintendo DS expansion)"));
			break;

		default:
			// Unknown ROM type.
			d->fields->addField_string(C_("GameBoyAdvance", "Entry Point"),
				C_("GameBoyAdvance", "Unknown"));
			break;
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

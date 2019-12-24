/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PokemonMini.cpp: Pokémon Mini ROM reader.                               *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "PokemonMini.hpp"
#include "librpbase/RomData_p.hpp"

#include "pkmnmini_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// libi18n
#include "libi18n/i18n.h"

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(PokemonMini)

class PokemonMiniPrivate : public RomDataPrivate
{
	public:
		PokemonMiniPrivate(PokemonMini *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PokemonMiniPrivate)

	public:
		// ROM header.
		PokemonMini_RomHeader romHeader;
};

/** PokemonMiniPrivate **/

PokemonMiniPrivate::PokemonMiniPrivate(PokemonMini *q, IRpFile *file)
	: super(q, file)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** PokemonMini **/

/**
 * Read a Nintendo Game Boy Advance ROM image.
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
PokemonMini::PokemonMini(IRpFile *file)
	: super(new PokemonMiniPrivate(this, file))
{
	RP_D(PokemonMini);
	d->className = "PokemonMini";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	size_t size = d->file->seekAndRead(POKEMONMINI_HEADER_ADDRESS, &d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = POKEMONMINI_HEADER_ADDRESS;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for Pokémon Mini.
	info.szFile = 0;	// Not needed for Pokémon Mini.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PokemonMini::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData) {
		// No detection information.
		return -1;
	}

	// Detection information starts at 0x2100.
	if (info->header.addr > POKEMONMINI_HEADER_ADDRESS) {
		// Incorrect starting address.
		return -1;
	}
	if (info->header.addr + info->header.size < POKEMONMINI_HEADER_ADDRESS + sizeof(PokemonMini_RomHeader)) {
		// Not enough data.
		return -1;
	}

	const PokemonMini_RomHeader *const romHeader =
		reinterpret_cast<const PokemonMini_RomHeader*>(&info->header.pData[info->header.addr - POKEMONMINI_HEADER_ADDRESS]);

	// Check the header magic.
	if (romHeader->pm_magic != be16_to_cpu(POKEMONMINI_PM_MAGIC) &&
	    romHeader->pm_magic != be16_to_cpu(POKEMONMINI_MN_MAGIC))
	{
		// Incorrect magic.
		return -1;
	}

	// Check "NINTENDO".
	if (memcmp(romHeader->nintendo, "NINTENDO", 8) != 0) {
		// Incorrect magic.
		return -1;
	}

	// This appears to be a Pokémon Mini ROM image.
	return 0;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *PokemonMini::systemName(unsigned int type) const
{
	RP_D(const PokemonMini);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Pokémon Mini has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PokemonMini::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Pok\xC3\xA9mon Mini",
		"Pok\xC3\xA9mon Mini",
		"Pkmn Mini",
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
const char *const *PokemonMini::supportedFileExtensions_static(void)
{
	// TODO
	static const char *const exts[] = {
		".bin",

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
const char *const *PokemonMini::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"application/x-pokemon-mini-rom",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PokemonMini::loadFieldData(void)
{
	RP_D(PokemonMini);
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

	// Pokémon Mini ROM header.
	const PokemonMini_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(2);	// Maximum of 2 fields.

	// Game title.
	string title;
	if (romHeader->game_id[3] == 'J') {
		// Japanese title. Assume it's Shift-JIS.
		// TODO: Also Korea?
		title = cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title));
	} else {
		// Assume other regions are cp1252.
		title = cp1252_to_utf8(romHeader->title, sizeof(romHeader->title));
	}
	d->fields->addField_string(C_("RomData", "Title"), title);

	// Game ID.
	// Replace any non-printable characters with underscores.
	char id4[5];
	for (int i = 0; i < 4; i++) {
		id4[i] = (ISPRINT(romHeader->game_id[i])
			? romHeader->game_id[i]
			: '_');
	}
	id4[4] = 0;
	d->fields->addField_string(C_("PokemonMini", "Game ID"), latin1_to_utf8(id4, 4));

	// TODO: IRQs.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PokemonMini.cpp: Pokémon Mini ROM reader.                               *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PokemonMini.hpp"
#include "pkmnmini_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class PokemonMiniPrivate final : public RomDataPrivate
{
	public:
		PokemonMiniPrivate(PokemonMini *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PokemonMiniPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// ROM header.
		PokemonMini_RomHeader romHeader;
};

ROMDATA_IMPL(PokemonMini)

/** PokemonMiniPrivate **/

/* RomDataInfo */
const char *const PokemonMiniPrivate::exts[] = {
	".min",

	nullptr
};
const char *const PokemonMiniPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-pokemon-mini-rom",

	nullptr
};
const RomDataInfo PokemonMiniPrivate::romDataInfo = {
	"PokemonMini", exts, mimeTypes
};

PokemonMiniPrivate::PokemonMiniPrivate(PokemonMini *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** PokemonMini **/

/**
 * Read a Pokémon Mini ROM image.
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
	d->mimeType = "application/x-pokemon-mini-rom";	// unofficial, not on fd.o

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	size_t size = d->file->seekAndRead(POKEMONMINI_HEADER_ADDRESS, &d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{POKEMONMINI_HEADER_ADDRESS, sizeof(d->romHeader),
			reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for PokemonMini)
		0		// szFile (not needed for PokemonMini)
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
	if (romHeader->pm_magic != be16_to_cpu(POKEMONMINI_MN_MAGIC)) {
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
	d->fields->reserve(3);	// Maximum of 3 fields.

	// Title.
	string title;
	if (romHeader->game_id[3] == 'J') {
		// Japanese title. Assume it's Shift-JIS.
		// TODO: Also Korea?
		title = cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title));
	} else {
		// Assume other regions are cp1252.
		title = cp1252_to_utf8(romHeader->title, sizeof(romHeader->title));
	}
	d->fields->addField_string(C_("RomData", "Title"), title, RomFields::STRF_TRIM_END);

	// Game ID.
	// Replace any non-printable characters with underscores.
	char id4[5];
	for (int i = 0; i < 4; i++) {
		id4[i] = (ISPRINT(romHeader->game_id[i])
			? romHeader->game_id[i]
			: '_');
	}
	id4[4] = 0;
	d->fields->addField_string(C_("RomData", "Game ID"), latin1_to_utf8(id4, 4));

	// Vector table.
	static const char *const vectors_names[PokemonMini_IRQ_MAX] = {
		// 0
		"Reset",
		"PRC Frame Copy",
		"PRC Render",
		"Timer 2 Underflow (upper)",
		"Timer 2 Underflow (lower)",
		"Timer 1 Underflow (upper)",
		"Timer 1 Underflow (lower)",
		"Timer 3 Underflow (upper)",

		// 8
		"Timer 3 Comparator",
		"32 Hz Timer",
		"8 Hz Timer",
		"2 Hz Timer",
		"1 Hz Timer",
		"IR Receiver",
		"Shake Sensor",
		"Power Key",

		// 16
		"Right Key",
		"Left Key",
		"Down Key",
		"Up Key",
		"C Key",
		"B Key",
		"A Key",
		"Vector #23",	// undefined

		// 24
		"Vector #24",	// undefined
		"Vector #25",	// undefined
		"Cartridge",
	};

	// Vector format: CE C4 00 F3 nn nn
	// - MOV U, #00
	// - JMPW #ssss
	// NOTE: JMPW is a RELATIVE jump.
	// NOTE: PC is the value *after* the jump instruction.
	// Offset: PC = PC + #ssss - 1
	// Reference: https://github.com/OpenEmu/PokeMini-Core/blob/master/PokeMini/pokemini-code/doc/PM_Opc_JMP.html
	static const uint8_t vec_prefix[4]   = {0xCE, 0xC4, 0x00, 0xF3};
	static const uint8_t vec_empty_ff[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	static const uint8_t vec_empty_00[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	auto vv_vectors = new RomFields::ListData_t(ARRAY_SIZE(vectors_names));
	uint32_t pc = 0x2100 + offsetof(PokemonMini_RomHeader, irqs);
	for (unsigned int i = 0; i < ARRAY_SIZE(vectors_names); i++, pc += 6) {
		auto &data_row = vv_vectors->at(i);
		data_row.reserve(3);

		// # (decimal)
		data_row.emplace_back(rp_sprintf("%u", i));

		// Vector name
		data_row.emplace_back(vectors_names[i]);

		// Address
		string s_address;
		if (!memcmp(&romHeader->irqs[i][0], vec_prefix, sizeof(vec_prefix))) {
			// Standard vector jump opcode.
			uint32_t offset = (romHeader->irqs[i][5] << 8) | romHeader->irqs[i][4];
			offset += pc + 3 + 3 - 1;
			s_address = rp_sprintf("0x%04X", offset);
		} else if (romHeader->irqs[i][0] == 0xF3) {
			// JMPW without MOV U.
			// Seen in some homebrew.
			uint32_t offset = (romHeader->irqs[i][2] << 8) | romHeader->irqs[i][1];
			offset += pc + 3 - 1;
			s_address = rp_sprintf("0x%04X", offset);
		} else if (!memcmp(&romHeader->irqs[i][0], vec_empty_ff, sizeof(vec_empty_ff)) ||
			   !memcmp(&romHeader->irqs[i][0], vec_empty_00, sizeof(vec_empty_00))) {
			// Empty vector.
			s_address = C_("RomData|VectorTable", "None");
		} else {
			// Not a standard jump opcode.
			// Show the hexdump.
			s_address = rp_sprintf("%02X %02X %02X %02X %02X %02X",
				romHeader->irqs[i][0], romHeader->irqs[i][1],
				romHeader->irqs[i][2], romHeader->irqs[i][3],
				romHeader->irqs[i][4], romHeader->irqs[i][5]);
		}
		data_row.emplace_back(std::move(s_address));
	}

	static const char *const vectors_headers[] = {
		NOP_C_("RomData|VectorTable", "#"),
		NOP_C_("RomData|VectorTable", "Vector"),
		NOP_C_("RomData|VectorTable", "Address"),
	};
	vector<string> *const v_vectors_headers = RomFields::strArrayToVector_i18n(
		"RomData|VectorTable", vectors_headers, ARRAY_SIZE(vectors_headers));

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
	params.headers = v_vectors_headers;
	params.data.single = vv_vectors;
	d->fields->addField_listData(C_("RomData", "Vector Table"), &params);

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PokemonMini::loadMetaData(void)
{
	RP_D(PokemonMini);
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

	// Pokémon Mini ROM header.
	const PokemonMini_RomHeader *const romHeader = &d->romHeader;

	// Title.
	string title;
	if (romHeader->game_id[3] == 'J') {
		// Japanese title. Assume it's Shift-JIS.
		// TODO: Also Korea?
		title = cp1252_sjis_to_utf8(romHeader->title, sizeof(romHeader->title));
	} else {
		// Assume other regions are cp1252.
		title = cp1252_to_utf8(romHeader->title, sizeof(romHeader->title));
	}
	d->metaData->addMetaData_string(Property::Title, title, RomMetaData::STRF_TRIM_END);

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}

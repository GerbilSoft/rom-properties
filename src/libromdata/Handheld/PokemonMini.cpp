/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PokemonMini.cpp: Pokémon Mini ROM reader.                               *
 *                                                                         *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PokemonMini.hpp"
#include "pkmnmini_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class PokemonMiniPrivate final : public RomDataPrivate
{
public:
	explicit PokemonMiniPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(PokemonMiniPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 1+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	PokemonMini_RomHeader romHeader;

public:
	/**
	 * Get the title.
	 * @return Title
	 */
	string getTitle(void) const;

	/**
	 * Get the game ID, with unprintable characters replaced with '_'.
	 * @return Game ID
	 */
	inline string getGameID(void) const;
};

ROMDATA_IMPL(PokemonMini)

/** PokemonMiniPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> PokemonMiniPrivate::exts = {{
	".min",

	nullptr
}};
const array<const char*, 1+1> PokemonMiniPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"application/x-pokemon-mini-rom",

	nullptr
}};
const RomDataInfo PokemonMiniPrivate::romDataInfo = {
	"PokemonMini", exts.data(), mimeTypes.data()
};

PokemonMiniPrivate::PokemonMiniPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get the title.
 * @return Title
 */
string PokemonMiniPrivate::getTitle(void) const
{
	if (romHeader.game_id[3] == 'J') {
		// Japanese title. Assume it's Shift-JIS.
		// TODO: Also Korea?
		return cp1252_sjis_to_utf8(romHeader.title, sizeof(romHeader.title));
	} else {
		// Assume other regions are cp1252.
		return cp1252_to_utf8(romHeader.title, sizeof(romHeader.title));
	}
}

/**
 * Get the game ID, with unprintable characters replaced with '_'.
 * @return Game ID
 */
inline string PokemonMiniPrivate::getGameID(void) const
{
	// Replace any non-printable characters with underscores.
	string id4;
	id4.resize(4, '_');
	for (size_t i = 0; i < 4; i++) {
		if (ISPRINT(romHeader.game_id[i])) {
			id4[i] = romHeader.game_id[i];
		}
	}
	return id4;
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
PokemonMini::PokemonMini(const IRpFilePtr &file)
	: super(new PokemonMiniPrivate(file))
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
		d->file.reset();
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
		d->file.reset();
		return;
	}

	// Is PAL?
	d->isPAL = (d->romHeader.game_id[3] == 'P');
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

	static const array<const char*, 4> sysNames = {{
		"Pok\xC3\xA9mon Mini", "Pok\xC3\xA9mon Mini", "Pkmn Mini", nullptr
	}};

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
	if (!d->fields.empty()) {
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
	d->fields.reserve(3);	// Maximum of 3 fields.

	// Title
	d->fields.addField_string(C_("RomData", "Title"), d->getTitle(), RomFields::STRF_TRIM_END);

	// Game ID
	d->fields.addField_string(C_("RomData", "Game ID"), d->getGameID());

	// Vector table.
	static const array<const char*, PokemonMini_IRQ_MAX> vectors_names = {{
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
	}};

	// Vector format: CE C4 00 F3 nn nn
	// - MOV U, #00
	// - JMPW #ssss
	// NOTE: JMPW is a RELATIVE jump.
	// NOTE: PC is the value *after* the jump instruction.
	// Offset: PC = PC + #ssss - 1
	// Reference: https://github.com/OpenEmu/PokeMini-Core/blob/master/PokeMini/pokemini-code/doc/PM_Opc_JMP.html
	static constexpr array<uint8_t, 4> vec_prefix   = {{0xCE, 0xC4, 0x00, 0xF3}};
	static constexpr array<uint8_t, 6> vec_empty_ff = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
	static constexpr array<uint8_t, 6> vec_empty_00 = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

	auto *const vv_vectors = new RomFields::ListData_t(vectors_names.size());
	uint32_t pc = 0x2100 + offsetof(PokemonMini_RomHeader, irqs);
	for (size_t i = 0; i < vectors_names.size(); i++, pc += 6) {
		auto &data_row = vv_vectors->at(i);
		data_row.reserve(3);

		// # (decimal)
		data_row.push_back(fmt::to_string(i));

		// Vector name
		data_row.emplace_back(vectors_names[i]);

		const uint8_t *const irq = romHeader->irqs[i];

		// Address
		string s_address;
		if (!memcmp(irq, vec_prefix.data(), vec_prefix.size())) {
			// Standard vector jump opcode.
			uint32_t offset = (irq[5] << 8) | irq[4];
			offset += pc + 3 + 3 - 1;
			s_address = fmt::format(FSTR("0x{:0>4X}"), offset);
		} else if (irq[0] == 0xF3) {
			// JMPW without MOV U.
			// Seen in some homebrew.
			uint32_t offset = (irq[2] << 8) | irq[1];
			offset += pc + 3 - 1;
			s_address = fmt::format(FSTR("0x{:0>4X}"), offset);
		} else if (!memcmp(irq, vec_empty_ff.data(), vec_empty_ff.size()) ||
			   !memcmp(irq, vec_empty_00.data(), vec_empty_00.size())) {
			// Empty vector.
			s_address = C_("RomData|VectorTable", "None");
		} else {
			// Not a standard jump opcode.
			// Show the hexdump.
			s_address = fmt::format(FSTR("{:0>2X} {:0>2X} {:0>2X} {:0>2X} {:0>2X} {:0>2X}"),
				irq[0], irq[1], irq[2], irq[3], irq[4], irq[5]);
		}
		data_row.push_back(std::move(s_address));
	}

	static const array<const char*, 3> vectors_headers = {{
		NOP_C_("RomData|VectorTable", "#"),
		NOP_C_("RomData|VectorTable", "Vector"),
		NOP_C_("RomData|VectorTable", "Address"),
	}};
	vector<string> *const v_vectors_headers = RomFields::strArrayToVector_i18n("RomData|VectorTable", vectors_headers);

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
	params.headers = v_vectors_headers;
	params.data.single = vv_vectors;
	d->fields.addField_listData(C_("RomData", "Vector Table"), &params);

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PokemonMini::loadMetaData(void)
{
	RP_D(PokemonMini);
	if (!d->metaData.empty()) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Pokémon Mini ROM header.
	//const PokemonMini_RomHeader *const romHeader = &d->romHeader;
	d->metaData.reserve(3);	// Maximum of 3 metadata properties.

	// Title
	d->metaData.addMetaData_string(Property::Title, d->getTitle(), RomMetaData::STRF_TRIM_END);

	/** Custom properties! **/

	// Game ID
	d->metaData.addMetaData_string(Property::GameID, d->getGameID());

	// Finished reading the metadata.
	return static_cast<int>(d->metaData.count());
}

} // namespace LibRomData

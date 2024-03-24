/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.cpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NESMappers.hpp"

// C++ STL classes
using std::array;

namespace LibRomData { namespace NESMappers {

/**
 * References:
 * - https://wiki.nesdev.com/w/index.php/Mapper
 * - https://wiki.nesdev.com/w/index.php/NES_2.0_submappers
 */

// Mirroring behaviors for different mappers
enum class NESMirroring : uint8_t {
	Unknown = 0,		// When submapper has this value, it inherits mapper's value
	// NOTE: for all of these we assume that if 4-Screen bit is set it means that the mapper's
	// logic gets ignored, and there's simply 4K of SRAM at $2000. For more complicated mappers
	// (MMC5) this would actually be a downgrade, and maybe even impossible, but iNES format
	// applies the same logic to all mappers (except 30 and 218, see below)
	// Reference: http://wiki.nesdev.com/w/index.php/NES_2.0#Hard-Wired_Mirroring
	// NOTE: H/V/A/B refers to CIRAM A10 being connected to PPU A11/A10/Vss/Vdd respectively
	// NOTE: boards that only ever existed in H or V configuration still use the H/V bit.
	Header,			// fixed H/V (the default)
	Mapper,			// Mapper-controlled (unspecified)
	MapperHVAB,		// - switchable H/V/A/B (e.g. MMC1)
	MapperHV,		// - switchable H/V     (e.g. MMC3)
	MapperAB,		// - switchable A/B     (e.g. AxROM)
	MapperMMC5,		// - arbitrary configuration with 3 NTs and fill mode
	MapperNamco163,		// - arbitrary configuration with 2 RAM and 224 ROM NTs
	MapperVRC6,		// - it's complicated (Konami games only use H/V/A/B)
	MapperJY,		// - J.Y. Company ASIC mapper (also complicated)
	MapperSunsoft4,		// - switchable H/V/A/B with 2 RAM and 128 ROM NTs
	MapperNamcot3425,	// - H but you can select how PPU A11 maps to CIRAM A10
				//   (effectively it's selectable H/A/B/swapped-H)
	MapperGTROM,		// - paged 4 screen RAM
	MapperTxSROM,		// - arbitrary configuration with 2 NTs
	MapperSachen8259,	// - switchable H/V/A/L-shaped (A10 or A11)
	MapperSachen74LS374N,	// - switchable H/V/A/L-shaped (A10 and A11)
	MapperDIS23C01,		// - switchable H/V. A on reset.
	Mapper233,		// - switchable H/V/B/L-shaped (A10 and A11)
	Mapper235,		// - switchable H/V/A
	OneScreen_A,		// fixed A
	OneScreen_B,		// fixed B
				// (the distinction is only relevant for Magic Floor)
	FourScreen,		// 4 screen regardless of header (e.g. Vs. System)
	// The following mappers interpret the header bits differently
	UNROM512,		// fixed H/V/4 or switchable A/B (mapper 30)
	BandaiFamilyTrainer,	// fixed H/V or switchable A/B (mapper 70) (see note below)
	MagicFloor,		// fixed H/V/A/B (mapper 218)

	// NOTE: fwNES describes mappers 70 and 78 as using 4 Screen bit to specify switchable A/B
	// - 70 normally has fixed H/V. For switchable A/B, 152 should be used instead.
	// - 78 has either switchable H/V or switchable A/B. Emulators default to one of those,
	// and use checksumming to detect the other. Submappers should be used instead.
};

/**
 * NES mapper data is generated using NESMappers_parser.py.
 * This file is *not* automatically updated by the build system.
 * The parser script should be run manually when the source file
 * is updated to add new mappers.
 *
 * - Source file: NESMappers_data.txt
 * - Output file: NESMappers_data.h
 */
#include "NESMappers_data.h"

/** Submappers **/

/**
 * NES 2.0 submapper information.
 *
 * `deprecated` can be one of the following:
 * - 0: Not deprecated.
 * - >0: Deprecated, and this is the replacement mapper number.
 * - 0xFFFF: Deprecated, no replacement.
 *
 * A submapper may be deprecated in favor of submapper 0, in which
 * case the `deprecated` value is equal to that mapper's number.
 * It is assumed that the replacement mapper always uses submapper 0.
 */
struct NESSubmapperInfo {
	uint8_t submapper;		// Submapper number
	uint8_t reserved;
	uint16_t deprecated;
	NESMirroring mirroring;		// Mirroring behavior
	const char *desc;		// Description
};

// Mapper 001: MMC1
static constexpr array<NESSubmapperInfo, 5> mmc1_submappers = {{
	{1, 0,   1, NESMirroring::Unknown, "SUROM"},
	{2, 0,   1, NESMirroring::Unknown, "SOROM"},
	{3, 0, 155, NESMirroring::Unknown, "MMC1A"},
	{4, 0,   1, NESMirroring::Unknown, "SXROM"},
	{5, 0,   0, NESMirroring::Unknown, "SEROM, SHROM, SH1ROM"},
}};

// Discrete logic mappers: UxROM (002), CNROM (003), AxROM (007)
static constexpr array<NESSubmapperInfo, 3> discrete_logic_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "Bus conflicts are unspecified"},
	{1, 0,   0, NESMirroring::Unknown, "Bus conflicts do not occur"},
	{2, 0,   0, NESMirroring::Unknown, "Bus conflicts occur, resulting in: bus AND rom"},
}};

// Mapper 004: MMC3
static constexpr array<NESSubmapperInfo, 5> mmc3_submappers = {{
	{0, 0,      0, NESMirroring::Unknown, "MMC3C"},
	{1, 0,      0, NESMirroring::Unknown, "MMC6"},
	{2, 0, 0xFFFF, NESMirroring::Unknown, "MMC3C with hard-wired mirroring"},
	{3, 0,      0, NESMirroring::Unknown, "MC-ACC"},
	{4, 0,      0, NESMirroring::Unknown, "MMC3A"},
}};

// Mapper 006: Game Doctor Mode 1
static constexpr array<NESSubmapperInfo, 8> mapper006_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "UNROM"},
	{1, 0,   0, NESMirroring::Unknown, "UN1ROM + CHRSW"},
	{2, 0,   0, NESMirroring::Unknown, "UOROM"},
	{3, 0,   0, NESMirroring::Unknown, "Reverse UOROM + CHRSW"},
	{4, 0,   0, NESMirroring::Unknown, "GNROM"},
	{5, 0,   0, NESMirroring::Unknown, "CNROM-256"},
	{6, 0,   0, NESMirroring::Unknown, "CNROM-128"},
	{7, 0,   0, NESMirroring::Unknown, "NROM-256"},
}};

// Mapper 016: Bandai FCG-x
static constexpr array<NESSubmapperInfo, 5> bandai_fcgx_submappers = {{
	{1, 0, 159, NESMirroring::Unknown, "LZ93D50 with 24C01"},
	{2, 0, 157, NESMirroring::Unknown, "Datach Joint ROM System"},
	{3, 0, 153, NESMirroring::Unknown, "8 KiB of WRAM instead of serial EEPROM"},
	{4, 0,   0, NESMirroring::Unknown, "FCG-1/2"},
	{5, 0,   0, NESMirroring::Unknown, "LZ93D50 with optional 24C02"},
}};

// Mapper 019: Namco 129, 163
static constexpr array<NESSubmapperInfo, 6> namco_129_164_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "Expansion sound volume unspecified"},
	{1, 0,  19, NESMirroring::Unknown, "Internal RAM battery-backed; no expansion sound"},
	{2, 0,   0, NESMirroring::Unknown, "No expansion sound"},
	{3, 0,   0, NESMirroring::Unknown, "N163 expansion sound: 11.0-13.0 dB louder than NES APU"},
	{4, 0,   0, NESMirroring::Unknown, "N163 expansion sound: 16.0-17.0 dB louder than NES APU"},
	{5, 0,   0, NESMirroring::Unknown, "N163 expansion sound: 18.0-19.5 dB louder than NES APU"},
}};

// Mapper 021: Konami VRC4c, VRC4c
static constexpr array<NESSubmapperInfo, 2> vrc4a_vrc4c_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "VRC4a"},
	{2, 0,   0, NESMirroring::Unknown, "VRC4c"},
}};

// Mapper 023: Konami VRC4e, VRC4f, VRC2b
static constexpr array<NESSubmapperInfo, 3> vrc4ef_vrc2b_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "VRC4f"},
	{2, 0,   0, NESMirroring::Unknown, "VRC4e"},
	{3, 0,   0, NESMirroring::Unknown, "VRC2b"},
}};

// Mapper 025: Konami VRC4b, VRC4d, VRC2c
static constexpr array<NESSubmapperInfo, 3> vrc4bd_vrc2c_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "VRC4b"},
	{2, 0,   0, NESMirroring::Unknown, "VRC4d"},
	{3, 0,   0, NESMirroring::Unknown, "VRC2c"},
}};

// Mapper 032: Irem G101
static constexpr array<NESSubmapperInfo, 2> irem_g101_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "Programmable mirroring"},
	{1, 0,   0, NESMirroring::OneScreen_B, "Fixed one-screen mirroring"},
}};

// Mapper 034: BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
static constexpr array<NESSubmapperInfo, 2> bnrom_nina001_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "NINA-001"},
	{2, 0,   0, NESMirroring::Unknown, "BNROM"},
}};

// Mapper 068: Sunsoft-4
static constexpr array<NESSubmapperInfo, 1> sunsoft4_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "Dual Cartridge System (NTB-ROM)"},
}};

// Mapper 071: Codemasters
static constexpr array<NESSubmapperInfo, 1> codemasters_submappers = {{
	{1, 0,   0, NESMirroring::MapperAB, "Programmable one-screen mirroring (Fire Hawk)"},
}};

// Mapper 078: Cosmo Carrier / Holy Diver
static constexpr array<NESSubmapperInfo, 3> mapper078_submappers = {{
	{1, 0,      0, NESMirroring::MapperAB, "Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)"},
	{2, 0, 0xFFFF, NESMirroring::Unknown, "Fixed vertical mirroring + WRAM"},
	{3, 0,      0, NESMirroring::MapperHV, "Programmable H/V mirroring (Holy Diver)"},
}};

// Mapper 083: Cony/Yoko
static constexpr array<NESSubmapperInfo, 3> cony_yoko_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "1 KiB CHR-ROM banking, no WRAM"},
	{1, 0,   0, NESMirroring::Unknown, "2 KiB CHR-ROM banking, no WRAM"},
	{2, 0,   0, NESMirroring::Unknown, "1 KiB CHR-ROM banking, 32 KiB banked WRAM"},
}};

// Mapper 108: FDS conversions
static constexpr array<NESSubmapperInfo, 4> mapper108_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "DH-08: Bubble Bobble (LH31)"},
	{2, 0,   0, NESMirroring::Unknown, "Bubble Bobble (LH31) (CHR-RAM)"},
	{3, 0,   0, NESMirroring::Unknown, "Falsion (LH54); Meikyuu Jiin Dababa (LH28)"},
	{4, 0,   0, NESMirroring::Unknown, "Pro Wrestling (LE05)"},
}};

// Mapper 114: Sugar Softec/Hosenkan
static constexpr array<NESSubmapperInfo, 2> mapper114_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "MMC3 registers: 0,3,1,5,6,7,2,4"},
	{1, 0,   0, NESMirroring::Unknown, "MMC3 registers: 0,2,5,3,6,1,7,4"},
}};

// Mapper 197: Kǎshèng (MMC3 clone)
static constexpr array<NESSubmapperInfo, 4> mapper197_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "Super Fighter III (PRG-ROM CRC32 0xC333F621)"},
	{1, 0,   0, NESMirroring::Unknown, "Super Fighter III (PRG-ROM CRC32 0x2091BEB2)"},
	{2, 0,   0, NESMirroring::Unknown, "Mortal Kombat III Special"},
	{3, 0,   0, NESMirroring::Unknown, "1995 Super 2-in-1"},
}};

// Mapper 210: Namcot 175, 340
static constexpr array<NESSubmapperInfo, 2> namcot_175_340_submappers = {{
	{1, 0,   0, NESMirroring::Header,     "Namcot 175 (fixed mirroring)"},
	{2, 0,   0, NESMirroring::MapperHVAB, "Namcot 340 (programmable mirroring)"},
}};

// Mapper 215: Sugar Softec
static constexpr array<NESSubmapperInfo, 2> sugar_softec_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "UNL-8237"},
	{1, 0,   0, NESMirroring::Unknown, "UNL-8237A"},
}};

// Mapper 232: Codemasters Quattro
static constexpr array<NESSubmapperInfo, 1> quattro_submappers = {{
	{1, 0,   0, NESMirroring::Unknown, "Aladdin Deck Enhancer"},
}};

// Mapper 256: OneBus Famiclones
static constexpr array<NESSubmapperInfo, 7> onebus_submappers = {{
	{ 1, 0,   0, NESMirroring::Unknown, "Waixing VT03"},
	{ 2, 0,   0, NESMirroring::Unknown, "Power Joy Supermax"},
	{ 3, 0,   0, NESMirroring::Unknown, "Zechess/Hummer Team"},
	{ 4, 0,   0, NESMirroring::Unknown, "Sports Game 69-in-1"},
	{ 5, 0,   0, NESMirroring::Unknown, "Waixing VT02"},
	{14, 0,   0, NESMirroring::Unknown, "Karaoto"},
	{15, 0,   0, NESMirroring::Unknown, "Jungletac"},
}};

// Mapper 268: SMD132/SMD133
static constexpr array<NESSubmapperInfo, 2> smd132_smd133_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "COOLBOY ($6000-$7FFF)"},
	{1, 0,   0, NESMirroring::Unknown, "MINDKIDS ($5000-$5FFF)"},
}};

// Mapper 313: Reset-based multicart (MMC3)
static constexpr array<NESSubmapperInfo, 5> mapper313_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "Game size: 128 KiB PRG, 128 KiB CHR"},
	{1, 0,   0, NESMirroring::Unknown, "Game size: 256 KiB PRG, 128 KiB CHR"},
	{2, 0,   0, NESMirroring::Unknown, "Game size: 128 KiB PRG, 256 KiB CHR"},
	{3, 0,   0, NESMirroring::Unknown, "Game size: 256 KiB PRG, 256 KiB CHR"},
	{4, 0,   0, NESMirroring::Unknown, "Game size: 256 KiB PRG (first game); 128 KiB PRG (other games); 128 KiB CHR"},
}};

// Mapper 407: Win, Lose, or Draw Plug-n-Play (VT03)
static constexpr array<NESSubmapperInfo, 1> mapper407_submappers = {{
	{15, 0,   0, NESMirroring::Unknown, "Opcode encryption (see mapper 256, submapper 15)"},
}};

// Mapper 444: NC7000M multicart (MMC3-compatible)
static constexpr array<NESSubmapperInfo, 4> mapper444_submappers = {{
	{0, 0,   0, NESMirroring::Unknown, "CHR A17 to $6000.0; CHR A18 to $6000.1"},
	{1, 0,   0, NESMirroring::Unknown, "CHR A17 to MMC3 CHR A17; CHR A18 to $6000.1"},
	{2, 0,   0, NESMirroring::Unknown, "CHR A17 to $6000.0; CHR A18 to $6000.4"},
	{3, 0,   0, NESMirroring::Unknown, "CHR A17 to MMC3 CHR A17; CHR A18 to $6000.4"},
}};

// Mapper 458: K-3102 and GN-23 multicart PCBs (MMC3-based)
static constexpr array<NESSubmapperInfo, 2> mapper458_submappers = {{
	{0, 0,   0, NESMirroring::MapperHV, "K-3102"},
	{1, 0,   0, NESMirroring::MapperHV, "GN-23"},
}};

/**
 * NES 2.0 submapper list.
 *
 * NOTE: Submapper 0 is optional, since it's the default behavior.
 * It may be included if NES 2.0 submapper 0 acts differently from
 * iNES mappers.
 */

struct NESSubmapperEntry {
	uint16_t mapper;		// Mapper number.
	uint16_t info_size;		// Number of entries in info.
	const NESSubmapperInfo *info;	// Submapper information.
};

#define NES2_SUBMAPPER(num, arr) {num, (uint16_t)((arr).size()), (arr).data()}
static constexpr array<NESSubmapperEntry, 31> submappers = {{
	NES2_SUBMAPPER(  1, mmc1_submappers),			// MMC1
	NES2_SUBMAPPER(  2, discrete_logic_submappers),		// UxROM
	NES2_SUBMAPPER(  3, discrete_logic_submappers),		// CNROM
	NES2_SUBMAPPER(  4, mmc3_submappers),			// MMC3
	NES2_SUBMAPPER(  6, mapper006_submappers),		// Game Doctor
	NES2_SUBMAPPER(  7, discrete_logic_submappers),		// AxROM
	NES2_SUBMAPPER( 16, bandai_fcgx_submappers),		// FCG-x
	NES2_SUBMAPPER( 19, namco_129_164_submappers),		// Namco 129/164
	NES2_SUBMAPPER( 21, vrc4a_vrc4c_submappers),		// Konami VRC4a, VRC4c
	NES2_SUBMAPPER( 23, vrc4ef_vrc2b_submappers),		// Konami VRC4e, VRC4f, VRC2b
	NES2_SUBMAPPER( 25, vrc4bd_vrc2c_submappers),		// Konami VRC4b, VRC4d, VRC2c
	NES2_SUBMAPPER( 32, irem_g101_submappers),		// Irem G101
	NES2_SUBMAPPER( 34, bnrom_nina001_submappers),		// BNROM / NINA-001
	NES2_SUBMAPPER( 68, sunsoft4_submappers),		// Sunsoft-4
	NES2_SUBMAPPER( 71, codemasters_submappers),		// Codemasters
	NES2_SUBMAPPER( 78, mapper078_submappers),
	NES2_SUBMAPPER( 83, cony_yoko_submappers),		// Cony/Yoko
	NES2_SUBMAPPER(108, mapper108_submappers),		// FDS conversions
	NES2_SUBMAPPER(114, mapper114_submappers),
	NES2_SUBMAPPER(197, mapper197_submappers),		// Kǎshèng (MMC3 clone)
	NES2_SUBMAPPER(210, namcot_175_340_submappers),		// Namcot 175, 340
	NES2_SUBMAPPER(215, sugar_softec_submappers),		// Sugar Softec (MMC3 clone)
	NES2_SUBMAPPER(232, quattro_submappers),		// Codemasters Quattro
	NES2_SUBMAPPER(256, onebus_submappers),			// OneBus Famiclones
	NES2_SUBMAPPER(268, smd132_smd133_submappers),		// SMD132/SMD133
	NES2_SUBMAPPER(313, mapper313_submappers),		// Reset-based multicart (MMC3)
	NES2_SUBMAPPER(407, mapper407_submappers),		// Win, Lose, or Draw Plug-n-Play (VT03)
	NES2_SUBMAPPER(444, mapper444_submappers),		// NC7000M multicart (MMC3-compatible)
	NES2_SUBMAPPER(458, mapper458_submappers),		// K-3102 / GN-23 multicart (MMC3-based)
	NES2_SUBMAPPER(561, mapper006_submappers),		// Bung Super Game Doctor
	NES2_SUBMAPPER(562, mapper006_submappers),		// Venus Turbo Game Doctor
}};

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper info, or nullptr if not found.
 */
const NESMapperEntry *lookup_ines_info(int mapper)
{
	assert(mapper >= 0);
	if (mapper < 0) {
		// Mapper number is out of range.
		return nullptr;
	}

	if (mapper >= ARRAY_SIZE_I(NESMappers_offtbl)) {
		// Mapper number is out of range.
		return nullptr;
	} else if (NESMappers_offtbl[mapper].name_idx == 0) {
		// Unused mapper number.
		return nullptr;
	}

	return &NESMappers_offtbl[mapper];
}

/**
 * Look up an NES 2.0 submapper number.
 * @param mapper Mapper number.
 * @param submapper Submapper number.
 * @return Submapper info, or nullptr if not found.
 */
const NESSubmapperInfo *lookup_nes2_submapper_info(int mapper, int submapper)
{
	assert(mapper >= 0);
	assert(submapper >= 0);
	assert(submapper < 16);
	if (mapper < 0 || submapper < 0 || submapper >= 16) {
		// Mapper or submapper number is out of range.
		return nullptr;
	}

	// Do a binary search in submappers[].
	auto pSubmapper = std::lower_bound(submappers.cbegin(), submappers.cend(), static_cast<uint16_t>(mapper),
		[](const NESSubmapperEntry &submapper, uint16_t mapper) noexcept -> bool {
			return (submapper.mapper < mapper);
		});
	if (pSubmapper == submappers.cend() || pSubmapper->mapper != mapper ||
	    !pSubmapper->info || pSubmapper->info_size == 0)
	{
		return nullptr;
	}

	// Do a binary search in pSubmapper->info.
	const NESSubmapperInfo *const pInfo_end =
		&pSubmapper->info[pSubmapper->info_size];
	auto pSubmapperInfo = std::lower_bound(pSubmapper->info, pInfo_end, static_cast<uint8_t>(submapper),
		[](const NESSubmapperInfo &submapperInfo, uint8_t submapper) noexcept -> bool {
			return (submapperInfo.submapper < submapper);
		});
	if (pSubmapperInfo == pInfo_end || pSubmapperInfo->submapper != submapper) {
		return nullptr;
	}
	return pSubmapperInfo;
}

/**
 * Get an entry from the string table.
 * @param idx String table index
 * @return String, or nullptr if 0 or out of bounds.
 */
static inline const char *get_from_strtbl(unsigned int idx)
{
	assert(idx < ARRAY_SIZE(NESMappers_strtbl));
	if (idx == 0 || idx >= ARRAY_SIZE(NESMappers_strtbl))
		return nullptr;

	return &NESMappers_strtbl[idx];
}

/** Public functions **/

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper name, or nullptr if not found.
 */
const char *lookup_ines(int mapper)
{
	const NESMapperEntry *const ent = lookup_ines_info(mapper);
	return (ent ? get_from_strtbl(ent->name_idx) : nullptr);
}

/**
 * Convert a TNES mapper number to iNES.
 * @param tnes_mapper TNES mapper number.
 * @return iNES mapper number, or -1 if unknown.
 */
int tnesMapperToInesMapper(int tnes_mapper)
{
	// 255 == not supported
	static const std::array<uint8_t, 52> ines_mappers = {
		// 0
		0,	// NROM
		1,	// SxROM (MMC1)
		9,	// PxROM (MMC2)
		4,	// TxROM (MMC3)
		10,	// FxROM (MMC4)
		5,	// ExROM (MMC5)
		2,	// UxROM
		3,	// CNROM
		66,	// GNROM
		7,	// AxROM

		// 10
		184,	// Sunsoft-1
		89,	// Sunsoft-2
		67,	// Sunsoft-3
		68,	// Sunsoft-4
		69,	// Sunsoft-5
		70,	// Bandai
		75,	// Konami VRC1
		22,	// Konami VRC2A
		23,	// Konami VRC2B
		73,	// Konami VRC3

		// 20
		21,	// Konami VRC4A
		25,	// Konami VRC4B
		255,	// Konami VRC4C (FIXME: Submapper?)
		255,	// Konami VRC4D (FIXME: Submapper?)
		255,	// Konami VRC4E (FIXME: Submapper?)
		24,	// Konami VRC6A
		26,	// Konami VRC6B
		85,	// Konami VRC7
		87,	// Jaleco-1
		48,	// Jaleco-2

		// 30
		92,	// Jaleco-3
		86,	// Jaleco-4
		18,	// Jaleco-SS8806
		93,	// 74161A
		94,	// 74161B
		95,	// Namcot
		19,	// Namcot-106
		76,	// Namco-109
		88,	// Namco-118
		118,	// MMC-3+TLS

		// 40
		33,	// Taito-TC0190
		255,	// Taito-TC0350 (FIXME: Submapper?)
		48,	// Taito-TC190V
		80,	// Taito-X-005
		82,	// Taito-X1-17
		77,	// Irem-1
		97,	// Irem-2
		78,	// Irem-74HC161
		255,	// Irem-74HC32 (FIXME: Submapper?)
		32,	// Irem-G-101

		// 50
		65,	// Irem-H-3001
		185,	// VROM-disable
	};

	if (tnes_mapper < 0 || tnes_mapper >= static_cast<int>(ines_mappers.size())) {
		// Undefined TNES mapper.
		return -1;
	} else if (ines_mappers[tnes_mapper] == 255) {
		// Not supported.
		return -1;
	}

	return ines_mappers[tnes_mapper];
}

/**
 * Look up an NES 2.0 submapper number.
 * TODO: Return the "depcrecated" value?
 * @param mapper Mapper number.
 * @param submapper Submapper number.
 * @return Submapper name, or nullptr if not found.
 */
const char *lookup_nes2_submapper(int mapper, int submapper)
{
	const NESSubmapperInfo *const ent = lookup_nes2_submapper_info(mapper, submapper);
	// TODO: Return the "deprecated" value?
	return ent ? ent->desc : nullptr;
}

/**
 * Look up a description of mapper mirroring behavior
 * @param mapper Mapper number.
 * @param submapper Submapper number.
 * @param vert Vertical bit in the iNES header
 * @param vert Four-screen bit in the iNES header
 * @return String describing the mirroring behavior
 */
const char *lookup_ines_mirroring(int mapper, int submapper, bool vert, bool four)
{
	const NESMapperEntry *const ent1 = lookup_ines_info(mapper);
	const NESSubmapperInfo *const ent2 = lookup_nes2_submapper_info(mapper, submapper);
	NESMirroring mirror = ent1 ? ent1->mirroring : NESMirroring::Unknown;

	const NESMirroring submapper_mirror = ent2 ? ent2->mirroring :NESMirroring::Unknown;
	if (submapper_mirror != NESMirroring::Unknown) // Override mapper's value
		mirror = submapper_mirror;

	// Handle some special cases
	switch (mirror) {
		case NESMirroring::Unknown:
			// Default to showing the header flags
			mirror = four ? NESMirroring::FourScreen : NESMirroring::Header;
			break;
		case NESMirroring::UNROM512:
			// xxxx0xx0 - Horizontal mirroring
			// xxxx0xx1 - Vertical mirroring
			// xxxx1xx0 - Mapper-controlled, single screen
			// xxxx1xx1 - Four screens
			mirror = four ? (vert ? NESMirroring::FourScreen : NESMirroring::MapperAB) : NESMirroring::Header;
			break;
		case NESMirroring::BandaiFamilyTrainer:
			// Mapper 152 should be used instead of setting 4sc on this mapper (70).
			mirror = four ? NESMirroring::MapperAB : NESMirroring::Header;
			break;
		case NESMirroring::MagicFloor:
			// Magic Floor maps CIRAM across the entire PPU address space
			// CIRAM A10:   A10  A11  A12  A13
			// iNES flag6: 0xx1 0xx0 1xx0 1xx1
			//       $0000   aB   aa   aa   aa < pattern table 1
			//       $0800   aB   BB   aa   aa
			//       $1000   aB   aa   BB   aa < pattern table 2
			//       $1800   aB   BB   BB   aa
			//       $2000   aB   aa   aa   BB < nametables
			//       $2800   aB   BB   aa   BB
			//       $3000   aB   aa   BB   BB < unused memory
			//       $3800   aB   BB   BB   BB
			// Mirroring:  Vert Hori 1scA 1scB
			//
			// It's important to differentiate between 1scA and 1scB as it affects
			// pattern table mapping.
			mirror = four ? (vert ? NESMirroring::OneScreen_B : NESMirroring::OneScreen_A)
				      : NESMirroring::Header;
			break;
		default:
			// In other modes, four screens overrides everything else
			if (four)
				mirror = NESMirroring::FourScreen;
			break;
	}

	// NOTE: to prevent useless noise like "Mapper: MMC5, Mirroring: MMC5-like", all of the weird
	// mirroring types are grouped under "Mapper-controlled"
	switch (mirror) {
		case NESMirroring::Header:
			return vert ? C_("NES|Mirroring", "Vertical") : C_("NES|Mirroring", "Horizontal");
		case NESMirroring::Mapper:
		default:
			return C_("NES|Mirroring", "Mapper-controlled");
		case NESMirroring::MapperHVAB:
			return C_("NES|Mirroring", "Mapper-controlled (H/V/A/B)");
		case NESMirroring::MapperHV:
			return C_("NES|Mirroring", "Mapper-controlled (H/V)");
		case NESMirroring::MapperAB:
			return C_("NES|Mirroring", "Mapper-controlled (A/B)");
		case NESMirroring::OneScreen_A:
			return C_("NES|Mirroring", "Single Screen (A)");
		case NESMirroring::OneScreen_B:
			return C_("NES|Mirroring", "Single Screen (B)");
		case NESMirroring::FourScreen:
			return C_("NES|Mirroring", "Four Screens");
	}
}

} }

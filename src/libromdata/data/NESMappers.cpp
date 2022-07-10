/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.cpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NESMappers.hpp"

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

/** Submappers. **/

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
	uint8_t submapper;		// Submapper number.
	uint8_t reserved;
	uint16_t deprecated;
	const char8_t *desc;		// Description.
	NESMirroring mirroring;		// Mirroring behavior.
};

// Mapper 001: MMC1
const struct NESSubmapperInfo mmc1_submappers[] = {
	{1, 0,   1, U8("SUROM"), NESMirroring::Unknown},
	{2, 0,   1, U8("SOROM"), NESMirroring::Unknown},
	{3, 0, 155, U8("MMC1A"), NESMirroring::Unknown},
	{4, 0,   1, U8("SXROM"), NESMirroring::Unknown},
	{5, 0,   0, U8("SEROM, SHROM, SH1ROM"), NESMirroring::Unknown},
};

// Discrete logic mappers: UxROM (002), CNROM (003), AxROM (007)
const struct NESSubmapperInfo discrete_logic_submappers[] = {
	{0, 0,   0, U8("Bus conflicts are unspecified"), NESMirroring::Unknown},
	{1, 0,   0, U8("Bus conflicts do not occur"), NESMirroring::Unknown},
	{2, 0,   0, U8("Bus conflicts occur, resulting in: bus AND rom"), NESMirroring::Unknown},
};

// Mapper 004: MMC3
const struct NESSubmapperInfo mmc3_submappers[] = {
	{0, 0,      0, U8("MMC3C"), NESMirroring::Unknown},
	{1, 0,      0, U8("MMC6"), NESMirroring::Unknown},
	{2, 0, 0xFFFF, U8("MMC3C with hard-wired mirroring"), NESMirroring::Unknown},
	{3, 0,      0, U8("MC-ACC"), NESMirroring::Unknown},
	{4, 0,      0, U8("MMC3A"), NESMirroring::Unknown},
};

// Mapper 006: Game Doctor Mode 1
const struct NESSubmapperInfo mapper006_submappers[] = {
	{0, 0,   0, U8("UNROM"), NESMirroring::Unknown},
	{1, 0,   0, U8("UN1ROM + CHRSW"), NESMirroring::Unknown},
	{2, 0,   0, U8("UOROM"), NESMirroring::Unknown},
	{3, 0,   0, U8("Reverse UOROM + CHRSW"), NESMirroring::Unknown},
	{4, 0,   0, U8("GNROM"), NESMirroring::Unknown},
	{5, 0,   0, U8("CNROM-256"), NESMirroring::Unknown},
	{6, 0,   0, U8("CNROM-128"), NESMirroring::Unknown},
	{7, 0,   0, U8("NROM-256"), NESMirroring::Unknown},
};

// Mapper 016: Bandai FCG-x
const struct NESSubmapperInfo bandai_fcgx_submappers[] = {
	{1, 0, 159, U8("LZ93D50 with 24C01"), NESMirroring::Unknown},
	{2, 0, 157, U8("Datach Joint ROM System"), NESMirroring::Unknown},
	{3, 0, 153, U8("8 KiB of WRAM instead of serial EEPROM"), NESMirroring::Unknown},
	{4, 0,   0, U8("FCG-1/2"), NESMirroring::Unknown},
	{5, 0,   0, U8("LZ93D50 with optional 24C02"), NESMirroring::Unknown},
};

// Mapper 019: Namco 129, 163
const struct NESSubmapperInfo namco_129_164_submappers[] = {
	{0, 0,   0, U8("Expansion sound volume unspecified"), NESMirroring::Unknown},
	{1, 0,  19, U8("Internal RAM battery-backed; no expansion sound"), NESMirroring::Unknown},
	{2, 0,   0, U8("No expansion sound"), NESMirroring::Unknown},
	{3, 0,   0, U8("N163 expansion sound: 11.0-13.0 dB louder than NES APU"), NESMirroring::Unknown},
	{4, 0,   0, U8("N163 expansion sound: 16.0-17.0 dB louder than NES APU"), NESMirroring::Unknown},
	{5, 0,   0, U8("N163 expansion sound: 18.0-19.5 dB louder than NES APU"), NESMirroring::Unknown},
};

// Mapper 021: Konami VRC4c, VRC4c
const struct NESSubmapperInfo vrc4a_vrc4c_submappers[] = {
	{1, 0,   0, U8("VRC4a"), NESMirroring::Unknown},
	{2, 0,   0, U8("VRC4c"), NESMirroring::Unknown},
};

// Mapper 023: Konami VRC4e, VRC4f, VRC2b
const struct NESSubmapperInfo vrc4ef_vrc2b_submappers[] = {
	{1, 0,   0, U8("VRC4f"), NESMirroring::Unknown},
	{2, 0,   0, U8("VRC4e"), NESMirroring::Unknown},
	{3, 0,   0, U8("VRC2b"), NESMirroring::Unknown},
};

// Mapper 025: Konami VRC4b, VRC4d, VRC2c
const struct NESSubmapperInfo vrc4bd_vrc2c_submappers[] = {
	{1, 0,   0, U8("VRC4b"), NESMirroring::Unknown},
	{2, 0,   0, U8("VRC4d"), NESMirroring::Unknown},
	{3, 0,   0, U8("VRC2c"), NESMirroring::Unknown},
};

// Mapper 032: Irem G101
const struct NESSubmapperInfo irem_g101_submappers[] = {
	{0, 0,   0, U8("Programmable mirroring"), NESMirroring::Unknown},
	{1, 0,   0, U8("Fixed one-screen mirroring"), NESMirroring::OneScreen_B},
};

// Mapper 034: BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
const struct NESSubmapperInfo bnrom_nina001_submappers[] = {
	{1, 0,   0, U8("NINA-001"), NESMirroring::Unknown},
	{2, 0,   0, U8("BNROM"), NESMirroring::Unknown},
};

// Mapper 068: Sunsoft-4
const struct NESSubmapperInfo sunsoft4_submappers[] = {
	{1, 0,   0, U8("Dual Cartridge System (NTB-ROM)"), NESMirroring::Unknown},
};

// Mapper 071: Codemasters
const struct NESSubmapperInfo codemasters_submappers[] = {
	{1, 0,   0, U8("Programmable one-screen mirroring (Fire Hawk)"), NESMirroring::MapperAB},
};

// Mapper 078: Cosmo Carrier / Holy Diver
const struct NESSubmapperInfo mapper078_submappers[] = {
	{1, 0,      0, U8("Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)"), NESMirroring::MapperAB},
	{2, 0, 0xFFFF, U8("Fixed vertical mirroring + WRAM"), NESMirroring::Unknown},
	{3, 0,      0, U8("Programmable H/V mirroring (Holy Diver)"), NESMirroring::MapperHV},
};

// Mapper 083: Cony/Yoko
const struct NESSubmapperInfo cony_yoko_submappers[] = {
	{0, 0,   0, U8("1 KiB CHR-ROM banking, no WRAM"), NESMirroring::Unknown},
	{1, 0,   0, U8("2 KiB CHR-ROM banking, no WRAM"), NESMirroring::Unknown},
	{2, 0,   0, U8("1 KiB CHR-ROM banking, 32 KiB banked WRAM"), NESMirroring::Unknown},
};

// Mapper 108: FDS conversions
const struct NESSubmapperInfo mapper108_submappers[] = {
	{1, 0,   0, U8("DH-08: Bubble Bobble (LH31)"), NESMirroring::Unknown},
	{2, 0,   0, U8("Bubble Bobble (LH31) (CHR-RAM)"), NESMirroring::Unknown},
	{3, 0,   0, U8("Falsion (LH54); Meikyuu Jiin Dababa (LH28)"), NESMirroring::Unknown},
	{4, 0,   0, U8("Pro Wrestling (LE05)"), NESMirroring::Unknown},
};

// Mapper 114: Sugar Softec/Hosenkan
const struct NESSubmapperInfo mapper114_submappers[] = {
	{0, 0,   0, U8("MMC3 registers: 0,3,1,5,6,7,2,4"), NESMirroring::Unknown},
	{1, 0,   0, U8("MMC3 registers: 0,2,5,3,6,1,7,4"), NESMirroring::Unknown},
};

// Mapper 197: Kǎshèng (MMC3 clone)
const struct NESSubmapperInfo mapper197_submappers[] = {
	{0, 0,   0, U8("Super Fighter III (PRG-ROM CRC32 0xC333F621)"), NESMirroring::Unknown},
	{1, 0,   0, U8("Super Fighter III (PRG-ROM CRC32 0x2091BEB2)"), NESMirroring::Unknown},
	{2, 0,   0, U8("Mortal Kombat III Special"), NESMirroring::Unknown},
	{3, 0,   0, U8("1995 Super 2-in-1"), NESMirroring::Unknown},
};

// Mapper 210: Namcot 175, 340
const struct NESSubmapperInfo namcot_175_340_submappers[] = {
	{1, 0,   0, U8("Namcot 175 (fixed mirroring)"),        NESMirroring::Header},
	{2, 0,   0, U8("Namcot 340 (programmable mirroring)"), NESMirroring::MapperHVAB},
};

// Mapper 215: Sugar Softec
const struct NESSubmapperInfo sugar_softec_submappers[] = {
	{0, 0,   0, U8("UNL-8237"), NESMirroring::Unknown},
	{1, 0,   0, U8("UNL-8237A"), NESMirroring::Unknown},
};

// Mapper 232: Codemasters Quattro
const struct NESSubmapperInfo quattro_submappers[] = {
	{1, 0,   0, U8("Aladdin Deck Enhancer"), NESMirroring::Unknown},
};

// Mapper 256: OneBus Famiclones
const struct NESSubmapperInfo onebus_submappers[] = {
	{ 1, 0,   0, U8("Waixing VT03"), NESMirroring::Unknown},
	{ 2, 0,   0, U8("Power Joy Supermax"), NESMirroring::Unknown},
	{ 3, 0,   0, U8("Zechess/Hummer Team"), NESMirroring::Unknown},
	{ 4, 0,   0, U8("Sports Game 69-in-1"), NESMirroring::Unknown},
	{ 5, 0,   0, U8("Waixing VT02"), NESMirroring::Unknown},
	{14, 0,   0, U8("Karaoto"), NESMirroring::Unknown},
	{15, 0,   0, U8("Jungletac"), NESMirroring::Unknown},
};

// Mapper 268: SMD132/SMD133
const struct NESSubmapperInfo smd132_smd133_submappers[] = {
	{0, 0,   0, U8("COOLBOY ($6000-$7FFF)"), NESMirroring::Unknown},
	{1, 0,   0, U8("MINDKIDS ($5000-$5FFF)"), NESMirroring::Unknown},
};

// Mapper 313: Reset-based multicart (MMC3)
const struct NESSubmapperInfo mapper313_submappers[] = {
	{0, 0,   0, U8("Game size: 128 KiB PRG, 128 KiB CHR"), NESMirroring::Unknown},
	{1, 0,   0, U8("Game size: 256 KiB PRG, 128 KiB CHR"), NESMirroring::Unknown},
	{2, 0,   0, U8("Game size: 128 KiB PRG, 256 KiB CHR"), NESMirroring::Unknown},
	{3, 0,   0, U8("Game size: 256 KiB PRG, 256 KiB CHR"), NESMirroring::Unknown},
	{4, 0,   0, U8("Game size: 256 KiB PRG (first game); 128 KiB PRG (other games); 128 KiB CHR"), NESMirroring::Unknown},
};

// Mapper 407: Win, Lose, or Draw Plug-n-Play (VT03)
const struct NESSubmapperInfo mapper407_submappers[] = {
	{15, 0,   0, U8("Opcode encryption (see mapper 256, submapper 15)"), NESMirroring::Unknown},
};

// Mapper 444: NC7000M multicart (MMC3-compatible)
const struct NESSubmapperInfo mapper444_submappers[] = {
	{0, 0,   0, U8("CHR A17 to $6000.0; CHR A18 to $6000.1"), NESMirroring::Unknown},
	{1, 0,   0, U8("CHR A17 to MMC3 CHR A17; CHR A18 to $6000.1"), NESMirroring::Unknown},
	{0, 0,   0, U8("CHR A17 to $6000.0; CHR A18 to $6000.4"), NESMirroring::Unknown},
	{1, 0,   0, U8("CHR A17 to MMC3 CHR A17; CHR A18 to $6000.4"), NESMirroring::Unknown},
};

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

#define NES2_SUBMAPPER(num, arr) {num, (uint16_t)ARRAY_SIZE(arr), arr}
const NESSubmapperEntry submappers[] = {
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
	NES2_SUBMAPPER(561, mapper006_submappers),		// Bung Super Game Doctor
	NES2_SUBMAPPER(562, mapper006_submappers),		// Venus Turbo Game Doctor

	{0, 0, nullptr}
};

/**
 * bsearch() comparison function for NESSubmapperInfo.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API NESSubmapperInfo_compar(const void *a, const void *b)
{
	uint8_t submapper1 = static_cast<const NESSubmapperInfo*>(a)->submapper;
	uint8_t submapper2 = static_cast<const NESSubmapperInfo*>(b)->submapper;
	if (submapper1 < submapper2) return -1;
	if (submapper1 > submapper2) return 1;
	return 0;
}

/**
 * bsearch() comparison function for NESSubmapperEntry.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API NESSubmapperEntry_compar(const void *a, const void *b)
{
	uint16_t mapper1 = static_cast<const NESSubmapperEntry*>(a)->mapper;
	uint16_t mapper2 = static_cast<const NESSubmapperEntry*>(b)->mapper;
	if (mapper1 < mapper2) return -1;
	if (mapper1 > mapper2) return 1;
	return 0;
}

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number
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
 * @param mapper Mapper number
 * @param submapper Submapper number
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
	const NESSubmapperEntry key = { static_cast<uint16_t>(mapper), 0, nullptr };
	const NESSubmapperEntry *const res =
		static_cast<const NESSubmapperEntry*>(bsearch(&key,
			submappers,
			ARRAY_SIZE(submappers)-1,
			sizeof(NESSubmapperEntry),
			NESSubmapperEntry_compar));
	if (!res || !res->info || res->info_size == 0)
		return nullptr;

	// Do a binary search in res->info.
	const NESSubmapperInfo key2 = {
		static_cast<uint8_t>(submapper), 0, 0,
		nullptr, NESMirroring::Unknown
	};
	const NESSubmapperInfo *const res2 =
		static_cast<const NESSubmapperInfo*>(bsearch(&key2,
			res->info, res->info_size,
			sizeof(NESSubmapperInfo),
			NESSubmapperInfo_compar));
	return res2;
}

/**
 * Get an entry from the string table.
 * @param idx String table index
 * @return String, or nullptr if 0 or out of bounds.
 */
static inline const char8_t *get_from_strtbl(unsigned int idx)
{
	assert(idx < ARRAY_SIZE(NESMappers_strtbl));
	if (idx == 0 || idx >= ARRAY_SIZE(NESMappers_strtbl))
		return nullptr;

	return &NESMappers_strtbl[idx];
}

/** Public functions **/

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number
 * @return Mapper name, or nullptr if not found.
 */
const char8_t *lookup_ines(int mapper)
{
	const NESMapperEntry *const ent = lookup_ines_info(mapper);
	return (ent ? get_from_strtbl(ent->name_idx) : nullptr);
}

/**
 * Convert a TNES mapper number to iNES.
 * @param tnes_mapper TNES mapper number
 * @return iNES mapper number, or -1 if unknown.
 */
int tnesMapperToInesMapper(int tnes_mapper)
{
	// 255 == not supported
	static const uint8_t ines_mappers[] = {
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

	if (tnes_mapper < 0 || tnes_mapper >= ARRAY_SIZE_I(ines_mappers)) {
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
const char8_t *lookup_nes2_submapper(int mapper, int submapper)
{
	const NESSubmapperInfo *const ent = lookup_nes2_submapper_info(mapper, submapper);
	// TODO: Return the "deprecated" value?
	return ent ? ent->desc : nullptr;
}

/**
 * Look up a description of mapper mirroring behavior
 * @param mapper Mapper number
 * @param submapper Submapper number
 * @param vert Vertical bit in the iNES header
 * @param four Four-screen bit in the iNES header
 * @return String describing the mirroring behavior
 */
const char8_t *lookup_ines_mirroring(int mapper, int submapper, bool vert, bool four)
{
	const NESMapperEntry *const ent1 = lookup_ines_info(mapper);
	const NESSubmapperInfo *const ent2 = lookup_nes2_submapper_info(mapper, submapper);
	NESMirroring mirror = ent1 ? ent1->mirroring : NESMirroring::Unknown;
	NESMirroring submapper_mirror = ent2 ? ent2->mirroring :NESMirroring::Unknown;

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

	// NOTE: to prevent useless noise like "Mapper: MMC5, Mirroring: MMC5-like"), all of the weird
	// mirroring types are grouped under "Mapper-controlled"
	const char8_t *s_ret;
	switch (mirror) {
		case NESMirroring::Header:
			s_ret = (vert) ? C_("NES|Mirroring", "Vertical") : C_("NES|Mirroring", "Horizontal");
			break;
		case NESMirroring::Mapper:
		default:
			s_ret = C_("NES|Mirroring", "Mapper-controlled");
			break;
		case NESMirroring::MapperHVAB:
			s_ret = C_("NES|Mirroring", "Mapper-controlled (H/V/A/B)");
			break;
		case NESMirroring::MapperHV:
			s_ret = C_("NES|Mirroring", "Mapper-controlled (H/V)");
			break;
		case NESMirroring::MapperAB:
			s_ret = C_("NES|Mirroring", "Mapper-controlled (A/B)");
			break;
		case NESMirroring::OneScreen_A:
			s_ret = C_("NES|Mirroring", "Single Screen (A)");
			break;
		case NESMirroring::OneScreen_B:
			s_ret = C_("NES|Mirroring", "Single Screen (B)");
			break;
		case NESMirroring::FourScreen:
			s_ret = C_("NES|Mirroring", "Four Screens");
			break;
	}
	return s_ret;
}

} }

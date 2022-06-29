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

// iNES mapper list
struct MapperEntry {
	const char *name;		// Name of the board. (If unknown, nullptr.)
	const char *manufacturer;	// Manufacturer. (If unknown, nullptr.)
	NESMirroring mirroring;		// Mirroring behavior.
};

/**
 * Mappers: NES 2.0 Plane 0 [000-255] (iNES 1.0)
 */
const MapperEntry mappers_plane0[] = {
	/** NES 2.0 Plane 0 [0-255] (iNES 1.0) **/

	// Mappers 000-009
	{"NROM",			"Nintendo",		NESMirroring::Header},
	{"SxROM (MMC1)",		"Nintendo",		NESMirroring::MapperHVAB},
	{"UxROM",			"Nintendo",		NESMirroring::Header},
	{"CNROM",			"Nintendo",		NESMirroring::Header},
	{"TxROM (MMC3), HKROM (MMC6)",	"Nintendo",		NESMirroring::MapperHV},
	{"ExROM (MMC5)",		"Nintendo",		NESMirroring::MapperMMC5},
	{"Game Doctor Mode 1",		"Bung/FFE",		NESMirroring::MapperHVAB},
	{"AxROM",			"Nintendo",		NESMirroring::MapperAB},
	{"Game Doctor Mode 4 (GxROM)",	"Bung/FFE",		NESMirroring::MapperHVAB},
	{"PxROM, PEEOROM (MMC2)",	"Nintendo",		NESMirroring::MapperHV},

	// Mappers 010-019
	{"FxROM (MMC4)",		"Nintendo",		NESMirroring::MapperHV},
	{"Color Dreams",		"Color Dreams",		NESMirroring::Header},
	{"MMC3 variant",		"FFE",			NESMirroring::MapperHV},
	{"NES-CPROM",			"Nintendo",		NESMirroring::Header},
	{"SL-1632 (MMC3/VRC2 clone)",	"Nintendo",		NESMirroring::MapperHVAB},
	{"K-1029 (multicart)",		nullptr,		NESMirroring::MapperHV},
	{"FCG-x",			"Bandai",		NESMirroring::MapperHVAB},
	{"FFE #17",			"FFE",			NESMirroring::MapperHVAB},
	{"SS 88006",			"Jaleco",		NESMirroring::MapperHVAB},
	{"Namco 129/163",		"Namco",		NESMirroring::MapperNamco163},	// TODO: Namcot-106?

	// Mappers 020-029
	{"Famicom Disk System",		"Nintendo",		NESMirroring::MapperHV}, // this isn't actually used, as FDS roms are stored in their own format.
	{"VRC4a, VRC4c",		"Konami",		NESMirroring::MapperHVAB},
	{"VRC2a",			"Konami",		NESMirroring::MapperHVAB},
	{"VRC4e, VRC4f, VRC2b",		"Konami",		NESMirroring::MapperHVAB},
	{"VRC6a",			"Konami",		NESMirroring::MapperVRC6},
	{"VRC4b, VRC4d, VRC2c",		"Konami",		NESMirroring::MapperHVAB},
	{"VRC6b",			"Konami",		NESMirroring::MapperVRC6},
	{"VRC4 variant",		nullptr,		NESMirroring::MapperHVAB}, //investigate
	{"Action 53",			"Homebrew",		NESMirroring::MapperHVAB},
	{"RET-CUFROM",			"Sealie Computing",	NESMirroring::Header},	// Homebrew

	// Mappers 030-039
	{"UNROM 512",			"RetroUSB",		NESMirroring::UNROM512},	// Homebrew
	{"NSF Music Compilation",	"Homebrew",		NESMirroring::Header},
	{"Irem G-101",			"Irem",			NESMirroring::MapperHV /* see submapper */},
	{"Taito TC0190",		"Taito",		NESMirroring::MapperHV},
	{"BNROM, NINA-001",		nullptr,		NESMirroring::Header},
	{"J.Y. Company ASIC (8 KiB WRAM)", "J.Y. Company",	NESMirroring::MapperJY},
	{"TXC PCB 01-22000-400",	"TXC",			NESMirroring::MapperHV},
	{"MMC3 multicart",		"Nintendo",		NESMirroring::MapperHV},
	{"GNROM variant",		"Bit Corp.",		NESMirroring::Header},
	{"BNROM variant",		nullptr,		NESMirroring::Header},

	// Mappers 040-049
	{"NTDEC 2722 (FDS conversion)",	"NTDEC",		NESMirroring::Header},
	{"Caltron 6-in-1",		"Caltron",		NESMirroring::MapperHV},
	{"FDS conversion",		nullptr,		NESMirroring::MapperHV},
	{"TONY-I, YS-612 (FDS conversion)", nullptr,		NESMirroring::Header},
	{"MMC3 multicart",		nullptr,		NESMirroring::MapperHV},
	{"MMC3 multicart (GA23C)",	nullptr,		NESMirroring::MapperHV},
	{"Rumble Station 15-in-1",	"Color Dreams",		NESMirroring::Header},	// NES-on-a-Chip
	{"MMC3 multicart",		"Nintendo",		NESMirroring::MapperHV},
	{"Taito TC0690",		"Taito",		NESMirroring::MapperHV},	// TODO: Taito-TC190V?
	{"MMC3 multicart",		nullptr,		NESMirroring::MapperHV},

	// Mappers 050-059
	{"PCB 761214 (FDS conversion)",	"N-32",			NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"MMC3 multicart",		nullptr,		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Novel Diamond 9999999-in-1",	nullptr,		NESMirroring::Unknown},	// conflicting information
	{"BTL-MARIO1-MALEE2",		nullptr,		NESMirroring::Header},	// From UNIF
	{"KS202 (unlicensed SMB3 reproduction)", nullptr,	NESMirroring::MapperHV},	// Some SMB3 unlicensed reproduction
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"(C)NROM-based multicart",	nullptr,		NESMirroring::MapperHV},
	{"BMC-T3H53/BMC-D1038 multicart", nullptr,		NESMirroring::MapperHV},	// From UNIF

	// Mappers 060-069
	{"Reset-based NROM-128 4-in-1 multicart", nullptr,	NESMirroring::Header},
	{"20-in-1 multicart",		nullptr,		NESMirroring::MapperHV},
	{"Super 700-in-1 multicart",	nullptr,		NESMirroring::MapperHV},
	{"Powerful 250-in-1 multicart",	"NTDEC",		NESMirroring::MapperHV},
	{"Tengen RAMBO-1",		"Tengen",		NESMirroring::MapperHV},
	{"Irem H3001",			"Irem",			NESMirroring::MapperHV},
	{"GxROM, MHROM",		"Nintendo",		NESMirroring::Header},
	{"Sunsoft-3",			"Sunsoft",		NESMirroring::MapperHVAB},
	{"Sunsoft-4",			"Sunsoft",		NESMirroring::MapperSunsoft4},
	{"Sunsoft FME-7",		"Sunsoft",		NESMirroring::MapperHVAB},

	// Mappers 070-079
	{"Family Trainer",		"Bandai",		NESMirroring::Header /* see wiki for a caveat */},
	{"Codemasters (UNROM clone)",	"Codemasters",		NESMirroring::Header /* see submapper */},
	{"Jaleco JF-17",		"Jaleco",		NESMirroring::Header},	// TODO: Jaleco-2?
	{"VRC3",			"Konami",		NESMirroring::Header},
	{"43-393/860908C (MMC3 clone)",	"Waixing",		NESMirroring::MapperHV},
	{"VRC1",			"Konami",		NESMirroring::MapperHV},
	{"NAMCOT-3446 (Namcot 108 variant)",	"Namco",	NESMirroring::Header},	// TODO: Namco-109?
	{"Napoleon Senki",		"Lenar",		NESMirroring::FourScreen},	// TODO: Irem-1? 
	{"Holy Diver; Uchuusen - Cosmo Carrier", nullptr,	NESMirroring::Mapper /* see submapper */},	// TODO: Irem-74HC161?
	{"NINA-03, NINA-06",		"American Video Entertainment", NESMirroring::Header},

	// Mappers 080-089
	{"Taito X1-005",		"Taito",		NESMirroring::MapperHV},
	{"Super Gun",			"NTDEC",		NESMirroring::Header},
	{"Taito X1-017 (incorrect PRG ROM bank ordering)", "Taito", NESMirroring::MapperHV},
	{"Cony/Yoko",			"Cony/Yoko",		NESMirroring::MapperHVAB},
	{"PC-SMB2J",			nullptr,		NESMirroring::Unknown},
	{"VRC7",			"Konami",		NESMirroring::MapperHVAB},
	{"Jaleco JF-13",		"Jaleco",		NESMirroring::Header},	// TODO: Jaleco-4?
	{"CNROM variant",		nullptr,		NESMirroring::Header},	// TODO: Jaleco-1?
	{"Namcot 118 variant",		nullptr,		NESMirroring::Header},	// TODO: Namco-118?
	{"Sunsoft-2 (Sunsoft-3 board)",	"Sunsoft",		NESMirroring::MapperAB},

	// Mappers 090-099
	{"J.Y. Company (simple nametable control)", "J.Y. Company", NESMirroring::MapperHVAB},
	{"J.Y. Company (Super Fighter III)", "J.Y. Company",	NESMirroring::MapperHV /* see submapper */},
	{"Moero!! Pro",			"Jaleco",		NESMirroring::Header},	// TODO: Jaleco-3?
	{"Sunsoft-2 (Sunsoft-3R board)", "Sunsoft",		NESMirroring::Header},	// TODO: 74161A?
	{"HVC-UN1ROM",			"Nintendo",		NESMirroring::Header},	// TODO: 74161B?
	{"NAMCOT-3425",			"Namco",		NESMirroring::MapperNamcot3425},	// TODO: Namcot?
	{"Oeka Kids",			"Bandai",		NESMirroring::Header},
	{"Irem TAM-S1",			"Irem",			NESMirroring::MapperHV},	// TODO: Irem-2?
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"CNROM (Vs. System)",		"Nintendo",		NESMirroring::FourScreen},

	// Mappers 100-109
	{"MMC3 variant (hacked ROMs)",	nullptr,		NESMirroring::MapperHV},	// Also used for UNIF
	{"Jaleco JF-10 (misdump)",	"Jaleceo",		NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Doki Doki Panic (FDS conversion)", nullptr,		NESMirroring::MapperHV},
	{"PEGASUS 5 IN 1",		nullptr,		NESMirroring::Header},
	{"NES-EVENT (MMC1 variant) (Nintendo World Championships 1990)", "Nintendo", NESMirroring::MapperHVAB},
	{"Super Mario Bros. 3 (bootleg)", nullptr,		NESMirroring::MapperHV},
	{"Magic Dragon",		"Magicseries",		NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 110-119
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Cheapocabra GTROM 512k flash board", "Membler Industries", NESMirroring::MapperGTROM},	// Homebrew
	{"Namcot 118 variant",		nullptr,		NESMirroring::MapperHV},
	{"NINA-03/06 multicart",	nullptr,		NESMirroring::MapperHV},
	{"MMC3 clone (scrambled registers)", nullptr,		NESMirroring::MapperHV},
	{"Kǎshèng SFC-02B/-03/-004 (MMC3 clone)", "Kǎshèng",	NESMirroring::MapperHV},
	{"SOMARI-P (Huang-1/Huang-2)",	"Gouder",		NESMirroring::MapperHVAB},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"TxSROM",			"Nintendo",		NESMirroring::MapperTxSROM},	// TODO: MMC-3+TLS?
	{"TQROM",			"Nintendo",		NESMirroring::MapperHV},

	// Mappers 120-129
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Kǎshèng A9711 and A9713 (MMC3 clone)", "Kǎshèng",	NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Kǎshèng H2288 (MMC3 clone)",	"Kǎshèng",		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Monty no Doki Doki Daisassō (FDS conversion)", "Whirlwind Manu", NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 130-139
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"TXC 05-00002-010 ASIC",	"TXC",			NESMirroring::Header},
	{"Jovial Race",			"Sachen",		NESMirroring::Header},
	{"T4A54A, WX-KB4K, BS-5652 (MMC3 clone)", nullptr,	NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Sachen 3011",			"Sachen",		NESMirroring::Header},
	{"Sachen 8259D",		"Sachen",		NESMirroring::MapperSachen8259},
	{"Sachen 8259B",		"Sachen",		NESMirroring::MapperSachen8259},
	{"Sachen 8259C",		"Sachen",		NESMirroring::MapperSachen8259},

	// Mappers 140-149
	{"Jaleco JF-11, JF-14 (GNROM variant)", "Jaleco",	NESMirroring::Header},
	{"Sachen 8259A",		"Sachen",		NESMirroring::MapperSachen8259},
	{"Kaiser KS202 (FDS conversions)", "Kaiser",		NESMirroring::Header},
	{"Copy-protected NROM",		nullptr,		NESMirroring::Header},
	{"Death Race (Color Dreams variant)", "American Game Cartridges", NESMirroring::Header},
	{"Sidewinder (CNROM clone)",	"Sachen",		NESMirroring::Header},
	{"Galactic Crusader (NINA-06 clone)", nullptr,		NESMirroring::Header},
	{"Sachen 3018",			"Sachen",		NESMirroring::Header},
	{"Sachen SA-008-A, Tengen 800008",	"Sachen / Tengen", NESMirroring::Header},
	{"SA-0036 (CNROM clone)",	"Sachen",		NESMirroring::Header},

	// Mappers 150-159
	{"Sachen SA-015, SA-630",	"Sachen",		NESMirroring::MapperSachen74LS374N},
	{"VRC1 (Vs. System)",		"Konami",		NESMirroring::FourScreen},
	{"Kaiser KS202 (FDS conversion)", "Kaiser",		NESMirroring::MapperAB},
	{"Bandai FCG: LZ93D50 with SRAM", "Bandai",		NESMirroring::MapperHVAB},
	{"NAMCOT-3453",			"Namco",		NESMirroring::MapperAB},
	{"MMC1A",			"Nintendo",		NESMirroring::MapperHVAB},
	{"DIS23C01",			"Daou Infosys",		NESMirroring::MapperDIS23C01},
	{"Datach Joint ROM System",	"Bandai",		NESMirroring::MapperHVAB},
	{"Tengen 800037",		"Tengen",		NESMirroring::MapperTxSROM},
	{"Bandai LZ93D50 with 24C01",	"Bandai",		NESMirroring::MapperHVAB},

	// Mappers 160-169
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Nanjing",			"Nanjing",		NESMirroring::Header},
	{"Waixing (unlicensed)",	"Waixing",		NESMirroring::Header},
	{"Fire Emblem (unlicensed) (MMC2+MMC3 hybrid)", nullptr, NESMirroring::MapperHV},
	{"Subor (variant 1)",		"Subor",		NESMirroring::Header},
	{"Subor (variant 2)",		"Subor",		NESMirroring::Header},
	{"Racermate Challenge 2",	"Racermate, Inc.",	NESMirroring::Header},
	{"Yuxing",			"Yuxing",		NESMirroring::Unknown},

	// Mappers 170-179
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Kaiser KS-7058",		"Kaiser",		NESMirroring::Header},
	{"Super Mega P-4040",		nullptr,		NESMirroring::MapperHV},
	{"Idea-Tek ET-xx",		"Idea-Tek",		NESMirroring::Header},
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Waixing multicart (MMC3 clone)", "Waixing",		NESMirroring::MapperHVAB},
	{"BNROM variant",		"Hénggé Diànzǐ",	NESMirroring::MapperHV},
	{"Waixing / Nanjing / Jncota / Henge Dianzi / GameStar", "Waixing / Nanjing / Jncota / Henge Dianzi / GameStar", NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 180-189
	{"Crazy Climber (UNROM clone)",	"Nichibutsu",		NESMirroring::Header},
	{"Seicross v2 (FCEUX hack)",	"Nichibutsu",		NESMirroring::Header},
	{"MMC3 clone (scrambled registers) (same as 114)", nullptr, NESMirroring::MapperHV},
	{"Suikan Pipe (VRC4e clone)",	nullptr,		NESMirroring::MapperHVAB},
	{"Sunsoft-1",			"Sunsoft",		NESMirroring::Header},
	{"CNROM with weak copy protection", nullptr,		NESMirroring::Header},	// Submapper field indicates required value for CHR banking. (TODO: VROM-disable?)
	{"Study Box",			"Fukutake Shoten",	NESMirroring::Header},
	{"Kǎshèng A98402 (MMC3 clone)",	"Kǎshèng",		NESMirroring::MapperHV},
	{"Bandai Karaoke Studio",	"Bandai",		NESMirroring::MapperHV},
	{"Thunder Warrior (MMC3 clone)", nullptr,		NESMirroring::MapperHV},

	// Mappers 190-199
	{"Magic Kid GooGoo",		nullptr,		NESMirroring::Header},
	{"MMC3 clone",			nullptr,		NESMirroring::MapperHV},
	{"MMC3 clone",			nullptr,		NESMirroring::MapperHV},
	{"NTDEC TC-112",		"NTDEC",		NESMirroring::MapperHV},
	{"MMC3 clone",			nullptr,		NESMirroring::MapperHV},
	{"Waixing FS303 (MMC3 clone)",	"Waixing",		NESMirroring::MapperHV},
	{"Mario bootleg (MMC3 clone)",	nullptr,		NESMirroring::MapperHV},
	{"Kǎshèng (MMC3 clone)",	"Kǎshèng",		NESMirroring::MapperHV /* not sure */},
	{"Tūnshí Tiāndì - Sānguó Wàizhuàn", nullptr,		NESMirroring::MapperHV},
	{"Waixing (clone of either Mapper 004 or 176)", "Waixing", NESMirroring::MapperHVAB},

	// Mappers 200-209
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"NROM-256 multicart",		nullptr,		NESMirroring::Header},
	{"150-in-1 multicart",		nullptr,		NESMirroring::MapperHV},
	{"35-in-1 multicart",		nullptr,		NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"MMC3 multicart",		nullptr,		NESMirroring::MapperHV},
	{"DxROM (Tengen MIMIC-1, Namcot 118)", "Nintendo",	NESMirroring::Header},
	{"Fudou Myouou Den",		"Taito",		NESMirroring::MapperNamcot3425},
	{"Street Fighter IV (unlicensed) (MMC3 clone)", nullptr, NESMirroring::MapperHV},
	{"J.Y. Company (MMC2/MMC4 clone)", "J.Y. Company",	NESMirroring::MapperJY},

	// Mappers 210-219
	{"Namcot 175, 340",		"Namco",		NESMirroring::MapperHVAB /* see submapper */},
	{"J.Y. Company (extended nametable control)", "J.Y. Company", NESMirroring::MapperJY},
	{"BMC Super HiK 300-in-1",	nullptr,		NESMirroring::MapperHV},
	{"(C)NROM-based multicart (same as 058)", nullptr,	NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Sugar Softec (MMC3 clone)",	"Sugar Softec",		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Magic Floor",			"Homebrew",		NESMirroring::MagicFloor},
	{"Kǎshèng A9461 (MMC3 clone)",	"Kǎshèng",		NESMirroring::MapperHV},

	// Mappers 220-229
	{"Summer Carnival '92 - Recca",	"Naxat Soft",		NESMirroring::Unknown /* TODO */},
	{"NTDEC N625092",		"NTDEC",		NESMirroring::MapperHV},
	{"CTC-31 (VRC2 + 74xx)",	nullptr,		NESMirroring::MapperHVAB},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Jncota KT-008",		"Jncota",		NESMirroring::Unknown /* TODO */},
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"Active Enterprises",		"Active Enterprises",	NESMirroring::MapperHV},
	{"BMC 31-IN-1",			nullptr,		NESMirroring::MapperHV},

	// Mappers 230-239
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"Multicart",			nullptr,		NESMirroring::MapperHV},
	{"Codemasters Quattro",		"Codemasters",		NESMirroring::Header},
	{"Multicart",			nullptr,		NESMirroring::Mapper233},
	{"Maxi 15 multicart",		nullptr,		NESMirroring::MapperHV},
	{"Golden Game 150-in-1 multicart", nullptr,		NESMirroring::Mapper235},
	{"Realtec 8155",		"Realtec",		NESMirroring::MapperHV},
	{"Teletubbies 420-in-1 multicart", nullptr,		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 240-249
	{"Multicart",			nullptr,		NESMirroring::Header},
	{"BNROM variant (similar to 034)", nullptr,		NESMirroring::Header},
	{"Unlicensed",			nullptr,		NESMirroring::MapperHV},
	{"Sachen SA-020A",		"Sachen",		NESMirroring::MapperSachen74LS374N},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"MMC3 clone",			nullptr,		NESMirroring::MapperHV},
	{"Fēngshénbǎng: Fúmó Sān Tàizǐ (C&E)", "C&E",		NESMirroring::Header},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Kǎshèng SFC-02B/-03/-004 (MMC3 clone) (incorrect assignment; should be 115)", "Kǎshèng", NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 250-255
	{"Nitra (MMC3 clone)",		"Nitra",		NESMirroring::MapperHV},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Waixing - Sangokushi",	"Waixing",		NESMirroring::MapperHVAB},
	{"Dragon Ball Z: Kyōshū! Saiya-jin (VRC4 clone)", "Waixing", NESMirroring::MapperHVAB},
	{"Pikachu Y2K of crypted ROMs",	nullptr,		NESMirroring::MapperHV},
	{"110-in-1 multicart (same as 225)", nullptr,		NESMirroring::MapperHV},
};

/**
 * Mappers: NES 2.0 Plane 1 [256-511]
 * TODO: Add mirroring info
 */
const MapperEntry mappers_plane1[] = {
	// Mappers 256-259
	{"OneBus Famiclone",		nullptr,		NESMirroring::Unknown},
	{"UNIF PEC-586",		nullptr,		NESMirroring::Unknown},	// From UNIF; reserved by FCEUX developers
	{"UNIF 158B",			nullptr,		NESMirroring::Unknown},	// From UNIF; reserved by FCEUX developers
	{"UNIF F-15 (MMC3 multicart)",	nullptr,		NESMirroring::Unknown},	// From UNIF; reserved by FCEUX developers

	// Mappers 260-269
	{"HP10xx/HP20xx multicart",	nullptr,		NESMirroring::Unknown},
	{"200-in-1 Elfland multicart",	nullptr,		NESMirroring::Unknown},
	{"Street Heroes (MMC3 clone)",	"Sachen",		NESMirroring::Unknown},
	{"King of Fighters '97 (MMC3 clone)", nullptr,		NESMirroring::Unknown},
	{"Cony/Yoko Fighting Games",	"Cony/Yoko",		NESMirroring::Unknown},
	{"T-262 multicart",		nullptr,		NESMirroring::Unknown},
	{"City Fighter IV",		nullptr,		NESMirroring::Unknown},	// Hack of Master Fighter II
	{"8-in-1 JY-119 multicart (MMC3 clone)", "J.Y. Company", NESMirroring::Unknown},
	{"SMD132/SMD133 (MMC3 clone)",	nullptr,		NESMirroring::Unknown},
	{"Multicart (MMC3 clone)",	nullptr,		NESMirroring::Unknown},

	// Mappers 270-279
	{"Game Prince RS-16",		nullptr,		NESMirroring::Unknown},
	{"TXC 4-in-1 multicart (MGC-026)", "TXC",		NESMirroring::Unknown},
	{"Akumajō Special: Boku Dracula-kun (bootleg)", nullptr, NESMirroring::Unknown},
	{"Gremlins 2 (bootleg)",	nullptr,		NESMirroring::Unknown},
	{"Cartridge Story multicart",	"RCM Group",		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 280-289
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"J.Y. Company Super HiK 3/4/5-in-1 multicart", "J.Y. Company", NESMirroring::Unknown},
	{"J.Y. Company multicart",	"J.Y. Company",		NESMirroring::Unknown},
	{"Block Family 6-in-1/7-in-1 multicart", nullptr,	NESMirroring::Unknown},
	{"Drip",			"Homebrew",		NESMirroring::Unknown},
	{"A65AS multicart",		nullptr,		NESMirroring::Unknown},
	{"Benshieng multicart",		"Benshieng",		NESMirroring::Unknown},
	{"4-in-1 multicart (411120-C, 811120-C)", nullptr,	NESMirroring::Unknown},
	{"GKCX1 21-in-1 multicart",	nullptr,		NESMirroring::Unknown},	// GoodNES 3.23b sets this to Mapper 133, which is wrong.
	{"BMC-60311C",			nullptr,		NESMirroring::Unknown},	// From UNIF

	// Mappers 290-299
	{"Asder 20-in-1 multicart",	"Asder",		NESMirroring::Unknown},
	{"Kǎshèng 2-in-1 multicart (MK6)", "Kǎshèng",		NESMirroring::Unknown},
	{"Dragon Fighter (unlicensed)",	nullptr,		NESMirroring::Unknown},
	{"NewStar 12-in-1/76-in-1 multicart", nullptr,		NESMirroring::Unknown},
	{"T4A54A, WX-KB4K, BS-5652 (MMC3 clone) (same as 134)", nullptr, NESMirroring::Unknown},
	{"J.Y. Company 13-in-1 multicart", "J.Y. Company",	NESMirroring::Unknown},
	{"FC Pocket RS-20 / dreamGEAR My Arcade Gamer V", nullptr, NESMirroring::Unknown},
	{"TXC 01-22110-000 multicart",	"TXC",			NESMirroring::Unknown},
	{"Lethal Weapon (unlicensed) (VRC4 clone)", nullptr,	NESMirroring::Unknown},
	{"TXC 6-in-1 multicart (MGC-023)", "TXC",		NESMirroring::Unknown},

	// Mappers 300-309
	{"Golden 190-in-1 multicart",	nullptr,		NESMirroring::Unknown},
	{"GG1 multicart",		nullptr,		NESMirroring::Unknown},
	{"Gyruss (FDS conversion)",	"Kaiser",		NESMirroring::Unknown},
	{"Almana no Kiseki (FDS conversion)", "Kaiser",		NESMirroring::Unknown},
	{"FDS conversion",		"Whirlwind Manu",	NESMirroring::Unknown},
	{"Dracula II: Noroi no Fūin (FDS conversion)", "Kaiser", NESMirroring::Unknown},
	{"Exciting Basket (FDS conversion)", "Kaiser",		NESMirroring::Unknown},
	{"Metroid (FDS conversion)",	"Kaiser",		NESMirroring::Unknown},
	{"Batman (Sunsoft) (bootleg) (VRC2 clone)", nullptr,	NESMirroring::Unknown},
	{"Ai Senshi Nicol (FDS conversion)", "Whirlwind Manu",	NESMirroring::Unknown},

	// Mappers 310-319
	{"Monty no Doki Doki Daisassō (FDS conversion) (same as 125)", "Whirlwind Manu", NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Highway Star (bootleg)",	"Kaiser",		NESMirroring::Unknown},
	{"Reset-based multicart (MMC3)", nullptr,		NESMirroring::Unknown},
	{"Y2K multicart",		nullptr,		NESMirroring::Unknown},
	{"820732C- or 830134C- multicart", nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"HP-898F, KD-7/9-E multicart",	nullptr,		NESMirroring::Unknown},

	// Mappers 320-329
	{"Super HiK 6-in-1 A-030 multicart", nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"35-in-1 (K-3033) multicart",	nullptr,		NESMirroring::Unknown},
	{"Farid's homebrew 8-in-1 SLROM multicart", nullptr,	NESMirroring::Unknown},	// Homebrew
	{"Farid's homebrew 8-in-1 UNROM multicart", nullptr,	NESMirroring::Unknown},	// Homebrew
	{"Super Mali Splash Bomb (bootleg)", nullptr,		NESMirroring::Unknown},
	{"Contra/Gryzor (bootleg)",	nullptr,		NESMirroring::Unknown},
	{"6-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"Test Ver. 1.01 Dlya Proverki TV Pristavok test cartridge", nullptr, NESMirroring::Unknown},
	{"Education Computer 2000",	nullptr,		NESMirroring::Unknown},

	// Mappers 330-339
	{"Sangokushi II: Haō no Tairiku (bootleg)", nullptr,	NESMirroring::Unknown},
	{"7-in-1 (NS03) multicart",	nullptr,		NESMirroring::Unknown},
	{"Super 40-in-1 multicart",	nullptr,		NESMirroring::Unknown},
	{"New Star Super 8-in-1 multicart", "New Star",		NESMirroring::Unknown},
	{"5/20-in-1 1993 Copyright multicart", nullptr,		NESMirroring::Unknown},
	{"10-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"11-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"12-in-1 Game Card multicart",	nullptr,		NESMirroring::Unknown},
	{"16-in-1, 200/300/600/1000-in-1 multicart", nullptr,	NESMirroring::Unknown},
	{"21-in-1 multicart",		nullptr,		NESMirroring::Unknown},

	// Mappers 340-349
	{"35-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"Simple 4-in-1 multicart",	nullptr,		NESMirroring::Unknown},
	{"COOLGIRL multicart (Homebrew)", "Homebrew",		NESMirroring::Unknown},	// Homebrew
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 multicart", nullptr, NESMirroring::Unknown},
	{"New Star 6-in-1 Game Cartridge multicart", "New Star", NESMirroring::Unknown},
	{"Zanac (FDS conversion)",	"Kaiser",		NESMirroring::Unknown},
	{"Yume Koujou: Doki Doki Panic (FDS conversion)", "Kaiser", NESMirroring::Unknown},
	{"830118C",			nullptr,		NESMirroring::Unknown},
	{"1994 Super HIK 14-in-1 (G-136) multicart", nullptr,	NESMirroring::Unknown},

	// Mappers 350-359
	{"Super 15-in-1 Game Card multicart", nullptr,		NESMirroring::Unknown},
	{"9-in-1 multicart",		"J.Y. Company / Techline", NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"92 Super Mario Family multicart", nullptr,		NESMirroring::Unknown},
	{"250-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"黃信維 3D-BLOCK",		nullptr,		NESMirroring::Unknown},
	{"7-in-1 Rockman (JY-208)",	"J.Y. Company",		NESMirroring::Unknown},
	{"4-in-1 (4602) multicart",	"Bit Corp.",		NESMirroring::Unknown},
	{"J.Y. Company multicart",	"J.Y. Company",		NESMirroring::Unknown},
	{"SB-5013 / GCL8050 / 841242C multicart", nullptr,	NESMirroring::Unknown},

	// Mappers 360-369
	{"31-in-1 (3150) multicart",	"Bit Corp.",		NESMirroring::Unknown},
	{"YY841101C multicart (MMC3 clone)", "J.Y. Company",	NESMirroring::Unknown},
	{"830506C multicart (VRC4f clone)", "J.Y. Company",	NESMirroring::Unknown},
	{"J.Y. Company multicart",	"J.Y. Company",		NESMirroring::Unknown},
	{"JY830832C multicart",		"J.Y. Company",		NESMirroring::Unknown},
	{"Asder PC-95 educational computer", "Asder",		NESMirroring::Unknown},
	{"GN-45 multicart (MMC3 clone)", nullptr,		NESMirroring::Unknown},
	{"7-in-1 multicart",		nullptr,		NESMirroring::Unknown},
	{"Super Mario Bros. 2 (J) (FDS conversion)", "YUNG-08",	NESMirroring::Unknown},
	{"N49C-300",			nullptr,		NESMirroring::Unknown},

	// Mappers 370-379
	{"F600",			nullptr,		NESMirroring::Unknown},
	{"Spanish PEC-586 home computer cartridge", "Dongda",	NESMirroring::Unknown},
	{"Rockman 1-6 (SFC-12) multicart", nullptr,		NESMirroring::Unknown},
	{"Super 4-in-1 (SFC-13) multicart", nullptr,		NESMirroring::Unknown},
	{"Reset-based MMC1 multicart",	nullptr,		NESMirroring::Unknown},
	{"135-in-1 (U)NROM multicart",	nullptr,		NESMirroring::Unknown},
	{"YY841155C multicart",		"J.Y. Company",		NESMirroring::Unknown},
	{"8-in-1 AxROM/UNROM multicart", nullptr,		NESMirroring::Unknown},
	{"35-in-1 NROM multicart",	nullptr,		NESMirroring::Unknown},

	// Mappers 380-389
	{"970630C",			nullptr,		NESMirroring::Unknown},
	{"KN-42",			nullptr,		NESMirroring::Unknown},
	{"830928C",			nullptr,		NESMirroring::Unknown},
	{"YY840708C (MMC3 clone)",	"J.Y. Company",		NESMirroring::Unknown},
	{"L1A16 (VRC4e clone)",		nullptr,		NESMirroring::Unknown},
	{"NTDEC 2779",			"NTDEC",		NESMirroring::Unknown},
	{"YY860729C",			"J.Y. Company",		NESMirroring::Unknown},
	{"YY850735C / YY850817C",	"J.Y. Company",		NESMirroring::Unknown},
	{"YY841145C / YY850835C",	"J.Y. Company",		NESMirroring::Unknown},
	{"Caltron 9-in-1 multicart",	"Caltron",		NESMirroring::Unknown},

	// Mappers 390-391
	{"Realtec 8031",		"Realtec",		NESMirroring::Unknown},
	{"NC7000M (MMC3 clone)",	nullptr,		NESMirroring::Unknown},
};

/**
 * Mappers: NES 2.0 Plane 2 [512-767]
 * TODO: Add mirroring info
 */
const MapperEntry mappers_plane2[] = {
	// Mappers 512-519
	{"Zhōngguó Dàhēng",		"Sachen",		NESMirroring::Unknown},
	{"Měi Shàonǚ Mèng Gōngchǎng III", "Sachen",		NESMirroring::Unknown},
	{"Subor Karaoke",		"Subor",		NESMirroring::Unknown},
	{"Family Noraebang",		nullptr,		NESMirroring::Unknown},
	{"Brilliant Com Cocoma Pack",	"EduBank",		NESMirroring::Unknown},
	{"Kkachi-wa Nolae Chingu",	nullptr,		NESMirroring::Unknown},
	{"Subor multicart",		"Subor",		NESMirroring::Unknown},
	{"UNL-EH8813A",			nullptr,		NESMirroring::Unknown},

	// Mappers 520-529
	{"2-in-1 Datach multicart (VRC4e clone)", nullptr,	NESMirroring::Unknown},
	{"Korean Igo",			nullptr,		NESMirroring::Unknown},
	{"Fūun Shōrinken (FDS conversion)", "Whirlwind Manu",	NESMirroring::Unknown},
	{"Fēngshénbǎng: Fúmó Sān Tàizǐ (Jncota)", "Jncota",	NESMirroring::Unknown},
	{"The Lord of King (Jaleco) (bootleg)", nullptr,	NESMirroring::Unknown},
	{"UNL-KS7021A (VRC2b clone)",	"Kaiser",		NESMirroring::Unknown},
	{"Sangokushi: Chūgen no Hasha (bootleg)", nullptr,	NESMirroring::Unknown},
	{"Fudō Myōō Den (bootleg) (VRC2b clone)", nullptr,	NESMirroring::Unknown},
	{"1995 New Series Super 2-in-1 multicart", nullptr,	NESMirroring::Unknown},
	{"Datach Dragon Ball Z (bootleg) (VRC4e clone)", nullptr, NESMirroring::Unknown},

	// Mappers 530-539
	{"Super Mario Bros. Pocker Mali (VRC4f clone)", nullptr, NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Sachen 3014",			"Sachen",		NESMirroring::Unknown},
	{"2-in-1 Sudoku/Gomoku (NJ064) (MMC3 clone)", nullptr,	NESMirroring::Unknown},
	{"Nazo no Murasamejō (FDS conversion)", "Whirlwind Manu", NESMirroring::Unknown},
	{"Waixing FS303 (MMC3 clone) (same as 195)",	"Waixing", NESMirroring::Unknown},
	{"Waixing FS303 (MMC3 clone) (same as 195)",	"Waixing", NESMirroring::Unknown},
	{"60-1064-16L",			nullptr,		NESMirroring::Unknown},
	{"Kid Icarus (FDS conversion)",	nullptr,		NESMirroring::Unknown},

	// Mappers 540-549
	{"Master Fighter VI' hack (variant of 359)", nullptr,	NESMirroring::Unknown},
	{"LittleCom 160-in-1 multicart", nullptr,		NESMirroring::Unknown},	// Is LittleCom the company name?
	{"World Hero hack (VRC4 clone)", nullptr,		NESMirroring::Unknown},
	{"5-in-1 (CH-501) multicart (MMC1 clone)", nullptr,	NESMirroring::Unknown},
	{"Waixing FS306",		"Waixing",		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Konami QTa adapter (VRC5)",	"Konami",		NESMirroring::Unknown},
	{"CTC-15",			"Co Tung Co.",		NESMirroring::Unknown},
	{nullptr,			nullptr,		NESMirroring::Unknown},

	// Mappers 550-552
	{nullptr,			nullptr,		NESMirroring::Unknown},
	{"Jncota RPG re-release (variant of 178)", "Jncota",	NESMirroring::Unknown},
	{"Taito X1-017 (correct PRG ROM bank ordering)", "Taito", NESMirroring::Unknown},
};

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
struct SubmapperInfo {
	uint8_t submapper;		// Submapper number.
	uint8_t reserved;
	uint16_t deprecated;
	const char *desc;		// Description.
	NESMirroring mirroring;		// Mirroring behavior.
};

// Mapper 001: MMC1
const struct SubmapperInfo mmc1_submappers[] = {
	{1, 0,   1, "SUROM", NESMirroring::Unknown},
	{2, 0,   1, "SOROM", NESMirroring::Unknown},
	{3, 0, 155, "MMC1A", NESMirroring::Unknown},
	{4, 0,   1, "SXROM", NESMirroring::Unknown},
	{5, 0,   0, "SEROM, SHROM, SH1ROM", NESMirroring::Unknown},
};

// Discrete logic mappers: UxROM (002), CNROM (003), AxROM (007)
const struct SubmapperInfo discrete_logic_submappers[] = {
	{0, 0,   0, "Bus conflicts are unspecified", NESMirroring::Unknown},
	{1, 0,   0, "Bus conflicts do not occur", NESMirroring::Unknown},
	{2, 0,   0, "Bus conflicts occur, resulting in: bus AND rom", NESMirroring::Unknown},
};

// Mapper 004: MMC3
const struct SubmapperInfo mmc3_submappers[] = {
	{0, 0,      0, "MMC3C", NESMirroring::Unknown},
	{1, 0,      0, "MMC6", NESMirroring::Unknown},
	{2, 0, 0xFFFF, "MMC3C with hard-wired mirroring", NESMirroring::Unknown},
	{3, 0,      0, "MC-ACC", NESMirroring::Unknown},
	{4, 0,      0, "MMC3A", NESMirroring::Unknown},
};

// Mapper 016: Bandai FCG-x
const struct SubmapperInfo bandai_fcgx_submappers[] = {
	{1, 0, 159, "LZ93D50 with 24C01", NESMirroring::Unknown},
	{2, 0, 157, "Datach Joint ROM System", NESMirroring::Unknown},
	{3, 0, 153, "8 KiB of WRAM instead of serial EEPROM", NESMirroring::Unknown},
	{4, 0,   0, "FCG-1/2", NESMirroring::Unknown},
	{5, 0,   0, "LZ93D50 with optional 24C02", NESMirroring::Unknown},
};

// Mapper 019: Namco 129, 163
const struct SubmapperInfo namco_129_164_submappers[] = {
	{0, 0,   0, "Expansion sound volume unspecified", NESMirroring::Unknown},
	{1, 0,  19, "Internal RAM battery-backed; no expansion sound", NESMirroring::Unknown},
	{2, 0,   0, "No expansion sound", NESMirroring::Unknown},
	{3, 0,   0, "N163 expansion sound: 11.0-13.0 dB louder than NES APU", NESMirroring::Unknown},
	{4, 0,   0, "N163 expansion sound: 16.0-17.0 dB louder than NES APU", NESMirroring::Unknown},
	{5, 0,   0, "N163 expansion sound: 18.0-19.5 dB louder than NES APU", NESMirroring::Unknown},
};

// Mapper 021: Konami VRC4c, VRC4c
const struct SubmapperInfo vrc4a_vrc4c_submappers[] = {
	{1, 0,   0, "VRC4a", NESMirroring::Unknown},
	{2, 0,   0, "VRC4c", NESMirroring::Unknown},
};

// Mapper 023: Konami VRC4e, VRC4f, VRC2b
const struct SubmapperInfo vrc4ef_vrc2b_submappers[] = {
	{1, 0,   0, "VRC4f", NESMirroring::Unknown},
	{2, 0,   0, "VRC4e", NESMirroring::Unknown},
	{3, 0,   0, "VRC2b", NESMirroring::Unknown},
};

// Mapper 025: Konami VRC4b, VRC4d, VRC2c
const struct SubmapperInfo vrc4bd_vrc2c_submappers[] = {
	{1, 0,   0, "VRC4b", NESMirroring::Unknown},
	{2, 0,   0, "VRC4d", NESMirroring::Unknown},
	{3, 0,   0, "VRC2c", NESMirroring::Unknown},
};

// Mapper 032: Irem G101
const struct SubmapperInfo irem_g101_submappers[] = {
	{0, 0,   0, "Programmable mirroring", NESMirroring::Unknown},
	{1, 0,   0, "Fixed one-screen mirroring", NESMirroring::OneScreen_B},
};

// Mapper 034: BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
const struct SubmapperInfo bnrom_nina001_submappers[] = {
	{1, 0,   0, "NINA-001", NESMirroring::Unknown},
	{2, 0,   0, "BNROM", NESMirroring::Unknown},
};

// Mapper 068: Sunsoft-4
const struct SubmapperInfo sunsoft4_submappers[] = {
	{1, 0,   0, "Dual Cartridge System (NTB-ROM)", NESMirroring::Unknown},
};

// Mapper 071: Codemasters
const struct SubmapperInfo codemasters_submappers[] = {
	{1, 0,   0, "Programmable one-screen mirroring (Fire Hawk)", NESMirroring::MapperAB},
};

// Mapper 078: Cosmo Carrier / Holy Diver
const struct SubmapperInfo mapper078_submappers[] = {
	{1, 0,      0, "Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)", NESMirroring::MapperAB},
	{2, 0, 0xFFFF,  "Fixed vertical mirroring + WRAM", NESMirroring::Unknown},
	{3, 0,      0, "Programmable H/V mirroring (Holy Diver)", NESMirroring::MapperHV},
};

// Mapper 083: Cony/Yoko
const struct SubmapperInfo cony_yoko_submappers[] = {
	{0, 0,   0, "1 KiB CHR-ROM banking, no WRAM", NESMirroring::Unknown},
	{1, 0,   0, "2 KiB CHR-ROM banking, no WRAM", NESMirroring::Unknown},
	{2, 0,   0, "1 KiB CHR-ROM banking, 32 KiB banked WRAM", NESMirroring::Unknown},
};

// Mapper 114: Sugar Softec/Hosenkan
const struct SubmapperInfo mapper114_submappers[] = {
	{0, 0,   0, "MMC3 registers: 0,3,1,5,6,7,2,4", NESMirroring::Unknown},
	{1, 0,   0, "MMC3 registers: 0,2,5,3,6,1,7,4", NESMirroring::Unknown},
};

// Mapper 197: Kǎshèng (MMC3 clone)
const struct SubmapperInfo mapper197_submappers[] = {
	{0, 0,   0, "Super Fighter III (PRG-ROM CRC32 0xC333F621)", NESMirroring::Unknown},
	{1, 0,   0, "Super Fighter III (PRG-ROM CRC32 0x2091BEB2)", NESMirroring::Unknown},
	{2, 0,   0, "Mortal Kombat III Special", NESMirroring::Unknown},
	{3, 0,   0, "1995 Super 2-in-1", NESMirroring::Unknown},
};

// Mapper 210: Namcot 175, 340
const struct SubmapperInfo namcot_175_340_submappers[] = {
	{1, 0,   0, "Namcot 175 (fixed mirroring)",        NESMirroring::Header},
	{2, 0,   0, "Namcot 340 (programmable mirroring)", NESMirroring::MapperHVAB},
};

// Mapper 215: Sugar Softec
const struct SubmapperInfo sugar_softec_submappers[] = {
	{0, 0,   0, "UNL-8237", NESMirroring::Unknown},
	{1, 0,   0, "UNL-8237A", NESMirroring::Unknown},
};

// Mapper 232: Codemasters Quattro
const struct SubmapperInfo quattro_submappers[] = {
	{1, 0,   0, "Aladdin Deck Enhancer", NESMirroring::Unknown},
};

// Mapper 256: OneBus Famiclones
const struct SubmapperInfo onebus_submappers[] = {
	{ 1, 0,   0, "Waixing VT03", NESMirroring::Unknown},
	{ 2, 0,   0, "Power Joy Supermax", NESMirroring::Unknown},
	{ 3, 0,   0, "Zechess/Hummer Team", NESMirroring::Unknown},
	{ 4, 0,   0, "Sports Game 69-in-1", NESMirroring::Unknown},
	{ 5, 0,   0, "Waixing VT02", NESMirroring::Unknown},
	{14, 0,   0, "Karaoto", NESMirroring::Unknown},
	{15, 0,   0, "Jungletac", NESMirroring::Unknown},
};

// Mapper 268: SMD132/SMD133
const struct SubmapperInfo smd132_smd133_submappers[] = {
	{0, 0,   0, "COOLBOY ($6000-$7FFF)", NESMirroring::Unknown},
	{1, 0,   0, "MINDKIDS ($5000-$5FFF)", NESMirroring::Unknown},
};

// Mapper 313: Reset-based multicart (MMC3)
const struct SubmapperInfo mapper313_submappers[] = {
	{0, 0,   0, "Game size: 128 KiB PRG, 128 KiB CHR", NESMirroring::Unknown},
	{1, 0,   0, "Game size: 256 KiB PRG, 128 KiB CHR", NESMirroring::Unknown},
	{2, 0,   0, "Game size: 128 KiB PRG, 256 KiB CHR", NESMirroring::Unknown},
	{3, 0,   0, "Game size: 256 KiB PRG, 256 KiB CHR", NESMirroring::Unknown},
	{4, 0,   0, "Game size: 256 KiB PRG (first game); 128 KiB PRG (other games); 128 KiB CHR", NESMirroring::Unknown},
};

/**
 * NES 2.0 submapper list.
 *
 * NOTE: Submapper 0 is optional, since it's the default behavior.
 * It may be included if NES 2.0 submapper 0 acts differently from
 * iNES mappers.
 */

struct SubmapperEntry {
	uint16_t mapper;		// Mapper number.
	uint16_t info_size;		// Number of entries in info.
	const SubmapperInfo *info;	// Submapper information.
};

#define NES2_SUBMAPPER(num, arr) {num, (uint16_t)ARRAY_SIZE(arr), arr}
const SubmapperEntry submappers[] = {
	NES2_SUBMAPPER(  1, mmc1_submappers),			// MMC1
	NES2_SUBMAPPER(  2, discrete_logic_submappers),		// UxROM
	NES2_SUBMAPPER(  3, discrete_logic_submappers),		// CNROM
	NES2_SUBMAPPER(  4, mmc3_submappers),			// MMC3
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
	NES2_SUBMAPPER(114, mapper114_submappers),
	NES2_SUBMAPPER(197, mapper197_submappers),		// Kǎshèng (MMC3 clone)
	NES2_SUBMAPPER(210, namcot_175_340_submappers),		// Namcot 175, 340
	NES2_SUBMAPPER(215, sugar_softec_submappers),		// Sugar Softec (MMC3 clone)
	NES2_SUBMAPPER(232, quattro_submappers),		// Codemasters Quattro
	NES2_SUBMAPPER(256, onebus_submappers),			// OneBus Famiclones
	NES2_SUBMAPPER(268, smd132_smd133_submappers),		// SMD132/SMD133
	NES2_SUBMAPPER(313, mapper313_submappers),		// Reset-based multicart (MMC3)

	{0, 0, nullptr}
};

/**
 * bsearch() comparison function for SubmapperInfo.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API SubmapperInfo_compar(const void *a, const void *b)
{
	uint8_t submapper1 = static_cast<const SubmapperInfo*>(a)->submapper;
	uint8_t submapper2 = static_cast<const SubmapperInfo*>(b)->submapper;
	if (submapper1 < submapper2) return -1;
	if (submapper1 > submapper2) return 1;
	return 0;
}

/**
 * bsearch() comparison function for SubmapperEntry.
 * @param a
 * @param b
 * @return
 */
static int RP_C_API SubmapperEntry_compar(const void *a, const void *b)
{
	uint16_t mapper1 = static_cast<const SubmapperEntry*>(a)->mapper;
	uint16_t mapper2 = static_cast<const SubmapperEntry*>(b)->mapper;
	if (mapper1 < mapper2) return -1;
	if (mapper1 > mapper2) return 1;
	return 0;
}

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper info, or nullptr if not found.
 */
const MapperEntry *lookup_ines_info(int mapper)
{
	assert(mapper >= 0);
	if (mapper < 0) {
		// Mapper number is out of range.
		return nullptr;
	}

	if (mapper < 256) {
		// NES 2.0 Plane 0 [000-255] (iNES 1.0)
		static_assert(sizeof(mappers_plane0) == (256 * sizeof(MapperEntry)),
			"mappers_plane0[] doesn't have 256 entries.");
		return &mappers_plane0[mapper];
	} else if (mapper < 512) {
		// NES 2.0 Plane 1 [256-511]
		mapper -= 256;
		if (mapper >= ARRAY_SIZE_I(mappers_plane1)) {
			// Mapper number is out of range for plane 1.
			return nullptr;
		}
		return &mappers_plane1[mapper];
	} else if (mapper < 768) {
		// NES 2.0 Plane 2 [512-767]
		mapper -= 512;
		if (mapper >= ARRAY_SIZE_I(mappers_plane2)) {
			// Mapper number is out of range for plane 2.
			return nullptr;
		}
		return &mappers_plane2[mapper];
	}

	// Invalid mapper number.
	return nullptr;
}

/**
 * Look up an NES 2.0 submapper number.
 * @param mapper Mapper number.
 * @param submapper Submapper number.
 * @return Submapper info, or nullptr if not found.
 */
const SubmapperInfo *lookup_nes2_submapper_info(int mapper, int submapper)
{
	assert(mapper >= 0);
	assert(submapper >= 0);
	assert(submapper < 16);
	if (mapper < 0 || submapper < 0 || submapper >= 16) {
		// Mapper or submapper number is out of range.
		return nullptr;
	}

	// Do a binary search in submappers[].
	const SubmapperEntry key = { static_cast<uint16_t>(mapper), 0, nullptr };
	const SubmapperEntry *res =
		static_cast<const SubmapperEntry*>(bsearch(&key,
			submappers,
			ARRAY_SIZE(submappers)-1,
			sizeof(SubmapperEntry),
			SubmapperEntry_compar));
	if (!res || !res->info || res->info_size == 0)
		return nullptr;

	// Do a binary search in res->info.
	const SubmapperInfo key2 = { static_cast<uint8_t>(submapper), 0, 0, nullptr, NESMirroring::Unknown };
	const SubmapperInfo *res2 =
		static_cast<const SubmapperInfo*>(bsearch(&key2,
			res->info, res->info_size,
			sizeof(SubmapperInfo),
			SubmapperInfo_compar));
	return res2;
}

/** Public functions **/

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper name, or nullptr if not found.
 */
const char *lookup_ines(int mapper)
{
	const MapperEntry *ent = lookup_ines_info(mapper);
	return ent ? ent->name : nullptr;
}

/**
 * Convert a TNES mapper number to iNES.
 * @param tnes_mapper TNES mapper number.
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
const char *lookup_nes2_submapper(int mapper, int submapper)
{
	const SubmapperInfo *ent = lookup_nes2_submapper_info(mapper, submapper);
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
	const MapperEntry *ent1 = lookup_ines_info(mapper);
	const SubmapperInfo *ent2 = lookup_nes2_submapper_info(mapper, submapper);
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

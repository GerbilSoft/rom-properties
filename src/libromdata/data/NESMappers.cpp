/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.cpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
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

#include "NESMappers.hpp"

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

/**
 * References:
 * - https://wiki.nesdev.com/w/index.php/Mapper
 * - https://wiki.nesdev.com/w/index.php/NES_2.0_submappers
 */

class NESMappersPrivate
{
	private:
		// Static class.
		NESMappersPrivate();
		~NESMappersPrivate();
		RP_DISABLE_COPY(NESMappersPrivate)

	public:
		// iNES mapper list.
		struct MapperEntry {
			const char *name;
			// TODO: Add manufacturers.
			//const char *manufacturer;
		};
		static const MapperEntry mappers[];

		/**
		 * NES 2.0 submapper information.
		 */
		struct SubmapperInfo {
			uint8_t submapper;	// Submapper number.
			bool deprecated;	// Deprecated. (TODO: Show replacement?)
			const char *desc;	// Description.
		};

		// Submappers.
		static const struct SubmapperInfo mmc1_submappers[];
		static const struct SubmapperInfo discrete_logic_submappers[];
		static const struct SubmapperInfo mmc3_submappers[];
		static const struct SubmapperInfo irem_g101_submappers[];
		static const struct SubmapperInfo bnrom_nina001_submappers[];
		static const struct SubmapperInfo sunsoft4_submappers[];
		static const struct SubmapperInfo codemasters_submappers[];
		static const struct SubmapperInfo mapper078_submappers[];
		static const struct SubmapperInfo namcot_175_340_submappers[];
		static const struct SubmapperInfo quattro_submappers[];

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
		static const SubmapperEntry submappers[];

		/**
		 * bsearch() comparison function for SubmapperInfo.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API SubmapperInfo_compar(const void *a, const void *b);

		/**
		 * bsearch() comparison function for SubmapperEntry.
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API SubmapperEntry_compar(const void *a, const void *b);
};

/**
 * iNES mapper list.
 * TODO: Add more fields:
 * - Publisher name (separate from mapper)
 * - Programmable mirroring
 * - Extra VRAM for 4 screens
 */
const NESMappersPrivate::MapperEntry NESMappersPrivate::mappers[] = {
	// Mappers 0-9
	{"NROM"},
	{"SxROM (MMC1)"},
	{"UxROM"},
	{"CNROM"},
	{"TxROM (MMC3, MMC6)"},
	{"ExROM (MMC5)"},
	{"FFE #6"},
	{"AxROM"},
	{"FFE #8"},
	{"PxROM (MMC2)"},

	// Mappers 10-19
	{"FxROM (MMC4)"},
	{"Color Dreams"},
	{"MMC3 clone"}, // this is also marked as another FFE mapper. conflict?
	{"CPROM"},
	{"MMC3 clone"},
	{"Multicart (unlicensed)"},
	{"FCG-x"},
	{"FFE #17"},
	{"Jaleco SS88006"},
	{"Namco 129/163"},

	// Mappers 20-29
	{"Famicom Disk System"}, // this isn't actually used, as FDS roms are stored in their own format.
	{"VRC4a, VRC4c"},
	{"VRC2a"},
	{"VRC4e, VRC4f, VRC2b"},
	{"VRC6a"},
	{"VRC4b, VRC4d"},
	{"VRC6b"},
	{"VRC4 variant"}, //investigate
	{"Multicart/Homebrew"},
	{"Homebrew"},

	// Mappers 30-39
	{"UNROM 512 (Homebrew)"},
	{"NSF (Homebrew)"},
	{"Irem G-101"},
	{"Taito TC0190"},
	{"BNROM, NINA-001"},
	{"JY Company subset"},
	{"TXC PCB 01-22000-400"},
	{"MMC3 multicart (official)"},
	{"GNROM variant"},
	{"BNROM variant"},

	// Mappers 40-49
	{"CD4020 (FDS conversion) (unlicensed)"},
	{"Caltron 6-in-1"},
	{"FDS conversion (unlicensed)"},
	{"FDS conversion (unlicensed)"},
	{"MMC3 clone"},
	{"MMC3 multicart (unlicensed)"},
	{"Rumble Station"},
	{"MMC3 multicart (official)"},
	{"Taito TC0690"},
	{"MMC3 multicart (unlicensed)"},

	// Mappers 50-59
	{"FDS conversion (unlicensed)"},
	{nullptr},
	{"MMC3 clone"},
	{nullptr},
	{"Novel Diamond"},	// conflicting information
	{nullptr},
	{nullptr},
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{nullptr},

	// Mappers 60-69
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{nullptr},
	{"Tengen RAMBO-1"},
	{"Irem H3001"},
	{"GxROM, MHROM"},
	{"Sunsoft-3"},
	{"Sunsoft-4"},
	{"Sunsoft FME-7"},

	// Mappers 70-79
	{"Family Trainer"},
	{"Codemasters (UNROM clone)"},
	{"Jaleco JF-17"},
	{"Konami VRC3"},
	{"MMC3 clone"},
	{"Konami VRC1"},
	{"Namcot 108 variant"},
	{nullptr},
	{"Holy Diver; Uchuusen - Cosmo Carrier"},
	{"NINA-03, NINA-06"},

	// Mappers 80-89
	{"Taito X1-005"},
	{nullptr},
	{"Taito X1-017"},
	{"Cony Soft"},
	{"PC-SMB2J"},
	{"Konami VRC7"},
	{"Jaleco JF-13"},
	{nullptr},
	{"Namcot 118 variant"},
	{"Sunsoft-2 (Sunsoft-3 board)"},	

	// Mappers 90-99
	{"JY Company (simple nametable control)"},
	{nullptr},
	{nullptr},
	{"Sunsoft-2 (Sunsoft-3R board)"},
	{"HVC-UN1ROM"},
	{"NAMCOT-3425"},
	{"Oeka Kids"},
	{"Irem TAM-S1"},
	{nullptr},
	{"CNROM (Vs. System)"},

	// Mappers 100-109
	{"MMC3 variant (hacked ROMs)"},	// Also used for UNIF
	{"Jaleco JF-10 (misdump)"},
	{nullptr},
	{nullptr},
	{nullptr},
	{"NES-EVENT (Nintendo World Championships 1990)"},
	{nullptr},
	{"Magic Dragon"},
	{nullptr},
	{nullptr},

	// Mappers 110-119
	{nullptr},
	{"Cheapocabra GT-ROM 512k flash board"},	// Membler Industries
	{"Namcot 118 variant"},
	{"NINA-03/06 multicart"},
	{nullptr},
	{"MMC3 clone (Carson)"},
	{"Copy-protected bootleg mapper"},
	{nullptr},
	{"TxSROM"},
	{"TQROM"},

	// Mappers 120-129
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},

	// Mappers 130-139
	{nullptr},
	{nullptr},
	{nullptr},
	{"Sachen Jovial Race"},
	{nullptr},
	{nullptr},
	{nullptr},
	{"Sachen 8259D"},
	{"Sachen 8259B"},
	{"Sachen 8259C"},

	// Mappers 140-149
	{"Jaleco JF-11, JF-14 (GNROM variant)"},
	{"Sachen 8259A"},
	{nullptr},
	{"Copy-protected NROM"},
	{"Death Race (Color Dreams variant)"},
	{"Sidewinder (CNROM clone)"},
	{"Galactic Crusader (NINA-06 clone)"},
	{"Sachen Challenge of the Dragon"},
	{"NINA-06 variant"},
	{"SA-0036 (CNROM clone)"},

	// Mappers 150-159
	{"Sachen 74LS374N (corrected)"},
	{"VRC1 (Vs. System)"},
	{nullptr},
	{"Bandai LZ93D50 with SRAM"},
	{"NAMCOT-3453"},
	{"MMC1A"},
	{"DIS23C01"},
	{"Datach Joint ROM System"},
	{"Tengen 800037"},
	{"Bandai LZ93D50 with 24C01"},

	// Mappers 160-169
	{nullptr},
	{nullptr},
	{nullptr},
	{"Nanjing"},
	{nullptr},
	{nullptr},
	{"SUBOR"},
	{"SUBOR"},
	{"Racermate Challenge 2"},
	{"Yuxing"},

	// Mappers 170-179
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{"Multicart (unlicensed)"},
	{nullptr},
	{"WaiXing (BMCFK23C?)"},
	{nullptr},
	{"Education / WaiXing / HengGe"},
	{nullptr},

	// Mappers 180-189
	{"Crazy Climber (UNROM clone)"},
	{nullptr},
	{"MMC3 variant"},
	{"Suikan Pipe (VRC4e clone)"},
	{"Sunsoft-1"},
	{"CNROM with weak copy protection"},
	{"Study Box"},
	{nullptr},
	{"Bandai Karaoke Studio"},
	{"Thunder Warrior (MMC3 clone)"},

	// Mappers 190-199
	{nullptr},
	{"MMC3 clone"},
	{"MMC3 clone"},
	{"NTDEC TC-112"},
	{"MMC3 clone"},	// same as 195 on NES 2.0
	{"MMC3 clone"},	// same as 194 on NES 2.0
	{"MMC3 clone"},
	{nullptr},
	{nullptr},
	{nullptr},

	// Mappers 200-209
	{"Multicart (unlicensed)"},
	{"NROM-256 multicart (unlicensed)"},
	{"150-in-1 multicart (unlicensed)"},
	{"35-in-1 multicart (unlicensed)"},
	{nullptr},
	{"MMC3 multicart (unlicensed)"},
	{"DxROM (Tengen MIMIC-1, Namcot 118)"},
	{"Fudou Myouou Den"},
	{nullptr},
	{"JY Company (MMC2/MMC4 clone)"},

	// Mappers 210-219
	{"Namcot 175, 340"},
	{"JY Company (extended nametable control)"},
	{nullptr},
	{nullptr},
	{nullptr},
	{"MMC3 clone"},
	{nullptr},
	{nullptr},
	{"Magic Floor (homebrew)"},
	{"UNL A9746"},

	// Mappers 220-229
	{"Summer Carnival '92 - Recca"},
	{nullptr},
	{"CTC-31 (VRC2 + 74xx)"},
	{nullptr},
	{nullptr},
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{"Active Enterprises"},
	{"BMC 31-IN-1"},

	// Mappers 230-239
	{"Multicart (unlicensed)"},
	{"Multicart (unlicensed)"},
	{"Codemasters Quattro"},
	{"Multicart (unlicensed)"},
	{"Maxi 15 multicart"},
	{nullptr},
	{nullptr},
	{"Teletubbies 420-in-1 multicart"},
	{nullptr},
	{nullptr},

	// Mappers 240-249
	{"Multicart (unlicensed)"},
	{"BNROM (similar to 034)"},
	{"Unlicensed"},
	{"Sachen 74LS374N"},
	{nullptr},
	{"MMC3 clone"},
	{"Feng Shen Bang - Zhu Lu Zhi Zhan"},
	{nullptr},
	{"Incorrect assignment (should be 115)"},
	{nullptr},

	// Mappers 250-255
	{"Nitra (MMC3 clone)"},
	{nullptr},
	{"WaiXing - Sangokushi"},
	{nullptr},
	{"Pikachu Y2K of crypted ROMs"},
	{nullptr},
};

/** Submappers. **/

// MMC1
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc1_submappers[] = {
	{1, true,  "SUROM"},
	{2, true,  "SOROM"},
	{3, true,  "MMC1A"},
	{4, true,  "SXROM"},
	{5, false,  "SEROM, SHROM, SH1ROM"},
};

// Discrete logic mappers: UxROM, CNROM, AxROM
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::discrete_logic_submappers[] = {
	{1, false, "Bus conflicts do not occur"},
	{2, false, "Bus conflicts occur, resulting in: bus AND rom"},
};

// MMC3
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc3_submappers[] = {
	{0, false, "MMC3C"},
	{1, false, "MMC6"},
	{2, true,  "MMC3C with hard-wired mirroring"},
	{3, false, "MC-ACC"},
};

// Irem G101
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::irem_g101_submappers[] = {
	// TODO: Some field to indicate mirroring override?
	{0, false, "Programmable mirroring"},
	{1, false, "Fixed one-screen mirroring"},
};

// BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::bnrom_nina001_submappers[] = {
	{1, false, "NINA-001"},
	{2, false, "BNROM"},
};

// Sunsoft-4
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::sunsoft4_submappers[] = {
	{1, false, "Dual Cartridge System (NTB-ROM)"},
};

// Codemasters
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::codemasters_submappers[] = {
	{1, false, "Programmable one-screen mirroring (Fire Hawk)"},
};

const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper078_submappers[] = {
	{1, false, "Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)"},
	{2, true,  "Fixed vertical mirroring + WRAM"},
	{3, false, "Programmable H/V mirroring (Holy Diver)"},
};

// Namcot 175, 340
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::namcot_175_340_submappers[] = {
	{1, false, "Namcot 175 (fixed mirroring)"},
	{2, false, "Namcot 340 (programmable mirroring)"},
};

// Codemasters Quattro
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::quattro_submappers[] = {
	{1, false, "Aladdin Deck Enhancer"},
};

/**
 * NES 2.0 submapper list.
 *
 * NOTE: Submapper 0 is optional, since it's the default behavior.
 * It may be included if NES 2.0 submapper 0 acts differently from
 * iNES mappers.
 */
#define NES2_SUBMAPPER(num, arr) {num, (uint16_t)ARRAY_SIZE(arr), arr}
const NESMappersPrivate::SubmapperEntry NESMappersPrivate::submappers[] = {
	NES2_SUBMAPPER(  1, mmc1_submappers),			// MMC1
	NES2_SUBMAPPER(  2, discrete_logic_submappers),		// UxROM
	NES2_SUBMAPPER(  3, discrete_logic_submappers),		// CNROM
	NES2_SUBMAPPER(  4, mmc3_submappers),			// MMC3
	NES2_SUBMAPPER(  7, discrete_logic_submappers),		// AxROM
	NES2_SUBMAPPER( 32, irem_g101_submappers),		// Irem G101
	NES2_SUBMAPPER( 34, bnrom_nina001_submappers),		// BNROM / NINA-001
	NES2_SUBMAPPER( 68, sunsoft4_submappers),		// Sunsoft-4
	NES2_SUBMAPPER( 71, codemasters_submappers),		// Codemasters
	NES2_SUBMAPPER( 78, mapper078_submappers),
	NES2_SUBMAPPER(210, namcot_175_340_submappers),		// Namcot 175, 340
	NES2_SUBMAPPER(232, quattro_submappers),		// Codemasters Quattro

	{0, 0, nullptr}
};

/**
 * bsearch() comparison function for SubmapperInfo.
 * @param a
 * @param b
 * @return
 */
int RP_C_API NESMappersPrivate::SubmapperInfo_compar(const void *a, const void *b)
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
int RP_C_API NESMappersPrivate::SubmapperEntry_compar(const void *a, const void *b)
{
	uint16_t mapper1 = static_cast<const SubmapperEntry*>(a)->mapper;
	uint16_t mapper2 = static_cast<const SubmapperEntry*>(b)->mapper;
	if (mapper1 < mapper2) return -1;
	if (mapper1 > mapper2) return 1;
	return 0;
}

/** NESMappers **/

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper name, or nullptr if not found.
 */
const char *NESMappers::lookup_ines(int mapper)
{
	static_assert(sizeof(NESMappersPrivate::mappers) == (256 * sizeof(NESMappersPrivate::MapperEntry)),
		"NESMappersPrivate::mappers[] doesn't have 256 entries.");
	assert(mapper >= 0);
	assert(mapper < ARRAY_SIZE(NESMappersPrivate::mappers));
	if (mapper < 0 || mapper >= ARRAY_SIZE(NESMappersPrivate::mappers)) {
		// Mapper number is out of range.
		return nullptr;
	}

	return NESMappersPrivate::mappers[mapper].name;
}

/**
 * Convert a TNES mapper number to iNES.
 * @param tnes_mapper TNES mapper number.
 * @return iNES mapper number, or -1 if unknown.
 */
int NESMappers::tnesMapperToInesMapper(int tnes_mapper)
{
	static const int8_t ines_mappers[10] = {
		0,	// TNES 0 = NROM
		1,	// TNES 1 = SxROM (MMC1)
		9,	// TNES 2 = PxROM (MMC2)
		4,	// TNES 3 = TxROM (MMC3)
		10,	// TNES 4 = FxROM (MMC4)
		5,	// TNES 5 = ExROM (MMC5)
		2,	// TNES 6 = UxROM
		3,	// TNES 7 = CNROM
		-1,	// TNES 8 = undefined
		7,	// TNES 9 = AxROM
	};

	if (tnes_mapper < 0 || tnes_mapper >= ARRAY_SIZE(ines_mappers)) {
		// Undefined TNES mapper.
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
const char *NESMappers::lookup_nes2_submapper(int mapper, int submapper)
{
	assert(mapper >= 0);
	assert(mapper < ARRAY_SIZE(NESMappersPrivate::mappers));
	assert(submapper >= 0);
	assert(submapper < 256);
	if (mapper < 0 || mapper >= ARRAY_SIZE(NESMappersPrivate::mappers) ||
	    submapper < 0 || submapper >= 256)
	{
		return nullptr;
	}

	// Do a binary search in submappers[].
	const NESMappersPrivate::SubmapperEntry key = {(uint16_t)mapper, 0, nullptr};
	const NESMappersPrivate::SubmapperEntry *res =
		static_cast<const NESMappersPrivate::SubmapperEntry*>(bsearch(&key,
			NESMappersPrivate::submappers,
			ARRAY_SIZE(NESMappersPrivate::submappers)-1,
			sizeof(NESMappersPrivate::SubmapperEntry),
			NESMappersPrivate::SubmapperEntry_compar));
	if (!res || !res->info || res->info_size == 0)
		return nullptr;

	// Do a minary search in res->info.
	const NESMappersPrivate::SubmapperInfo key2 = {(uint8_t)submapper, false, nullptr};
	const NESMappersPrivate::SubmapperInfo *res2 =
		static_cast<const NESMappersPrivate::SubmapperInfo*>(bsearch(&key2,
			res->info, res->info_size,
			sizeof(NESMappersPrivate::SubmapperInfo),
			NESMappersPrivate::SubmapperInfo_compar));
	// TODO: Return the "deprecated" value?
	return (res2 ? res2->desc : nullptr);
}

}

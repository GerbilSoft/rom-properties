/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * INESMappers.cpp: List of iNES mappers                                   *
 *                                                                         *
 * Copyright (c) 2016 by Egor.                                             *
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

#include "NESMappers.hpp"
#include "common.h"

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
		NESMappersPrivate(const NESMappersPrivate &other);
		NESMappersPrivate &operator=(const NESMappersPrivate &other);

	public:
		// iNES mapper list.
		struct MapperEntry {
			const rp_char *name;
			// TODO: Add manufacturers.
			//const rp_char *manufacturer;
		};
		static const MapperEntry mappers[];

		/**
		 * NES 2.0 submapper information.
		 */
		struct SubmapperInfo {
			uint8_t submapper;	// Submapper number.
			const rp_char *desc;	// Description.
			bool deprecated;	// Deprecated. (TODO: Show replacement?)
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
			const SubmapperInfo *info;	// Submapper information.
			int info_size;			// Number of entries in info.
		};
		static const SubmapperEntry submappers[];
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
	{_RP("NROM")},
	{_RP("SxROM (MMC1)")},
	{_RP("UxROM")},
	{_RP("CNROM")},
	{_RP("TxROM (MMC3, MMC6)")},
	{_RP("ExROM (MMC5)")},
	{_RP("FFE #6")},
	{_RP("AxROM")},
	{_RP("FFE #8")},
	{_RP("PxROM (MMC2)")},

	// Mappers 10-19
	{_RP("FxROM (MMC4)")},
	{_RP("Color Dreams")},
	{_RP("MMC3 clone")}, // this is also marked as another FFE mapper. conflict?
	{_RP("CPROM")},
	{_RP("MMC3 clone")},
	{_RP("Multicart (unlicensed)")},
	{_RP("FCG-x")},
	{_RP("FFE #17")},
	{_RP("Jaleco SS88006")},
	{_RP("Namco 129/163")},

	// Mappers 20-29
	{_RP("Famicom Disk System")}, // this isn't actually used, as FDS roms are stored in their own format.
	{_RP("VRC4a, VRC4c")},
	{_RP("VRC2a")},
	{_RP("VRC4e, VRC4f, VRC2b")},
	{_RP("VRC6a")},
	{_RP("VRC4b, VRC4d")},
	{_RP("VRC6b")},
	{_RP("VRC4 variant")}, //investigate
	{_RP("Multicart/Homebrew")},
	{_RP("Homebrew")},

	// Mappers 30-39
	{_RP("UNROM 512 (Homebrew)")},
	{_RP("NSF (Homebrew)")},
	{_RP("Irem G-101")},
	{_RP("Taito TC0190")},
	{_RP("BNROM, NINA-001")},
	{_RP("JY Company subset")},
	{_RP("TXC PCB 01-22000-400")},
	{_RP("MMC3 multicart (official)")},
	{_RP("GNROM variant")},
	{_RP("BNROM variant")},

	// Mappers 40-49
	{_RP("CD4020 (FDS conversion) (unlicensed)")},
	{_RP("Caltron 6-in-1")},
	{_RP("FDS conversion (unlicensed)")},
	{_RP("FDS conversion (unlicensed)")},
	{_RP("MMC3 clone")},
	{_RP("MMC3 multicart (unlicensed)")},
	{_RP("Rumble Station")},
	{_RP("MMC3 multicart (official)")},
	{_RP("Taito TC0690")},
	{_RP("MMC3 multicart (unlicensed)")},

	// Mappers 50-59
	{_RP("FDS conversion (unlicensed)")},
	{nullptr},
	{_RP("MMC3 clone")},
	{nullptr},
	{_RP("Novel Diamond")},	// conflicting information
	{nullptr},
	{nullptr},
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{nullptr},

	// Mappers 60-69
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{nullptr},
	{_RP("Tengen RAMBO-1")},
	{_RP("Irem H3001")},
	{_RP("GxROM, MHROM")},
	{_RP("Sunsoft-3")},
	{_RP("Sunsoft-4")},
	{_RP("Sunsoft FME-7")},

	// Mappers 70-79
	{_RP("Family Trainer")},
	{_RP("Codemasters (UNROM clone)")},
	{_RP("Jaleco JF-17")},
	{_RP("Konami VRC3")},
	{_RP("MMC3 clone")},
	{_RP("Konami VRC1")},
	{_RP("Namcot 108 variant")},
	{nullptr},
	{_RP("Holy Diver; Uchuusen - Cosmo Carrier")},
	{_RP("NINA-03, NINA-06")},

	// Mappers 80-89
	{_RP("Taito X1-005")},
	{nullptr},
	{_RP("Taito X1-017")},
	{_RP("Cony Soft")},
	{_RP("PC-SMB2J")},
	{_RP("Konami VRC7")},
	{_RP("Jaleco JF-13")},
	{nullptr},
	{_RP("Namcot 118 variant")},
	{_RP("Sunsoft-2 (Sunsoft-3 board)")},	

	// Mappers 90-99
	{_RP("JY Company (simple nametable control)")},
	{nullptr},
	{nullptr},
	{_RP("Sunsoft-2 (Sunsoft-3R board)")},
	{_RP("HVC-UN1ROM")},
	{_RP("NAMCOT-3425")},
	{_RP("Oeka Kids")},
	{_RP("Irem TAM-S1")},
	{nullptr},
	{_RP("CNROM (Vs. System)")},

	// Mappers 100-109
	{_RP("MMC3 variant (hacked ROMs)")},	// Also used for UNIF
	{_RP("Jaleco JF-10 (misdump)")},
	{nullptr},
	{nullptr},
	{nullptr},
	{_RP("NES-EVENT (Nintendo World Championships 1990)")},
	{nullptr},
	{_RP("Magic Dragon")},
	{nullptr},
	{nullptr},

	// Mappers 110-119
	{nullptr},
	{_RP("Cheapocabra GT-ROM 512k flash board")},	// Membler Industries
	{_RP("Namcot 118 variant")},
	{_RP("NINA-03/06 multicart")},
	{nullptr},
	{_RP("MMC3 clone (Carson)")},
	{_RP("Copy-protected bootleg mapper")},
	{nullptr},
	{_RP("TxSROM")},
	{_RP("TQROM")},

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
	{_RP("Sachen Jovial Race")},
	{nullptr},
	{nullptr},
	{nullptr},
	{_RP("Sachen 8259D")},
	{_RP("Sachen 8259B")},
	{_RP("Sachen 8259C")},

	// Mappers 140-149
	{_RP("Jaleco JF-11, JF-14 (GNROM variant)")},
	{_RP("Sachen 8259A")},
	{nullptr},
	{_RP("Copy-protected NROM")},
	{_RP("Death Race (Color Dreams variant)")},
	{_RP("Sidewinder (CNROM clone)")},
	{_RP("Galactic Crusader (NINA-06 clone)")},
	{_RP("Sachen Challenge of the Dragon")},
	{_RP("NINA-06 variant")},
	{_RP("SA-0036 (CNROM clone)")},

	// Mappers 150-159
	{_RP("Sachen 74LS374N (corrected)")},
	{_RP("VRC1 (Vs. System)")},
	{nullptr},
	{_RP("Bandai LZ93D50 with SRAM")},
	{_RP("NAMCOT-3453")},
	{_RP("MMC1A")},
	{_RP("DIS23C01")},
	{_RP("Datach Joint ROM System")},
	{_RP("Tengen 800037")},
	{_RP("Bandai LZ93D50 with 24C01")},

	// Mappers 160-169
	{nullptr},
	{nullptr},
	{nullptr},
	{_RP("Nanjing")},
	{nullptr},
	{nullptr},
	{_RP("SUBOR")},
	{_RP("SUBOR")},
	{_RP("Racermate Challenge 2")},
	{_RP("Yuxing")},

	// Mappers 170-179
	{nullptr},
	{nullptr},
	{nullptr},
	{nullptr},
	{_RP("Multicart (unlicensed)")},
	{nullptr},
	{_RP("WaiXing (BMCFK23C?)")},
	{nullptr},
	{_RP("Education / WaiXing / HengGe")},
	{nullptr},

	// Mappers 180-189
	{_RP("Crazy Climber (UNROM clone)")},
	{nullptr},
	{_RP("MMC3 variant")},
	{_RP("Suikan Pipe (VRC4e clone)")},
	{_RP("Sunsoft-1")},
	{_RP("CNROM with weak copy protection")},
	{_RP("Study Box")},
	{nullptr},
	{_RP("Bandai Karaoke Studio")},
	{_RP("Thunder Warrior (MMC3 clone)")},

	// Mappers 190-199
	{nullptr},
	{_RP("MMC3 clone")},
	{_RP("MMC3 clone")},
	{_RP("NTDEC TC-112")},
	{_RP("MMC3 clone")},	// same as 195 on NES 2.0
	{_RP("MMC3 clone")},	// same as 194 on NES 2.0
	{_RP("MMC3 clone")},
	{nullptr},
	{nullptr},
	{nullptr},

	// Mappers 200-209
	{_RP("Multicart (unlicensed)")},
	{_RP("NROM-256 multicart (unlicensed)")},
	{_RP("150-in-1 multicart (unlicensed)")},
	{_RP("35-in-1 multicart (unlicensed)")},
	{nullptr},
	{_RP("MMC3 multicart (unlicensed)")},
	{_RP("DxROM (Tengen MIMIC-1, Namcot 118)")},
	{_RP("Fudou Myouou Den")},
	{nullptr},
	{_RP("JY Company (MMC2/MMC4 clone)")},

	// Mappers 210-219
	{_RP("Namcot 175, 340")},
	{_RP("JY Company (extended nametable control)")},
	{nullptr},
	{nullptr},
	{nullptr},
	{_RP("MMC3 clone")},
	{nullptr},
	{nullptr},
	{_RP("Magic Floor (homebrew)")},
	{_RP("UNL A9746")},

	// Mappers 220-229
	{_RP("Summer Carnival '92 - Recca")},
	{nullptr},
	{_RP("CTC-31 (VRC2 + 74xx)")},
	{nullptr},
	{nullptr},
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{_RP("Active Enterprises")},
	{_RP("BMC 31-IN-1")},

	// Mappers 230-239
	{_RP("Multicart (unlicensed)")},
	{_RP("Multicart (unlicensed)")},
	{_RP("Codemasters Quattro")},
	{_RP("Multicart (unlicensed)")},
	{_RP("Maxi 15 multicart")},
	{nullptr},
	{nullptr},
	{_RP("Teletubbies 420-in-1 multicart")},
	{nullptr},
	{nullptr},

	// Mappers 240-249
	{_RP("Multicart (unlicensed)")},
	{_RP("BNROM (similar to 034)")},
	{_RP("Unlicensed")},
	{_RP("Sachen 74LS374N")},
	{nullptr},
	{_RP("MMC3 clone")},
	{_RP("Feng Shen Bang - Zhu Lu Zhi Zhan")},
	{nullptr},
	{_RP("Incorrect assignment (should be 115)")},
	{nullptr},

	// Mappers 250-255
	{_RP("Nitra (MMC3 clone)")},
	{nullptr},
	{_RP("WaiXing - Sangokushi")},
	{nullptr},
	{_RP("Pikachu Y2K of crypted ROMs")},
	{nullptr},
};

/** Submappers. **/

// MMC1
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc1_submappers[] = {
	{1, _RP("SUROM"), true},
	{2, _RP("SOROM"), true},
	{3, _RP("MMC1A"), true},
	{4, _RP("SXROM"), true},
	{5, _RP("SEROM, SHROM, SH1ROM"), false},
};

// Discrete logic mappers: UxROM, CNROM, AxROM
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::discrete_logic_submappers[] = {
	{1, _RP("Bus conflicts do not occur"), false},
	{2, _RP("Bus conflicts occur, resulting in: bus AND rom"), false},
};

// MMC3
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc3_submappers[] = {
	{0, _RP("MMC3C"), false},
	{1, _RP("MMC6"), false},
	{2, _RP("MMC3C with hard-wired mirroring"), true},
	{3, _RP("MC-ACC"), false},
};

// Irem G101
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::irem_g101_submappers[] = {
	// TODO: Some field to indicate mirroring override?
	{0, _RP("Programmable mirroring"), false},
	{1, _RP("Fixed one-screen mirroring"), false},
};

// BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::bnrom_nina001_submappers[] = {
	{1, _RP("NINA-001"), false},
	{2, _RP("BNROM"), false},
};

// Sunsoft-4
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::sunsoft4_submappers[] = {
	{1, _RP("Dual Cartridge System (NTB-ROM)"), false},
};

// Codemasters
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::codemasters_submappers[] = {
	{1, _RP("Programmable one-screen mirroring (Fire Hawk)"), false},
};

const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper078_submappers[] = {
	{1, _RP("Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)"), false},
	{2, _RP("Fixed vertical mirroring + WRAM"), true},
	{3, _RP("Programmable H/V mirroring (Holy Diver)"), false},
};

// Namcot 175, 340
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::namcot_175_340_submappers[] = {
	{1, _RP("Namcot 175 (fixed mirroring)"), false},
	{2, _RP("Namcot 340 (programmable mirroring)"), false},
};

// Codemasters Quattro
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::quattro_submappers[] = {
	{1, _RP("Aladdin Deck Enhancer"), false},
};

/**
 * NES 2.0 submapper list.
 *
 * NOTE: Submapper 0 is optional, since it's the default behavior.
 * It may be included if NES 2.0 submapper 0 acts differently from
 * iNES mappers.
 */
const NESMappersPrivate::SubmapperEntry NESMappersPrivate::submappers[] = {
	{  1, mmc1_submappers, ARRAY_SIZE(mmc1_submappers)},				// MMC1
	{  2, discrete_logic_submappers, ARRAY_SIZE(discrete_logic_submappers)},	// UxROM
	{  3, discrete_logic_submappers, ARRAY_SIZE(discrete_logic_submappers)},	// CNROM
	{  4, mmc3_submappers, ARRAY_SIZE(mmc3_submappers)},				// MMC3
	{  7, discrete_logic_submappers, ARRAY_SIZE(discrete_logic_submappers)},	// AxROM
	{ 32, irem_g101_submappers, ARRAY_SIZE(irem_g101_submappers)},			// Irem G101
	{ 34, bnrom_nina001_submappers, ARRAY_SIZE(bnrom_nina001_submappers)},		// BNROM / NINA-001
	{ 68, sunsoft4_submappers, ARRAY_SIZE(sunsoft4_submappers)},			// Sunsoft-4
	{ 71, codemasters_submappers, ARRAY_SIZE(codemasters_submappers)},		// Codemasters
	{ 78, mapper078_submappers, ARRAY_SIZE(mapper078_submappers)},
	{210, namcot_175_340_submappers, ARRAY_SIZE(namcot_175_340_submappers)},	// Namcot 175, 340
	{232, quattro_submappers, ARRAY_SIZE(quattro_submappers)},			// Codemasters Quattro
};

/** NESMappers **/

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper name, or nullptr if not found.
 */
const rp_char *NESMappers::lookup_ines(int mapper)
{
	static_assert(sizeof(NESMappersPrivate::mappers) == (256 * sizeof(NESMappersPrivate::MapperEntry)),
		"NESMappersPrivate::mappers[] doesn't have 256 entries.");
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

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NESMappers.cpp: NES mapper data.                                        *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NESMappers.hpp"

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
			const char *name;		// Name of the board. (If unknown, nullptr.)
			const char *manufacturer;	// Manufacturer. (If unknown, nullptr.)
		};
		static const MapperEntry mappers_plane0[];
		static const MapperEntry mappers_plane1[];
		static const MapperEntry mappers_plane2[];

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
			uint8_t submapper;	// Submapper number.
			uint8_t reserved;
			uint16_t deprecated;
			const char *desc;	// Description.
		};

		// Submappers.
		static const struct SubmapperInfo mmc1_submappers[];		// 001
		static const struct SubmapperInfo discrete_logic_submappers[];	// 002, 003, 007
		static const struct SubmapperInfo mmc3_submappers[];		// 004
		static const struct SubmapperInfo bandai_fcgx_submappers[];	// 016
		static const struct SubmapperInfo namco_129_164_submappers[];	// 019
		static const struct SubmapperInfo vrc4a_vrc4c_submappers[];	// 021
		static const struct SubmapperInfo vrc4ef_vrc2b_submappers[];	// 023
		static const struct SubmapperInfo vrc4bd_vrc2c_submappers[];	// 025
		static const struct SubmapperInfo irem_g101_submappers[];	// 032
		static const struct SubmapperInfo bnrom_nina001_submappers[];	// 034
		static const struct SubmapperInfo sunsoft4_submappers[];	// 068
		static const struct SubmapperInfo codemasters_submappers[];	// 071
		static const struct SubmapperInfo mapper078_submappers[];	// 078
		static const struct SubmapperInfo cony_yoko_submappers[];	// 083
		static const struct SubmapperInfo mapper114_submappers[];	// 114
		static const struct SubmapperInfo mapper197_submappers[];	// 197
		static const struct SubmapperInfo namcot_175_340_submappers[];	// 210
		static const struct SubmapperInfo sugar_softec_submappers[];	// 215
		static const struct SubmapperInfo quattro_submappers[];		// 232
		static const struct SubmapperInfo onebus_submappers[];		// 256
		static const struct SubmapperInfo smd132_smd133_submappers[];	// 268
		static const struct SubmapperInfo mapper313_submappers[];	// 313

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
 * Mappers: NES 2.0 Plane 0 [000-255] (iNES 1.0)
 * TODO: Add more fields:
 * - Programmable mirroring
 * - Extra VRAM for 4 screens
 */
const NESMappersPrivate::MapperEntry NESMappersPrivate::mappers_plane0[] = {
	/** NES 2.0 Plane 0 [0-255] (iNES 1.0) **/

	// Mappers 000-009
	{"NROM",			"Nintendo"},
	{"SxROM (MMC1)",		"Nintendo"},
	{"UxROM",			"Nintendo"},
	{"CNROM",			"Nintendo"},
	{"TxROM (MMC3), HKROM (MMC6)",	"Nintendo"},
	{"ExROM (MMC5)",		"Nintendo"},
	{"Game Doctor Mode 1",		"Bung/FFE"},
	{"AxROM",			"Nintendo"},
	{"Game Doctor Mode 4 (GxROM)",	"Bung/FFE"},
	{"PxROM, PEEOROM (MMC2)",	"Nintendo"},

	// Mappers 010-019
	{"FxROM (MMC4)",		"Nintendo"},
	{"Color Dreams",		"Color Dreams"},
	{"MMC3 variant",		"FFE"},
	{"NES-CPROM",			"Nintendo"},
	{"SL-1632 (MMC3/VRC2 clone)",	"Nintendo"},
	{"K-1029 (multicart)",		nullptr},
	{"FCG-x",			"Bandai"},
	{"FFE #17",			"FFE"},
	{"SS 88006",			"Jaleco"},
	{"Namco 129/163",		"Namco"},

	// Mappers 020-029
	{"Famicom Disk System",		"Nintendo"}, // this isn't actually used, as FDS roms are stored in their own format.
	{"VRC4a, VRC4c",		"Konami"},
	{"VRC2a",			"Konami"},
	{"VRC4e, VRC4f, VRC2b",		"Konami"},
	{"VRC6a",			"Konami"},
	{"VRC4b, VRC4d, VRC2c",		"Konami"},
	{"VRC6b",			"Konami"},
	{"VRC4 variant",		nullptr}, //investigate
	{"Action 53",			"Homebrew"},
	{"RET-CUFROM",			"Sealie Computing"},	// Homebrew

	// Mappers 030-039
	{"UNROM 512",			"RetroUSB"},	// Homebrew
	{"NSF Music Compilation",	"Homebrew"},
	{"Irem G-101",			"Irem"},
	{"Taito TC0190",		"Taito"},
	{"BNROM, NINA-001",		nullptr},
	{"J.Y. Company ASIC (8 KiB WRAM)", "J.Y. Company"},
	{"TXC PCB 01-22000-400",	"TXC"},
	{"MMC3 multicart",		"Nintendo"},
	{"GNROM variant",		"Bit Corp."},
	{"BNROM variant",		nullptr},

	// Mappers 040-049
	{"NTDEC 2722 (FDS conversion)",	"NTDEC"},
	{"Caltron 6-in-1",		"Caltron"},
	{"FDS conversion",		nullptr},
	{"TONY-I, YS-612 (FDS conversion)", nullptr},
	{"MMC3 multicart",		nullptr},
	{"MMC3 multicart (GA23C)",	nullptr},
	{"Rumble Station 15-in-1",	"Color Dreams"},	// NES-on-a-Chip
	{"MMC3 multicart",		"Nintendo"},
	{"Taito TC0690",		"Taito"},
	{"MMC3 multicart",		nullptr},

	// Mappers 050-059
	{"PCB 761214 (FDS conversion)",	"N-32"},
	{nullptr,			nullptr},
	{"MMC3 multicart",		nullptr},
	{nullptr,			nullptr},
	{"Novel Diamond 9999999-in-1",	nullptr},	// conflicting information
	{"BTL-MARIO1-MALEE2",		nullptr},	// From UNIF
	{"KS202 (unlicensed SMB3 reproduction)", nullptr},	// Some SMB3 unlicensed reproduction
	{"Multicart",			nullptr},
	{"(C)NROM-based multicart",	nullptr},
	{"BMC-T3H53/BMC-D1038 multicart", nullptr},	// From UNIF

	// Mappers 060-069
	{"Reset-based NROM-128 4-in-1 multicart", nullptr},
	{"20-in-1 multicart",		nullptr},
	{"Super 700-in-1 multicart",	nullptr},
	{"Powerful 250-in-1 multicart",	"NTDEC"},
	{"Tengen RAMBO-1",		"Tengen"},
	{"Irem H3001",			"Irem"},
	{"GxROM, MHROM",		"Nintendo"},
	{"Sunsoft-3",			"Sunsoft"},
	{"Sunsoft-4",			"Sunsoft"},
	{"Sunsoft FME-7",		"Sunsoft"},

	// Mappers 070-079
	{"Family Trainer",		"Bandai"},
	{"Codemasters (UNROM clone)",	"Codemasters"},
	{"Jaleco JF-17",		"Jaleco"},
	{"VRC3",			"Konami"},
	{"43-393/860908C (MMC3 clone)",	"Waixing"},
	{"VRC1",			"Konami"},
	{"NAMCOT-3446 (Namcot 108 variant)",	"Namco"},
	{"Napoleon Senki",		"Lenar"},
	{"Holy Diver; Uchuusen - Cosmo Carrier", nullptr},
	{"NINA-03, NINA-06",		"American Video Entertainment"},

	// Mappers 080-089
	{"Taito X1-005",		"Taito"},
	{"Super Gun",			"NTDEC"},
	{"Taito X1-017 (incorrect PRG ROM bank ordering)", "Taito"},
	{"Cony/Yoko",			"Cony/Yoko"},
	{"PC-SMB2J",			nullptr},
	{"VRC7",			"Konami"},
	{"Jaleco JF-13",		"Jaleco"},
	{"CNROM variant",		nullptr},
	{"Namcot 118 variant",		nullptr},
	{"Sunsoft-2 (Sunsoft-3 board)",	"Sunsoft"},

	// Mappers 090-099
	{"J.Y. Company (simple nametable control)", "J.Y. Company"},
	{"J.Y. Company (Super Fighter III)", "J.Y. Company"},
	{"Moero!! Pro",			"Jaleco"},
	{"Sunsoft-2 (Sunsoft-3R board)", "Sunsoft"},
	{"HVC-UN1ROM",			"Nintendo"},
	{"NAMCOT-3425",			"Namco"},
	{"Oeka Kids",			"Bandai"},
	{"Irem TAM-S1",			"Irem"},
	{nullptr,			nullptr},
	{"CNROM (Vs. System)",		"Nintendo"},

	// Mappers 100-109
	{"MMC3 variant (hacked ROMs)",	nullptr},	// Also used for UNIF
	{"Jaleco JF-10 (misdump)",	"Jaleceo"},
	{nullptr,			nullptr},
	{"Doki Doki Panic (FDS conversion)", nullptr},
	{"PEGASUS 5 IN 1",		nullptr},
	{"NES-EVENT (MMC1 variant) (Nintendo World Championships 1990)", "Nintendo"},
	{"Super Mario Bros. 3 (bootleg)", nullptr},
	{"Magic Dragon",		"Magicseries"},
	{nullptr,			nullptr},
	{nullptr,			nullptr},

	// Mappers 110-119
	{nullptr,			nullptr},
	{"Cheapocabra GTROM 512k flash board", "Membler Industries"},	// Homebrew
	{"Namcot 118 variant",		nullptr},
	{"NINA-03/06 multicart",	nullptr},
	{"MMC3 clone (scrambled registers)", nullptr},
	{"Kǎshèng SFC-02B/-03/-004 (MMC3 clone)", "Kǎshèng"},
	{"SOMARI-P (Huang-1/Huang-2)",	"Gouder"},
	{nullptr,			nullptr},
	{"TxSROM",			"Nintendo"},
	{"TQROM",			"Nintendo"},

	// Mappers 120-129
	{nullptr,			nullptr},
	{"Kǎshèng A9711 and A9713 (MMC3 clone)", "Kǎshèng"},
	{nullptr,			nullptr},
	{"Kǎshèng H2288 (MMC3 clone)",	"Kǎshèng"},
	{nullptr,			nullptr},
	{"Monty no Doki Doki Daisassō (FDS conversion)", "Whirlwind Manu"},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},

	// Mappers 130-139
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"TXC 05-00002-010 ASIC",	"TXC"},
	{"Jovial Race",			"Sachen"},
	{"T4A54A, WX-KB4K, BS-5652 (MMC3 clone)", nullptr},
	{nullptr,			nullptr},
	{"Sachen 3011",			"Sachen"},
	{"Sachen 8259D",		"Sachen"},
	{"Sachen 8259B",		"Sachen"},
	{"Sachen 8259C",		"Sachen"},

	// Mappers 140-149
	{"Jaleco JF-11, JF-14 (GNROM variant)", "Jaleco"},
	{"Sachen 8259A",		"Sachen"},
	{"Kaiser KS202 (FDS conversions)", "Kaiser"},
	{"Copy-protected NROM",		nullptr},
	{"Death Race (Color Dreams variant)", "American Game Cartridges"},
	{"Sidewinder (CNROM clone)",	"Sachen"},
	{"Galactic Crusader (NINA-06 clone)", nullptr},
	{"Sachen 3018",			"Sachen"},
	{"Sachen SA-008-A, Tengen 800008",	"Sachen / Tengen"},
	{"SA-0036 (CNROM clone)",	"Sachen"},

	// Mappers 150-159
	{"Sachen SA-015, SA-630",	"Sachen"},
	{"VRC1 (Vs. System)",		"Konami"},
	{"Kaiser KS202 (FDS conversion)", "Kaiser"},
	{"Bandai FCG: LZ93D50 with SRAM", "Bandai"},
	{"NAMCOT-3453",			"Namco"},
	{"MMC1A",			"Nintendo"},
	{"DIS23C01",			"Daou Infosys"},
	{"Datach Joint ROM System",	"Bandai"},
	{"Tengen 800037",		"Tengen"},
	{"Bandai LZ93D50 with 24C01",	"Bandai"},

	// Mappers 160-169
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"Nanjing",			"Nanjing"},
	{"Waixing (unlicensed)",	"Waixing"},
	{"Fire Emblem (unlicensed) (MMC2+MMC3 hybrid)", nullptr},
	{"Subor (variant 1)",		"Subor"},
	{"Subor (variant 2)",		"Subor"},
	{"Racermate Challenge 2",	"Racermate, Inc."},
	{"Yuxing",			"Yuxing"},

	// Mappers 170-179
	{nullptr,			nullptr},
	{"Kaiser KS-7058",		"Kaiser"},
	{"Super Mega P-4040",		nullptr},
	{"Idea-Tek ET-xx",		"Idea-Tek"},
	{"Multicart",			nullptr},
	{nullptr,			nullptr},
	{"Waixing multicart (MMC3 clone)", "Waixing"},
	{"BNROM variant",		"Hénggé Diànzǐ"},
	{"Waixing / Nanjing / Jncota / Henge Dianzi / GameStar", "Waixing / Nanjing / Jncota / Henge Dianzi / GameStar"},
	{nullptr,			nullptr},

	// Mappers 180-189
	{"Crazy Climber (UNROM clone)",	"Nichibutsu"},
	{"Seicross v2 (FCEUX hack)",	"Nichibutsu"},
	{"MMC3 clone (scrambled registers) (same as 114)", nullptr},
	{"Suikan Pipe (VRC4e clone)",	nullptr},
	{"Sunsoft-1",			"Sunsoft"},
	{"CNROM with weak copy protection", nullptr},	// Submapper field indicates required value for CHR banking.
	{"Study Box",			"Fukutake Shoten"},
	{"Kǎshèng A98402 (MMC3 clone)",	"Kǎshèng"},
	{"Bandai Karaoke Studio",	"Bandai"},
	{"Thunder Warrior (MMC3 clone)", nullptr},

	// Mappers 190-199
	{"Magic Kid GooGoo",		nullptr},
	{"MMC3 clone",			nullptr},
	{"MMC3 clone",			nullptr},
	{"NTDEC TC-112",		"NTDEC"},
	{"MMC3 clone",			nullptr},
	{"Waixing FS303 (MMC3 clone)",	"Waixing"},
	{"Mario bootleg (MMC3 clone)",	nullptr},
	{"Kǎshèng (MMC3 clone)",	"Kǎshèng"},
	{"Tūnshí Tiāndì - Sānguó Wàizhuàn", nullptr},
	{"Waixing (clone of either Mapper 004 or 176)", "Waixing"},

	// Mappers 200-209
	{"Multicart",			nullptr},
	{"NROM-256 multicart",		nullptr},
	{"150-in-1 multicart",		nullptr},
	{"35-in-1 multicart",		nullptr},
	{nullptr,			nullptr},
	{"MMC3 multicart",		nullptr},
	{"DxROM (Tengen MIMIC-1, Namcot 118)", "Nintendo"},
	{"Fudou Myouou Den",		"Taito"},
	{"Street Fighter IV (unlicensed) (MMC3 clone)", nullptr},
	{"J.Y. Company (MMC2/MMC4 clone)", "J.Y. Company"},

	// Mappers 210-219
	{"Namcot 175, 340",		"Namco"},
	{"J.Y. Company (extended nametable control)", "J.Y. Company"},
	{"BMC Super HiK 300-in-1",	nullptr},
	{"(C)NROM-based multicart (same as 058)", nullptr},
	{nullptr,			nullptr},
	{"Sugar Softec (MMC3 clone)",	"Sugar Softec"},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"Magic Floor",			"Homebrew"},
	{"Kǎshèng A9461 (MMC3 clone)",	"Kǎshèng"},

	// Mappers 220-229
	{"Summer Carnival '92 - Recca",	"Naxat Soft"},
	{"NTDEC N625092",		"NTDEC"},
	{"CTC-31 (VRC2 + 74xx)",	nullptr},
	{nullptr,			nullptr},
	{"Jncota KT-008",		"Jncota"},
	{"Multicart",			nullptr},
	{"Multicart",			nullptr},
	{"Multicart",			nullptr},
	{"Active Enterprises",		"Active Enterprises"},
	{"BMC 31-IN-1",			nullptr},

	// Mappers 230-239
	{"Multicart",			nullptr},
	{"Multicart",			nullptr},
	{"Codemasters Quattro",		"Codemasters"},
	{"Multicart",			nullptr},
	{"Maxi 15 multicart",		nullptr},
	{"Golden Game 150-in-1 multicart", nullptr},
	{"Realtec 8155",		"Realtec"},
	{"Teletubbies 420-in-1 multicart", nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},

	// Mappers 240-249
	{"Multicart",			nullptr},
	{"BNROM variant (similar to 034)", nullptr},
	{"Unlicensed",			nullptr},
	{"Sachen SA-020A",		"Sachen"},
	{nullptr,			nullptr},
	{"MMC3 clone",			nullptr},
	{"Fēngshénbǎng: Fúmó Sān Tàizǐ (C&E)", "C&E"},
	{nullptr,			nullptr},
	{"Kǎshèng SFC-02B/-03/-004 (MMC3 clone) (incorrect assignment; should be 115)", "Kǎshèng"},
	{nullptr,			nullptr},

	// Mappers 250-255
	{"Nitra (MMC3 clone)",		"Nitra"},
	{nullptr,			nullptr},
	{"Waixing - Sangokushi",	"Waixing"},
	{"Dragon Ball Z: Kyōshū! Saiya-jin (VRC4 clone)", "Waixing"},
	{"Pikachu Y2K of crypted ROMs",	nullptr},
	{"110-in-1 multicart (same as 225)",		nullptr},
};

/**
 * Mappers: NES 2.0 Plane 1 [256-511]
 * TODO: Add more fields:
 * - Programmable mirroring
 * - Extra VRAM for 4 screens
 */
const NESMappersPrivate::MapperEntry NESMappersPrivate::mappers_plane1[] = {
	// Mappers 256-259
	{"OneBus Famiclone",		nullptr},
	{"UNIF PEC-586",		nullptr},	// From UNIF; reserved by FCEUX developers
	{"UNIF 158B",			nullptr},	// From UNIF; reserved by FCEUX developers
	{"UNIF F-15 (MMC3 multicart)",	nullptr},	// From UNIF; reserved by FCEUX developers

	// Mappers 260-269
	{"HP10xx/HP20xx multicart",	nullptr},
	{"200-in-1 Elfland multicart",	nullptr},
	{"Street Heroes (MMC3 clone)",	"Sachen"},
	{"King of Fighters '97 (MMC3 clone)", nullptr},
	{"Cony/Yoko Fighting Games",	"Cony/Yoko"},
	{"T-262 multicart",		nullptr},
	{"City Fighter IV",		nullptr},	// Hack of Master Fighter II
	{"8-in-1 JY-119 multicart (MMC3 clone)", "J.Y. Company"},
	{"SMD132/SMD133 (MMC3 clone)",	nullptr},
	{"Multicart (MMC3 clone)",	nullptr},

	// Mappers 270-279
	{"Game Prince RS-16",		nullptr},
	{"TXC 4-in-1 multicart (MGC-026)", "TXC"},
	{"Akumajō Special: Boku Dracula-kun (bootleg)", nullptr},
	{"Gremlins 2 (bootleg)",	nullptr},
	{"Cartridge Story multicart",	"RCM Group"},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},

	// Mappers 280-289
	{nullptr,			nullptr},
	{"J.Y. Company Super HiK 3/4/5-in-1 multicart", "J.Y. Company"},
	{"J.Y. Company multicart",	"J.Y. Company"},
	{"Block Family 6-in-1/7-in-1 multicart", nullptr},
	{"Drip",			"Homebrew"},
	{"A65AS multicart",		nullptr},
	{"Benshieng multicart",		"Benshieng"},
	{"4-in-1 multicart (411120-C, 811120-C)", nullptr},
	{"GKCX1 21-in-1 multicart",	nullptr},	// GoodNES 3.23b sets this to Mapper 133, which is wrong.
	{"BMC-60311C",			nullptr},	// From UNIF

	// Mappers 290-299
	{"Asder 20-in-1 multicart",	"Asder"},
	{"Kǎshèng 2-in-1 multicart (MK6)", "Kǎshèng"},
	{"Dragon Fighter (unlicensed)",	nullptr},
	{"NewStar 12-in-1/76-in-1 multicart", nullptr},
	{"T4A54A, WX-KB4K, BS-5652 (MMC3 clone) (same as 134)", nullptr},
	{"J.Y. Company 13-in-1 multicart", "J.Y. Company"},
	{"FC Pocket RS-20 / dreamGEAR My Arcade Gamer V", nullptr},
	{"TXC 01-22110-000 multicart",	"TXC"},
	{"Lethal Weapon (unlicensed) (VRC4 clone)", nullptr},
	{"TXC 6-in-1 multicart (MGC-023)", "TXC"},

	// Mappers 300-309
	{"Golden 190-in-1 multicart",	nullptr},
	{"GG1 multicart",		nullptr},
	{"Gyruss (FDS conversion)",	"Kaiser"},
	{"Almana no Kiseki (FDS conversion)", "Kaiser"},
	{"FDS conversion",		"Whirlwind Manu"},
	{"Dracula II: Noroi no Fūin (FDS conversion)", "Kaiser"},
	{"Exciting Basket (FDS conversion)", "Kaiser"},
	{"Metroid (FDS conversion)",	"Kaiser"},
	{"Batman (Sunsoft) (bootleg) (VRC2 clone)", nullptr},
	{"Ai Senshi Nicol (FDS conversion)", "Whirlwind Manu"},

	// Mappers 310-319
	{"Monty no Doki Doki Daisassō (FDS conversion) (same as 125)", "Whirlwind Manu"},
	{nullptr,			nullptr},
	{"Highway Star (bootleg)",	"Kaiser"},
	{"Reset-based multicart (MMC3)", nullptr},
	{"Y2K multicart",		nullptr},
	{"820732C- or 830134C- multicart", nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"HP-898F, KD-7/9-E multicart",	nullptr},

	// Mappers 320-329
	{"Super HiK 6-in-1 A-030 multicart", nullptr},
	{nullptr,			nullptr},
	{"35-in-1 (K-3033) multicart",	nullptr},
	{"Farid's homebrew 8-in-1 SLROM multicart", nullptr},	// Homebrew
	{"Farid's homebrew 8-in-1 UNROM multicart", nullptr},	// Homebrew
	{"Super Mali Splash Bomb (bootleg)", nullptr},
	{"Contra/Gryzor (bootleg)",	nullptr},
	{"6-in-1 multicart",		nullptr},
	{"Test Ver. 1.01 Dlya Proverki TV Pristavok test cartridge", nullptr},
	{"Education Computer 2000",	nullptr},

	// Mappers 330-339
	{"Sangokushi II: Haō no Tairiku (bootleg)", nullptr},
	{"7-in-1 (NS03) multicart",	nullptr},
	{"Super 40-in-1 multicart",	nullptr},
	{"New Star Super 8-in-1 multicart", "New Star"},
	{"5/20-in-1 1993 Copyright multicart", nullptr},
	{"10-in-1 multicart",		nullptr},
	{"11-in-1 multicart",		nullptr},
	{"12-in-1 Game Card multicart",	nullptr},
	{"16-in-1, 200/300/600/1000-in-1 multicart", nullptr},
	{"21-in-1 multicart",		nullptr},

	// Mappers 340-349
	{"35-in-1 multicart",		nullptr},
	{"Simple 4-in-1 multicart",	nullptr},
	{"COOLGIRL multicart (Homebrew)", "Homebrew"},	// Homebrew
	{nullptr,			nullptr},
	{"Kuai Da Jin Ka Zhong Ji Tiao Zhan 3-in-1 multicart", nullptr},
	{"New Star 6-in-1 Game Cartridge multicart", "New Star"},
	{"Zanac (FDS conversion)",	"Kaiser"},
	{"Yume Koujou: Doki Doki Panic (FDS conversion)", "Kaiser"},
	{"830118C",			nullptr},
	{"1994 Super HIK 14-in-1 (G-136) multicart", nullptr},

	// Mappers 350-359
	{"Super 15-in-1 Game Card multicart", nullptr},
	{"9-in-1 multicart",		"J.Y. Company / Techline"},
	{nullptr,			nullptr},
	{"92 Super Mario Family multicart", nullptr},
	{"250-in-1 multicart",		nullptr},
	{"黃信維 3D-BLOCK",		nullptr},
	{"7-in-1 Rockman (JY-208)",	"J.Y. Company"},
	{"4-in-1 (4602) multicart",	"Bit Corp."},
	{"J.Y. Company multicart",	"J.Y. Company"},
	{"SB-5013 / GCL8050 / 841242C multicart", nullptr},

	// Mappers 360-369
	{"31-in-1 (3150) multicart",	"Bit Corp."},
	{"YY841101C multicart (MMC3 clone)", "J.Y. Company"},
	{"830506C multicart (VRC4f clone)", "J.Y. Company"},
	{"J.Y. Company multicart",	"J.Y. Company"},
	{"JY830832C multicart",		"J.Y. Company"},
	{"Asder PC-95 educational computer", "Asder"},
	{"GN-45 multicart (MMC3 clone)", nullptr},
	{"7-in-1 multicart",		nullptr},
	{"Super Mario Bros. 2 (J) (FDS conversion)", "YUNG-08"},
	{"N49C-300",			nullptr},

	// Mappers 370-379
	{"F600",			nullptr},
	{"Spanish PEC-586 home computer cartridge", "Dongda"},
	{"Rockman 1-6 (SFC-12) multicart", nullptr},
	{"Super 4-in-1 (SFC-13) multicart", nullptr},
	{"Reset-based MMC1 multicart",	nullptr},
	{"135-in-1 (U)NROM multicart",	nullptr},
	{"YY841155C multicart",		"J.Y. Company"},
	{"8-in-1 AxROM/UNROM multicart", nullptr},
	{"35-in-1 NROM multicart",	nullptr},

	// Mappers 380-389
	{"970630C",			nullptr},
	{"KN-42",			nullptr},
	{"830928C",			nullptr},
	{"YY840708C (MMC3 clone)",	"J.Y. Company"},
	{"L1A16 (VRC4e clone)",		nullptr},
	{"NTDEC 2779",			"NTDEC"},
	{"YY860729C",			"J.Y. Company"},
	{"YY850735C / YY850817C",	"J.Y. Company"},
	{"YY841145C / YY850835C",	"J.Y. Company"},
	{"Caltron 9-in-1 multicart",	"Caltron"},

	// Mappers 390-391
	{"Realtec 8031",		"Realtec"},
	{"NC7000M (MMC3 clone)",	nullptr},
};

/**
 * Mappers: NES 2.0 Plane 2 [512-767]
 * TODO: Add more fields:
 * - Programmable mirroring
 * - Extra VRAM for 4 screens
 */
const NESMappersPrivate::MapperEntry NESMappersPrivate::mappers_plane2[] = {
	// Mappers 512-519
	{"Zhōngguó Dàhēng",		"Sachen"},
	{"Měi Shàonǚ Mèng Gōngchǎng III", "Sachen"},
	{"Subor Karaoke",		"Subor"},
	{"Family Noraebang",		nullptr},
	{"Brilliant Com Cocoma Pack",	"EduBank"},
	{"Kkachi-wa Nolae Chingu",	nullptr},
	{"Subor multicart",		"Subor"},
	{"UNL-EH8813A",			nullptr},

	// Mappers 520-529
	{"2-in-1 Datach multicart (VRC4e clone)", nullptr},
	{"Korean Igo",			nullptr},
	{"Fūun Shōrinken (FDS conversion)", "Whirlwind Manu"},
	{"Fēngshénbǎng: Fúmó Sān Tàizǐ (Jncota)", "Jncota"},
	{"The Lord of King (Jaleco) (bootleg)", nullptr},
	{"UNL-KS7021A (VRC2b clone)",	"Kaiser"},
	{"Sangokushi: Chūgen no Hasha (bootleg)", nullptr},
	{"Fudō Myōō Den (bootleg) (VRC2b clone)", nullptr},
	{"1995 New Series Super 2-in-1 multicart", nullptr},
	{"Datach Dragon Ball Z (bootleg) (VRC4e clone)", nullptr},

	// Mappers 530-539
	{"Super Mario Bros. Pocker Mali (VRC4f clone)", nullptr},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"Sachen 3014",			"Sachen"},
	{"2-in-1 Sudoku/Gomoku (NJ064) (MMC3 clone)", nullptr},
	{"Nazo no Murasamejō (FDS conversion)", "Whirlwind Manu"},
	{"Waixing FS303 (MMC3 clone) (same as 195)",	"Waixing"},
	{"Waixing FS303 (MMC3 clone) (same as 195)",	"Waixing"},
	{"60-1064-16L",			nullptr},
	{"Kid Icarus (FDS conversion)",	nullptr},

	// Mappers 540-549
	{"Master Fighter VI' hack (variant of 359)", nullptr},
	{"LittleCom 160-in-1 multicart", nullptr},	// Is LittleCom the company name?
	{"World Hero hack (VRC4 clone)", nullptr},
	{"5-in-1 (CH-501) multicart (MMC1 clone)", nullptr},
	{"Waixing FS306",		"Waixing"},
	{nullptr,			nullptr},
	{nullptr,			nullptr},
	{"Konami QTa adapter (VRC5)",	"Konami"},
	{"CTC-15",			"Co Tung Co."},
	{nullptr,			nullptr},

	// Mappers 550-552
	{nullptr,			nullptr},
	{"Jncota RPG re-release (variant of 178)", "Jncota"},
	{"Taito X1-017 (correct PRG ROM bank ordering)", "Taito"},
};

/** Submappers. **/

// Mapper 001: MMC1
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc1_submappers[] = {
	{1, 0,   1, "SUROM"},
	{2, 0,   1, "SOROM"},
	{3, 0, 155, "MMC1A"},
	{4, 0,   1, "SXROM"},
	{5, 0,   0, "SEROM, SHROM, SH1ROM"},
};

// Discrete logic mappers: UxROM (002), CNROM (003), AxROM (007)
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::discrete_logic_submappers[] = {
	{0, 0,   0, "Bus conflicts are unspecified"},
	{1, 0,   0, "Bus conflicts do not occur"},
	{2, 0,   0, "Bus conflicts occur, resulting in: bus AND rom"},
};

// Mapper 004: MMC3
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mmc3_submappers[] = {
	{0, 0,      0, "MMC3C"},
	{1, 0,      0, "MMC6"},
	{2, 0, 0xFFFF, "MMC3C with hard-wired mirroring"},
	{3, 0,      0, "MC-ACC"},
	{4, 0,      0, "MMC3A"},
};

// Mapper 016: Bandai FCG-x
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::bandai_fcgx_submappers[] = {
	{1, 0, 159, "LZ93D50 with 24C01"},
	{2, 0, 157, "Datach Joint ROM System"},
	{3, 0, 153, "8 KiB of WRAM instead of serial EEPROM"},
	{4, 0,   0, "FCG-1/2"},
	{5, 0,   0, "LZ93D50 with optional 24C02"},
};

// Mapper 019: Namco 129, 163
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::namco_129_164_submappers[] = {
	{0, 0,   0, "Expansion sound volume unspecified"},
	{1, 0,  19, "Internal RAM battery-backed; no expansion sound"},
	{2, 0,   0, "No expansion sound"},
	{3, 0,   0, "N163 expansion sound: 11.0-13.0 dB louder than NES APU"},
	{4, 0,   0, "N163 expansion sound: 16.0-17.0 dB louder than NES APU"},
	{5, 0,   0, "N163 expansion sound: 18.0-19.5 dB louder than NES APU"},
};

// Mapper 021: Konami VRC4c, VRC4c
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::vrc4a_vrc4c_submappers[] = {
	{1, 0,   0, "VRC4a"},
	{2, 0,   0, "VRC4c"},
};

// Mapper 023: Konami VRC4e, VRC4f, VRC2b
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::vrc4ef_vrc2b_submappers[] = {
	{1, 0,   0, "VRC4f"},
	{2, 0,   0, "VRC4e"},
	{3, 0,   0, "VRC2b"},
};

// Mapper 025: Konami VRC4b, VRC4d, VRC2c
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::vrc4bd_vrc2c_submappers[] = {
	{1, 0,   0, "VRC4b"},
	{2, 0,   0, "VRC4d"},
	{3, 0,   0, "VRC2c"},
};

// Mapper 032: Irem G101
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::irem_g101_submappers[] = {
	// TODO: Some field to indicate mirroring override?
	{0, 0,   0, "Programmable mirroring"},
	{1, 0,   0, "Fixed one-screen mirroring"},
};

// Mapper 034: BNROM / NINA-001
// TODO: Distinguish between these two for iNES ROMs.
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::bnrom_nina001_submappers[] = {
	{1, 0,   0, "NINA-001"},
	{2, 0,   0, "BNROM"},
};

// Mapper 068: Sunsoft-4
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::sunsoft4_submappers[] = {
	{1, 0,   0, "Dual Cartridge System (NTB-ROM)"},
};

// Mapper 071: Codemasters
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::codemasters_submappers[] = {
	{1, 0,   0, "Programmable one-screen mirroring (Fire Hawk)"},
};

// Mapper 078: Cosmo Carrier / Holy Diver
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper078_submappers[] = {
	{1, 0,      0, "Programmable one-screen mirroring (Uchuusen: Cosmo Carrier)"},
	{2, 0, 0xFFFF,  "Fixed vertical mirroring + WRAM"},
	{3, 0,      0, "Programmable H/V mirroring (Holy Diver)"},
};

// Mapper 083: Cony/Yoko
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::cony_yoko_submappers[] = {
	{0, 0,   0, "1 KiB CHR-ROM banking, no WRAM"},
	{1, 0,   0, "2 KiB CHR-ROM banking, no WRAM"},
	{2, 0,   0, "1 KiB CHR-ROM banking, 32 KiB banked WRAM"},
};

// Mapper 114: Sugar Softec/Hosenkan
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper114_submappers[] = {
	{0, 0,   0, "MMC3 registers: 0,3,1,5,6,7,2,4"},
	{1, 0,   0, "MMC3 registers: 0,2,5,3,6,1,7,4"},
};

// Mapper 197: Kǎshèng (MMC3 clone)
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper197_submappers[] = {
	{0, 0,   0, "Super Fighter III (PRG-ROM CRC32 0xC333F621)"},
	{1, 0,   0, "Super Fighter III (PRG-ROM CRC32 0x2091BEB2)"},
	{2, 0,   0, "Mortal Kombat III Special"},
	{3, 0,   0, "1995 Super 2-in-1"},
};

// Mapper 210: Namcot 175, 340
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::namcot_175_340_submappers[] = {
	{1, 0,   0, "Namcot 175 (fixed mirroring)"},
	{2, 0,   0, "Namcot 340 (programmable mirroring)"},
};

// Mapper 215: Sugar Softec
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::sugar_softec_submappers[] = {
	{0, 0,   0, "UNL-8237"},
	{1, 0,   0, "UNL-8237A"},
};

// Mapper 232: Codemasters Quattro
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::quattro_submappers[] = {
	{1, 0,   0, "Aladdin Deck Enhancer"},
};

// Mapper 256: OneBus Famiclones
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::onebus_submappers[] = {
	{ 1, 0,   0, "Waixing VT03"},
	{ 2, 0,   0, "Power Joy Supermax"},
	{ 3, 0,   0, "Zechess/Hummer Team"},
	{ 4, 0,   0, "Sports Game 69-in-1"},
	{ 5, 0,   0, "Waixing VT02"},
	{14, 0,   0, "Karaoto"},
	{15, 0,   0, "Jungletac"},
};

// Mapper 268: SMD132/SMD133
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::smd132_smd133_submappers[] = {
	{0, 0,   0, "COOLBOY ($6000-$7FFF)"},
	{1, 0,   0, "MINDKIDS ($5000-$5FFF)"},
};

// Mapper 313: Reset-based multicart (MMC3)
const struct NESMappersPrivate::SubmapperInfo NESMappersPrivate::mapper313_submappers[] = {
	{0, 0,   0, "Game size: 128 KiB PRG, 128 KiB CHR"},
	{1, 0,   0, "Game size: 256 KiB PRG, 128 KiB CHR"},
	{2, 0,   0, "Game size: 128 KiB PRG, 256 KiB CHR"},
	{3, 0,   0, "Game size: 256 KiB PRG, 256 KiB CHR"},
	{4, 0,   0, "Game size: 256 KiB PRG (first game); 128 KiB PRG (other games); 128 KiB CHR"},
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
	assert(mapper >= 0);
	if (mapper < 0) {
		// Mapper number is out of range.
		return nullptr;
	}

	if (mapper < 256) {
		// NES 2.0 Plane 0 [000-255] (iNES 1.0)
		static_assert(sizeof(NESMappersPrivate::mappers_plane0) == (256 * sizeof(NESMappersPrivate::MapperEntry)),
			"NESMappersPrivate::mappers_plane0[] doesn't have 256 entries.");
		return NESMappersPrivate::mappers_plane0[mapper].name;
	} else if (mapper < 512) {
		// NES 2.0 Plane 1 [256-511]
		mapper -= 256;
		if (mapper >= ARRAY_SIZE(NESMappersPrivate::mappers_plane1)) {
			// Mapper number is out of range for plane 1.
			return nullptr;
		}
		return NESMappersPrivate::mappers_plane1[mapper].name;
	} else if (mapper < 768) {
		// NES 2.0 Plane 2 [512-767]
		mapper -= 512;
		if (mapper >= ARRAY_SIZE(NESMappersPrivate::mappers_plane2)) {
			// Mapper number is out of range for plane 2.
			return nullptr;
		}
		return NESMappersPrivate::mappers_plane2[mapper].name;
	}

	// Invalid mapper number.
	return nullptr;
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
	assert(submapper >= 0);
	assert(submapper < 256);
	if (mapper < 0 || submapper < 0 || submapper >= 256) {
		// Mapper or submapper number is out of range.
		return nullptr;
	}

	// Do a binary search in submappers[].
	const NESMappersPrivate::SubmapperEntry key = { static_cast<uint16_t>(mapper), 0, nullptr };
	const NESMappersPrivate::SubmapperEntry *res =
		static_cast<const NESMappersPrivate::SubmapperEntry*>(bsearch(&key,
			NESMappersPrivate::submappers,
			ARRAY_SIZE(NESMappersPrivate::submappers)-1,
			sizeof(NESMappersPrivate::SubmapperEntry),
			NESMappersPrivate::SubmapperEntry_compar));
	if (!res || !res->info || res->info_size == 0)
		return nullptr;

	// Do a binary search in res->info.
	const NESMappersPrivate::SubmapperInfo key2 = { static_cast<uint8_t>(submapper), 0, 0, nullptr };
	const NESMappersPrivate::SubmapperInfo *res2 =
		static_cast<const NESMappersPrivate::SubmapperInfo*>(bsearch(&key2,
			res->info, res->info_size,
			sizeof(NESMappersPrivate::SubmapperInfo),
			NESMappersPrivate::SubmapperInfo_compar));
	// TODO: Return the "deprecated" value?
	return (res2 ? res2->desc : nullptr);
}

}

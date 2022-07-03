/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CBMData.hpp: Commodore cartridge data.                                  *
 *                                                                         *
 * Copyright (c) 2022 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CBMData.hpp"

namespace LibRomData { namespace CBMData {

/** Cartridge types are synchronized with VICE 3.6. **/

// C64 cartridge types
static const char *const crt_types_c64[] = {
	// 0
	"generic cartridge", "Action Replay", "KCS Power Cartridge",
	"Final Cartridge III", "Simons' BASIC", "Ocean type 1",
	"Expert Cartridge", "Fun Play, Power Play", "Super Games",
	"Atomic Power",

	// 10
	"Epyx Fastload", "Westermann Learning", "Rex Utility",
	"Final Cartridge I", "Magic Formel", "C64 Game System, System 3",
	"Warp Speed", "Dinamic", "Zaxxon / Super Zaxxon (Sega)",
	"Magic Desk, Domark, HES Australia",

	// 20
	"Super Snapshot V5", "Comal-80", "Structured BASIC",
	"Ross", "Dela EP64", "Dela EP7x8", "Dela EP256",
	"Rex EP256", "Mikro Assembler", "Final Cartridge Plus",

	// 30
	"Action Replay 4", "Stardos", "EasyFlash", "EasyFlash Xbank",
	"Capture", "Action Replay 3", "Retro Replay",
	"MMC64", "MMC Replay", "IDE64",

	// 40
	"Super Snapshot V4", "IEEE-488", "Game Killer", "Prophet64",
	"EXOS", "Freeze Frame", "Freeze Machine", "Snapshot64",
	"Super Explode V5.0", "Magic Voice",

	// 50
	"Action Replay 2", "MACH 5", "Diashow-Maker", "Pagefox",
	"Kingsoft", "Silverrock 128K Cartridge", "Formel 64",
	"RGCD", "RR-Net MK3", "EasyCalc",

	// 60
	"GMod2", "MAX Basic", "GMod3", "ZIPP-CODE 48",
	"Blackbox V8", "Blackbox V3", "Blackbox V4",
	"REX RAM-Floppy", "BIS-Plus", "SD-BOX",

	// 70
	"MultiMAX", "Blackbox V9", "Lt. Kernal Host Adaptor",
	"RAMLink", "H.E.R.O.", "IEEE Flash! 64",
	"Turtle Graphics II", "Freeze Frame MK2",
};

// VIC-20 cartridge types
static const char *const crt_types_vic20[] = {
	"generic cartridge",
	"Mega-Cart",
	"Behr Bonz",
	"Vic Flash Plugin",
	"UltiMem",
	"Final Expansion",
};

/** Public functions **/

/**
 * Look up a C64 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_C64_cart_type(uint16_t type)
{
	if (type > ARRAY_SIZE(crt_types_c64)) {
		return nullptr;
	}
	return crt_types_c64[type];
}

/**
 * Look up a VIC-20 cartridge type.
 * @param type Cartridge type
 * @return Cartridge type name, or nullptr if not found.
 */
const char *lookup_VIC20_cart_type(uint16_t type)
{
	if (type > ARRAY_SIZE(crt_types_vic20)) {
		return nullptr;
	}
	return crt_types_vic20[type];
}

} }

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
 * iNES mapper list.
 */
const NESMappers::MapperList NESMappers::ms_mapperList[] = {
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
	{_RP("MMC multicart (official)")},
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
};

/**
 * Look up an iNES mapper number.
 * @param mapper Mapper number.
 * @return Mapper name, or nullptr if not found.
 */
const rp_char *NESMappers::lookup_ines(int mapper)
{
	if (mapper < 0 || mapper >= ARRAY_SIZE(ms_mapperList)) {
		// Mapper number is out of range.
		return nullptr;
	}

	return ms_mapperList[mapper].name;
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

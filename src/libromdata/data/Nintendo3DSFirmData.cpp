/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.cpp: Nintendo 3DS firmware data.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Nintendo3DSFirmData.hpp"

namespace LibRomData { namespace Nintendo3DSFirmData {

/**
 * Firmware binary version information.
 * NOTE: Sorted by CRC32 for bsearch().
 */
static const FirmBin_t firmBins[] = {
	{0x0FD41774, {2,27, 0}, { 1,0}, false},
	{0x104F1A22, {2,50, 9}, {10,2}, true},
	{0x11A9A4BA, {2,36, 0}, { 5,1}, false},
	{0x13A10539, {2,39, 0}, { 7,0}, false},
	{0x2B0726F1, {2,49, 0}, { 9,5}, false},
	{0x32E9236F, {2,35, 6}, { 5,0}, false},
	{0x415BEAFE, {2,52, 0}, {11,2}, true},
	{0x41C8A171, {2,52, 0}, {11,2}, false},
	{0x4380DB8D, {2,46, 0}, { 9,0}, false},
	{0x4A07016A, {2,54, 0}, {11,4}, true},
	{0x4EE22A07, {2,55, 0}, {11,8}, false},
	{0x528E293F, {2,37, 0}, { 6,0}, false},
	{0x584C9AF5, {2,48, 3}, { 9,3}, false},
	{0x6488499E, {2,33, 4}, { 4,0}, false},
	{0x6E4ED781, {2,50,11}, {10,4}, false},
	{0x70A08ACD, {2,28, 0}, { 1,1}, false},
	{0x7421ACB4, {2,53, 0}, {11,3}, false},
	{0x80D26BB6, {2,30,18}, { 2,1}, false},
	{0x8662D9E4, {2,50, 7}, {10,0}, true},
	{0x8904168D, {2,46, 0}, { 9,0}, true},
	{0x90B92754, {2,38, 0}, { 6,1}, false},
	{0x925C092E, {2,45, 5}, { 8,1}, true},
	{0x93D29ADA, {2,50, 7}, {10,0}, false},
	{0x9622D367, {2,44, 6}, { 8,0}, false},
	{0x985699BF, {2,51, 2}, {11,1}, false},
	{0x98640F5C, {2,34, 0}, { 4,1}, false},
	{0xA89A6392, {2,51, 0}, {11,0}, true},
	{0xA8E660DF, {2,49, 0}, { 9,5}, true},
	{0xAB6D5279, {2,51, 0}, {11,0}, false},
	{0xACCC5EC4, {2,50,11}, {10,4}, true},
	{0xB7B6499E, {2,50, 1}, { 9,6}, true},
	{0xBDD9D878, {2,50, 9}, {10,2}, false},
	{0xC110E2F9, {2,56, 0}, {11,12}, true},
	{0xC5380DCC, {2,53, 0}, {11,3}, true},
	{0xC645B9A5, {2,50, 1}, { 9,6}, false},
	{0xC9829406, {2,29, 7}, { 2,0}, false},
	{0xDA0F7831, {2,54, 0}, {11,4}, false},
	{0xE0D74F64, {2,32,15}, { 3,0}, false},
	{0xE25F25F5, {2,31,40}, { 2,2}, false},
	{0xEA07F21E, {2,40, 0}, { 7,2}, false},
	{0xEE23547A, {2,55, 0}, {11,8}, true},
	{0xF5D833A2, {2,51, 2}, {11,1}, true},
	{0xFA7997F7, {2,56, 0}, {11,12}, false},
	{0xFFA6777A, {2,48, 3}, { 9,3}, true},
};

/** Public functions **/

/**
 * Look up a Nintendo 3DS firmware binary.
 * @param crc Firmware binary CRC32.
 * @return Firmware binary data, or nullptr if not found.
 */
const FirmBin_t *lookup_firmBin(const uint32_t crc)
{
	// Do a binary search.
	static const FirmBin_t *const pFirmBins_end =
		&firmBins[ARRAY_SIZE(firmBins)];
	auto pFirmBin = std::lower_bound(firmBins, pFirmBins_end, crc,
		[](const FirmBin_t &firmBin, uint32_t crc) noexcept -> bool {
			return (firmBin.crc < crc);
		});
	if (pFirmBin == pFirmBins_end || pFirmBin->crc != crc) {
		return nullptr;
	}
	return pFirmBin;
}

} }

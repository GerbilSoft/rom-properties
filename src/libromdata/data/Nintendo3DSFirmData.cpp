/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.cpp: Nintendo 3DS firmware data.                    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "Nintendo3DSFirmData.hpp"

// C++ STL classes
#include <algorithm>
#include <array>
using std::array;

namespace LibRomData { namespace Nintendo3DSFirmData {

/**
 * Firmware binary version information.
 * NOTE: Sorted by CRC32 for bsearch().
 * TODO: Add TWL_FIRM and AGB_FIRM.
 */
static const array<FirmBin_t, 49> firmBins = {{
	{0x0FD41774, {2,27, 0}, { 1, 0}, 0},
	{0x104F1A22, {2,50, 9}, {10, 2}, FLAG_New3DS},
	{0x11A9A4BA, {2,36, 0}, { 5, 1}, 0},
	{0x13A10539, {2,39, 0}, { 7, 0}, 0},
	{0x14AF04A9, {2,58, 0}, {11,16}, FLAG_New3DS},
	{0x2180C8A7, {2,57, 0}, {11,14}, 0},
	{0x2B0726F1, {2,49, 0}, { 9, 5}, 0},
	{0x32E9236F, {2,35, 6}, { 5, 0}, 0},
	{0x415BEAFE, {2,52, 0}, {11, 2}, FLAG_New3DS},
	{0x41C8A171, {2,52, 0}, {11, 2}, 0},
	{0x4380DB8D, {2,46, 0}, { 9, 0}, 0},
	{0x4A07016A, {2,54, 0}, {11, 4}, FLAG_New3DS},
	{0x4EE22A07, {2,55, 0}, {11, 8}, 0},
	{0x528E293F, {2,37, 0}, { 6, 0}, 0},
	{0x584C9AF5, {2,48, 3}, { 9, 3}, 0},
	{0x6488499E, {2,33, 4}, { 4, 0}, 0},
	{0x6E4ED781, {2,50,11}, {10, 4}, 0},
	{0x70A08ACD, {2,28, 0}, { 1, 1}, 0},
	{0x7421ACB4, {2,53, 0}, {11, 3}, 0},
	{0x7D1F9D40, {2,32, 0}, { 3, 0}, FLAG_SafeMode},	// FIXME: Verify kernel and FW versions!
	{0x80D26BB6, {2,30,18}, { 2, 1}, 0},
	{0x8662D9E4, {2,50, 7}, {10, 0}, FLAG_New3DS},
	{0x8904168D, {2,46, 0}, { 9, 0}, FLAG_New3DS},
	{0x90B92754, {2,38, 0}, { 6, 1}, 0},
	{0x925C092E, {2,45, 5}, { 8, 1}, FLAG_New3DS},
	{0x93D29ADA, {2,50, 7}, {10, 0}, 0},
	{0x9622D367, {2,44, 6}, { 8, 0}, 0},
	{0x985699BF, {2,51, 2}, {11, 1}, 0},
	{0x98640F5C, {2,34, 0}, { 4, 1}, 0},
	{0xA89A6392, {2,51, 0}, {11, 0}, FLAG_New3DS},
	{0xA8E660DF, {2,49, 0}, { 9, 5}, FLAG_New3DS},
	{0xAB6D5279, {2,51, 0}, {11, 0}, 0},
	{0xACCC5EC4, {2,50,11}, {10, 4}, FLAG_New3DS},
	{0xB1E2179B, {2,58, 0}, {11,16}, 0},
	{0xB7B6499E, {2,50, 1}, { 9, 6}, FLAG_New3DS},
	{0xBDD9D878, {2,50, 9}, {10, 2}, 0},
	{0xC110E2F9, {2,56, 0}, {11,12}, FLAG_New3DS},
	{0xC5380DCC, {2,53, 0}, {11, 3}, FLAG_New3DS},
	{0xC645B9A5, {2,50, 1}, { 9, 6}, 0},
	{0xC9829406, {2,29, 7}, { 2, 0}, 0},
	{0xDA0F7831, {2,54, 0}, {11, 4}, 0},
	{0xE0D74F64, {2,32,15}, { 3, 0}, 0},
	{0xE25F25F5, {2,31,40}, { 2, 2}, 0},
	{0xEA07F21E, {2,40, 0}, { 7, 2}, 0},
	{0xEE23547A, {2,55, 0}, {11, 8}, FLAG_New3DS},
	{0xF0ADC912, {2,57, 0}, {11,14}, FLAG_New3DS},
	{0xF5D833A2, {2,51, 2}, {11, 1}, FLAG_New3DS},
	{0xFA7997F7, {2,56, 0}, {11,12}, 0},
	{0xFFA6777A, {2,48, 3}, { 9, 3}, FLAG_New3DS},
}};

/** Public functions **/

/**
 * Look up a Nintendo 3DS firmware binary.
 * @param crc Firmware binary CRC32.
 * @return Firmware binary data, or nullptr if not found.
 */
const FirmBin_t *lookup_firmBin(const uint32_t crc)
{
	// Do a binary search.
	const FirmBin_t key = {crc, {0,0,0}, {0,0}, 0};
	void *ptr = bsearch(&key, firmBins.data(),
		firmBins.size(), sizeof(firmBins[0]),
		[](const void *a, const void *b) -> int
		{
			const FirmBin_t *const pa = static_cast<const FirmBin_t*>(a);
			const FirmBin_t *const pb = static_cast<const FirmBin_t*>(b);
			return (static_cast<int>(pa->crc) - static_cast<int>(pb->crc));
		});
	if (!ptr) {
		return nullptr;
	}

	return static_cast<const FirmBin_t*>(ptr);
}

} } // namespace LibRomData::Nintendo3DSFirmData

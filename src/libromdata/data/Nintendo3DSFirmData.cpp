/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirmData.cpp: Nintendo 3DS firmware data.                    *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "Nintendo3DSFirmData.hpp"

// C includes.
#include <stdlib.h>

namespace LibRomData {

class Nintendo3DSFirmDataPrivate {
	private:
		// Static class.
		Nintendo3DSFirmDataPrivate();
		~Nintendo3DSFirmDataPrivate();
		RP_DISABLE_COPY(Nintendo3DSFirmDataPrivate)

	public:
		/**
		 * Firmware binary version information.
		 * NOTE: Sorted by CRC32 for bsearch().
		 */
		static const Nintendo3DSFirmData::FirmBin_t firmBins[];

		/**
		 * Comparison function for bsearch().
		 * @param a
		 * @param b
		 * @return
		 */
		static int RP_C_API compar(const void *a, const void *b);
};

/** Nintendo3DSFirmDataPrivate **/

/**
 * Firmware binary version information.
 * NOTE: Sorted by CRC32 for bsearch().
 */
const Nintendo3DSFirmData::FirmBin_t Nintendo3DSFirmDataPrivate::firmBins[] = {
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
	{0xC5380DCC, {2,53, 0}, {11,3}, true},
	{0xC645B9A5, {2,50, 1}, { 9,6}, false},
	{0xC9829406, {2,29, 7}, { 2,0}, false},
	{0xDA0F7831, {2,54, 0}, {11,4}, false},
	{0xE0D74F64, {2,32,15}, { 3,0}, false},
	{0xE25F25F5, {2,31,40}, { 2,2}, false},
	{0xEA07F21E, {2,40, 0}, { 7,2}, false},
	{0xF5D833A2, {2,51, 2}, {11,1}, true},
	{0xFFA6777A, {2,48, 3}, { 9,3}, true},

	{0, {0,0,0}, {0,0}, false}
};

/**
 * Comparison function for bsearch().
 * @param a
 * @param b
 * @return
 */
int RP_C_API Nintendo3DSFirmDataPrivate::compar(const void *a, const void *b)
{
	uint32_t crc_1 = static_cast<const Nintendo3DSFirmData::FirmBin_t*>(a)->crc;
	uint32_t crc_2 = static_cast<const Nintendo3DSFirmData::FirmBin_t*>(b)->crc;
	if (crc_1 < crc_2) return -1;
	if (crc_1 > crc_2) return 1;
	return 0;
}

/** Nintendo3DSFirmData **/

/**
 * Look up a Nintendo 3DS firmware binary.
 * @param Firmware binary CRC32.
 * @return Firmware binary data, or nullptr if not found.
 */
const Nintendo3DSFirmData::FirmBin_t *Nintendo3DSFirmData::lookup_firmBin(const uint32_t crc)
{
	// Do a binary search.
	const FirmBin_t key = {crc, {0,0,0}, {0,0}, false};
	return static_cast<const FirmBin_t*>(bsearch(&key,
			Nintendo3DSFirmDataPrivate::firmBins,
			ARRAY_SIZE(Nintendo3DSFirmDataPrivate::firmBins)-1,
			sizeof(FirmBin_t),
			Nintendo3DSFirmDataPrivate::compar));
}

}

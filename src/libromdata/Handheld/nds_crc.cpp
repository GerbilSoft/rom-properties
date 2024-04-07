/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nds_crc.hpp: Nintendo DS CRC16 function.                                *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "nds_crc.hpp"

/**
 * Calculate the CRC16 of a block of data.
 * Polynomial: 0x8005 (for NDS icon/title)
 * @param buf Buffer
 * @param size Size of buffer
 * @param crc Previous CRC16 for chaining
 * @return CRC16
 */
uint16_t crc16_0x8005(const uint8_t *buf, size_t size, uint16_t crc)
{
	// Reference: https://www.reddit.com/r/embedded/comments/1acoobg/crc16_again_with_a_little_gift_for_you_all/
	// NOTE: NDS icon/title CRC16 uses polynomial 0x8005.

	while (size--) {
		uint32_t x = ((crc ^ *buf++) & 0xff) << 8;
		uint32_t y = x;

		x ^= x << 1;
		x ^= x << 2;
		x ^= x << 4;

		x  = (x & 0x8000) | (y >> 1);

		crc = (crc >> 8) ^ (x >> 15) ^ (x >> 1) ^ x;
	}

	return crc;
}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * nds_crc.hpp: Nintendo DS CRC16 function.                                *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate the CRC16 of a block of data.
 * Polynomial: 0x8005 (for NDS icon/title)
 * @param buf Buffer
 * @param size Size of buffer
 * @param crc Previous CRC16 for chaining (use 0xFFFFU for the initial block)
 * @return CRC16
 */
#ifdef __cplusplus
ATTR_ACCESS_SIZE(read_only, 1, 2)
uint16_t crc16_0x8005(const uint8_t *buf, size_t size, uint16_t crc = 0xFFFFU);
#else /* !__cplusplus */
ATTR_ACCESS_SIZE(read_only, 1, 2)
uint16_t crc16_0x8005(const uint8_t *buf, size_t size, uint16_t crc);
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif

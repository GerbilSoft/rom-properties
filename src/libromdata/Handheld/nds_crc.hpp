/*
	Cyclic Redundancy Code (CRC) functions
	by Rafael Vuijk (aka DarkFader)
*/

#pragma once

#include <string.h>

//#include "little.h"		// FixCrc is not yet big endian compatible

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data
 */
extern const uint16_t crc16tab[];

#ifdef __cplusplus
}
#endif

/*
 * CalcCrc16
 * Does not perform final inversion.
 */
static inline uint16_t CalcCrc16(const uint8_t *data, size_t length, uint16_t crc
#ifdef __cplusplus
	= (uint16_t)~0U
#endif /* __cplusplus */
	)
{
	for (; length > 0; length--, data++) {
		crc = (crc >> 8) ^ crc16tab[(crc ^ *data) & 0xFF];
	}
	return crc;
}

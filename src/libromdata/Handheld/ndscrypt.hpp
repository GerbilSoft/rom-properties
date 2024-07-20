/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ndscrypt.hpp: Nintendo DS encryption.                                   *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// nds-blowfish.bin size.
#define NDS_BLOWFISH_SIZE 0x1048U

typedef enum {
	NDSCRYPT_BF_NDS = 0,	// Nintendo DS
	NDSCRYPT_BF_DSi,	// Nintendo DSi (prod)
	NDSCRYPT_BF_DSi_DEVEL,	// Nintendo DSi (devel)

	NDSCRYPT_BF_MAX
} BlowfishKey;

/**
 * Load and verify a Blowfish file.
 * These must be present in order to use ndscrypt_secure_area().
 * @param bfkey Blowfish key ID.
 * @return 0 on success; negative POSIX error code, positive custom error code on error.
 */
int ndscrypt_load_blowfish_bin(BlowfishKey bfkey);

/**
 * Encrypt the ROM's Secure Area, if necessary.
 * @param pRom NDS or DSi secure area. (For DSi secure area, first 4 KB is the ROM header.)
 * @param len Length of pRom.
 * @param bfkey Blowfish key.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_encrypt_secure_area(uint8_t *pRom, size_t len, BlowfishKey bfkey);

/**
 * Decrypt the ROM's Secure Area, if necessary.
 * @param pRom NDS or DSi secure area. (For DSi secure area, first 4 KB is the ROM header.)
 * @param len Length of pRom.
 * @param bfkey Blowfish key.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_decrypt_secure_area(uint8_t *pRom, size_t len, BlowfishKey bfkey);

#ifdef __cplusplus
}
#endif

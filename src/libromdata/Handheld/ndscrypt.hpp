/***************************************************************************
 * Ortin (IS-NITRO management) (libortin)                                  *
 * ndscrypt.hpp: Nintendo DS encryption.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_NDSCRYPT_HPP__
#define __ROMPROPERTIES_LIBROMDATA_NDSCRYPT_HPP__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// nds-blowfish.bin size.
#define NDS_BLOWFISH_SIZE 0x1048

/**
 * Load and verify nds-blowfish.bin.
 * This must be present in order to use ndscrypt_secure_area().
 * TODO: Custom error codes enum.
 * @return 0 on success; negative POSIX error code, positive custom error code on error.
 */
int ndscrypt_load_nds_blowfish_bin(void);

/**
 * Encrypt the ROM's Secure Area, if necessary.
 * @param pRom First 32 KB of the ROM image.
 * @param len Length of pRom.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_encrypt_secure_area(uint8_t *pRom, size_t len);

/**
 * Decrypt the ROM's Secure Area, if necessary.
 * @param pRom First 32 KB of the ROM image.
 * @param len Length of pRom.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_decrypt_secure_area(uint8_t *pRom, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_NDSCRYPT_HPP__ */

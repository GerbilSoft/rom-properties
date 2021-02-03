/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * MD5HashNettle.cpp: MD5 hash class. (Win32 CryptoAPI implementation.)    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MD5Hash.hpp"

// Nettle MD5 functions.
#include <nettle/md5.h>

namespace LibRpBase {

/**
 * Calculate the MD5 hash of the specified data.
 * @param pHash		[out] Output hash buffer. (Must be 16 bytes.)
 * @param hash_len	[in] Size of hash buffer.
 * @param pData		[in] Input data.
 * @param len		[in] Data length.
 * @return 0 on success; negative POSIX error code on error.
 */
int MD5Hash::calcHash(uint8_t *pHash, size_t hash_len, const void *pData, size_t len)
{
	struct md5_ctx md5;

	assert(pHash != nullptr);
	assert(hash_len == 16);
	assert(pData != nullptr);
	if (!pHash || hash_len != 16 || !pData) {
		// Invalid parameters.
		return -EINVAL;
	}

	md5_init(&md5);
	md5_update(&md5, len, static_cast<const uint8_t*>(pData));
	md5_digest(&md5, hash_len, pHash);
	return 0;
}

}

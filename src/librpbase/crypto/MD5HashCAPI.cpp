/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * MD5HashCAPI.cpp: MD5 hash class. (Win32 CryptoAPI implementation.)      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MD5Hash.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.h"

// References:
// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
#include <wincrypt.h>

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
	HCRYPTPROV hProvider;
	HCRYPTHASH hHash;

	assert(pHash != nullptr);
	assert(hash_len == 16);
	assert(pData != nullptr);
	if (!pHash || hash_len != 16 || !pData) {
		// Invalid parameters.
		return -EINVAL;
	}

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProvider, nullptr, nullptr,
	    PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		// Failed to get a handle to the crypto provider.
		return -w32err_to_posix(GetLastError());
	}

	// Create an MD5 hash object.
	if (!CryptCreateHash(hProvider, CALG_MD5, 0, 0, &hHash)) {
		// Error creating the MD5 hash object.
		int ret = -w32err_to_posix(GetLastError());
		CryptReleaseContext(hProvider, 0);
		return ret;
	}

	// Hash the data.
	if (!CryptHashData(hHash, static_cast<const BYTE*>(pData), len, 0)) {
		// Error hashing the data.
		int ret = -w32err_to_posix(GetLastError());
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProvider, 0);
		return ret;
	}

	// Get the hash data.
	int ret = 0;
	DWORD cbHash = static_cast<DWORD>(hash_len);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, pHash, &cbHash, 0)) {
		// Error getting the hash.
		ret = -w32err_to_posix(GetLastError());
	} else if (cbHash != static_cast<DWORD>(hash_len)) {
		// Wrong hash length.
		ret = -EINVAL;
	}
	return ret;
}

}

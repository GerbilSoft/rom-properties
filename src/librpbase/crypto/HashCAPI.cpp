/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * HashCAPI.cpp: Hash class. (Win32 CryptoAPI implementation)              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Hash.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.hpp"

// References:
// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
#include <wincrypt.h>

namespace LibRpBase {

class HashPrivate
{
public:
	HashPrivate(Hash::Algorithm algorithm);
	~HashPrivate();

public:
	Hash::Algorithm algorithm;

	HCRYPTPROV hProvider;
	HCRYPTHASH hHash;
};

/** HashPrivate **/

HashPrivate::HashPrivate(Hash::Algorithm algorithm)
	: algorithm(algorithm)
	, hProvider(NULL)
	, hHash(NULL)
{
	// Get a handle to the crypto provider.
	// NOTE: MS_ENH_RSA_AES_PROV is required for SHA-256/SHA-512.
	// This provider requires Windows XP SP3 or later.
	if (!CryptAcquireContext(&hProvider, nullptr,
	    MS_ENH_RSA_AES_PROV, PROV_RSA_AES,
	    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		// Enhanced provider is not available.
		// Fall back to the standard provider.
		if (!CryptAcquireContext(&hProvider, nullptr,
		    nullptr, PROV_RSA_FULL,
		    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		{
			// Failed to get a handle to the crypto provider.
			// FIXME: MSVC is complaining if we use nullptr here:
			// error C2440: '=': cannot convert from 'nullptr' to 'HCRYPTPROV' (also 'HCRYPTHASH')
			hProvider = NULL;	// TODO: Is this necessary? (Verify nullptr)
		}
	}
}

HashPrivate::~HashPrivate()
{
	if (hHash)
		CryptDestroyHash(hHash);
	if (hProvider)
		CryptReleaseContext(hProvider, 0);
}

/** Hash **/

Hash::Hash(Algorithm algorithm)
	: d_ptr(new HashPrivate(algorithm))
{
	// Initialize the hash object.
	reset();
}

Hash::~Hash()
{
	delete d_ptr;
}

/**
 * Reset the internal hash state.
 */
void Hash::reset(void)
{
	RP_D(Hash);
	if (d->hHash) {
		CryptDestroyHash(d->hHash);
		d->hHash = NULL;
	}

	// Lookup table
	// NOTE: SHA256 and SHA512 requires XP SP3 or later.
	static const ALG_ID alg_id_tbl[] = {
		0,			// Unknown

		CALG_MD5,	// MD5
		CALG_SHA1,	// SHA1
		CALG_SHA_256,	// SHA256
		CALG_SHA_512,	// SHA512
	};

	assert(d->algorithm > Algorithm::Unknown);
	assert(d->algorithm < Algorithm::Max);
	if (d->algorithm <= Algorithm::Unknown || d->algorithm >= Algorithm::Max)
		return;

	const ALG_ID id = alg_id_tbl[static_cast<int>(d->algorithm)];
	if (!CryptCreateHash(d->hProvider, id, 0, 0, &d->hHash)) {
		// Error creating the hash object.
		// TODO: Verify that d->hHash is nullptr here.
		d->hHash = NULL;
	}
}

/**
 * Get the specified hash algorithm.
 * @return Hash algorithm.
 */
Hash::Algorithm Hash::algorithm(void) const
{
	RP_D(const Hash);
	return d->algorithm;
}

/**
 * Is the specified hash algorithm usable?
 * @return True if it is; false if it isn't.
 */
bool Hash::isUsable(void) const
{
	RP_D(const Hash);
	return (d->hHash != NULL);
}

/**
 * Process a block of data using the previously-specified hashing algorithm.
 * @param pData		[in] Input data
 * @param len		[in] Data length
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(read_only, 2, 3)
int Hash::process(const void *pData, size_t len)
{
	assert(pData != nullptr);
	if (!pData)
		return -EINVAL;
	else if (len == 0)
		return 0;

	RP_D(Hash);
	assert(d->hHash != NULL);
	if (!d->hHash)
		return -EINVAL;

	// Hash the data.
	if (!CryptHashData(d->hHash, static_cast<const BYTE*>(pData), static_cast<DWORD>(len), 0)) {
		// Error hashing the data.
		return -w32err_to_posix(GetLastError());
	}

	return 0;
}

/**
 * Get the hash length, in bytes.
 * @return Hash length, in bytes. (0 on error)
 */
size_t Hash::hashLength(void) const
{
	// TODO: Combine with HashNettle.
	RP_D(const Hash);

	// Lookup table
	static const uint8_t hash_length_tbl[] = {
		0,	// Unknown

		16,	// MD5
		20,	// SHA1
		32,	// SHA256
		64,	// SHA512
	};

	assert(d->algorithm > Algorithm::Unknown);
	assert(d->algorithm < Algorithm::Max);
	if (d->algorithm > Algorithm::Unknown && d->algorithm < Algorithm::Max) {
		return hash_length_tbl[static_cast<int>(d->algorithm)];
	}

	// Invalid algorithm.
	return 0;
}

/**
 * Get the hash value.
 * @param pHash		[out] Output buffer for the hash
 * @param hash_len	[in] Size of the output buffer, in bytes
 * @return 0 on success; negative POSIX error code on error.
 */
ATTR_ACCESS_SIZE(read_write, 2, 3)
int Hash::getHash(uint8_t *pHash, size_t hash_len)
{
	RP_D(Hash);

	const size_t expected_hash_len = hashLength();
	if (expected_hash_len == 0)
		return -EINVAL;
	else if (hash_len < expected_hash_len)
		return -ENOMEM;

	int ret = 0;
	DWORD cbHash = static_cast<DWORD>(hash_len);
	if (!CryptGetHashParam(d->hHash, HP_HASHVAL, pHash, &cbHash, 0)) {
		// Error getting the hash.
		ret = -w32err_to_posix(GetLastError());
	} else if (cbHash != static_cast<DWORD>(hash_len)) {
		// Wrong hash length.
		ret = -EINVAL;
	}

	return ret;
}

}

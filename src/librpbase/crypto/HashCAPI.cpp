/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * HashCAPI.cpp: Hash class. (Win32 CryptoAPI implementation)              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "Hash.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.hpp"

#ifdef ENABLE_DECRYPTION
// References:
// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
#  include <wincrypt.h>
#endif /* ENABLE_DECRYPTION */

// zlib for crc32()
#include <zlib.h>

// C++ STL classes
using std::array;

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
#  define CHECK_DELAYLOAD 1
#endif

#ifdef CHECK_DELAYLOAD
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
// Only checking for DelayLoad once.
#  include "librpthreads/pthread_once.h"
#endif /* _MSC_VER */

namespace LibRpBase {

#ifdef CHECK_DELAYLOAD
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* CHECK_DELAYLOAD */

class HashPrivate
{
public:
	HashPrivate(Hash::Algorithm algorithm);
	~HashPrivate();

public:
	Hash::Algorithm algorithm;

	union {
		uint32_t crc32;
#ifdef ENABLE_DECRYPTION
		struct {
			HCRYPTPROV hProvider;
			HCRYPTHASH hHash;
		};
#endif /* ENABLE_DECRYPTION */
	} ctx;

#ifdef CHECK_DELAYLOAD
	static pthread_once_t delay_load_check;
	static bool has_zlib;

	/**
	 * DelayLoad check function for zlib.
	 * NOTE: Call this function using pthread_once().
	 */
	static void delayload_check_once(void);
#endif /* CHECK_DELAYLOAD */
};

/** HashPrivate **/

#ifdef CHECK_DELAYLOAD
pthread_once_t HashPrivate::delay_load_check = PTHREAD_ONCE_INIT;
bool HashPrivate::has_zlib = false;
#endif /* CHECK_DELAYLOAD */

HashPrivate::HashPrivate(Hash::Algorithm algorithm)
	: algorithm(algorithm)
{
	if (algorithm == Hash::Algorithm::CRC32) {
		// Using zlib for CRC32, not CryptoAPI.
		ctx.crc32 = 0;
		return;
	}

#ifdef ENABLE_DECRYPTION
	// Get a handle to the crypto provider.
	// NOTE: MS_ENH_RSA_AES_PROV is required for SHA-256/SHA-512.
	// This provider requires Windows XP SP3 or later.
	if (!CryptAcquireContext(&ctx.hProvider, nullptr,
	    MS_ENH_RSA_AES_PROV, PROV_RSA_AES,
	    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		// Enhanced provider is not available.
		// Fall back to the standard provider.
		if (!CryptAcquireContext(&ctx.hProvider, nullptr,
		    nullptr, PROV_RSA_FULL,
		    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		{
			// Failed to get a handle to the crypto provider.
			// FIXME: MSVC is complaining if we use nullptr here:
			// error C2440: '=': cannot convert from 'nullptr' to 'HCRYPTPROV' (also 'HCRYPTHASH')
			ctx.hProvider = NULL;	// TODO: Is this necessary? (Verify nullptr)
		}
	}
#endif /* ENABLE_DECRYPTION */
}

HashPrivate::~HashPrivate()
{
#ifdef ENABLE_DECRYPTION
	if (algorithm != Hash::Algorithm::CRC32) {
		if (ctx.hHash)
			CryptDestroyHash(ctx.hHash);
		if (ctx.hProvider)
			CryptReleaseContext(ctx.hProvider, 0);
	}
#endif /* ENABLE_DECRYPTION */
}

#ifdef CHECK_DELAYLOAD
/**
 * DelayLoad check function for zlib.
 * NOTE: Call this function using pthread_once().
 */
void HashPrivate::delayload_check_once(void)
{
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	has_zlib = (DelayLoad_test_get_crc_table() == 0);
}
#endif /* CHECK_DELAYLOAD */

/** Hash **/

Hash::Hash(Algorithm algorithm)
	: d_ptr(new HashPrivate(algorithm))
{
	// Initialize the hash object.
	reset();

	if (algorithm == Algorithm::CRC32) {
#ifdef CHECK_DELAYLOAD
		d_ptr->delayload_check_once();
#else /* CHECK_DELAYLOAD */
		// Not checking for DelayLoad, but we need to ensure that the
		// CRC table is initialized anyway.
		get_crc_table();
#endif /* CHECK_DELAYLOAD */
	}
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
	if (d->algorithm == Algorithm::CRC32) {
		// Using zlib for CRC32, not CryptoAPI.
		d->ctx.crc32 = 0;
		return;
	}

#ifdef ENABLE_DECRYPTION
	if (d->ctx.hHash) {
		CryptDestroyHash(d->ctx.hHash);
		d->ctx.hHash = NULL;
	}

	// Lookup table
	// NOTE: SHA256 and SHA512 requires XP SP3 or later.
	static constexpr array<ALG_ID, (int)Algorithm::Max> alg_id_tbl = {{
		0,			// Unknown

		0,		// CRC32 (not using CryptoAPI)
		CALG_MD5,	// MD5
		CALG_SHA1,	// SHA1
		CALG_SHA_256,	// SHA256
		CALG_SHA_512,	// SHA512
	}};

	assert(d->algorithm > Algorithm::CRC32);
	assert(d->algorithm < Algorithm::Max);
	if (d->algorithm <= Algorithm::CRC32 || d->algorithm >= Algorithm::Max)
		return;

	const ALG_ID id = alg_id_tbl[static_cast<int>(d->algorithm)];
	if (!CryptCreateHash(d->ctx.hProvider, id, 0, 0, &d->ctx.hHash)) {
		// Error creating the hash object.
		// TODO: Verify that d->hHash is nullptr here.
		d->ctx.hHash = NULL;
	}
#endif /* ENABLE_DECRYPTION */
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
	if (d->algorithm == Algorithm::CRC32) {
		// Using zlib for CRC32.
#ifdef CHECK_DELAYLOAD
		return d->has_zlib;
#else /* !CHECK_DELAYLOAD */
		return true;
#endif /* CHECK_DELAYLOAD */
	}

#ifdef ENABLE_DECRYPTION
	return (d->ctx.hHash != NULL);
#else /* !ENABLE_DECRYPTION */
	return false;
#endif /* ENABLE_DECRYPTION */
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
	if (d->algorithm == Algorithm::CRC32) {
		// Using zlib for CRC32.
#ifdef CHECK_DELAYLOAD
		if (!d->has_zlib) {
			// zlib is not available...
			return -EIO;	// TODO: Better error code?
		}
#endif /* CHECK_DELAYLOAD */
		d->ctx.crc32 = crc32(d->ctx.crc32, static_cast<const uint8_t*>(pData), len);
		return 0;
	}

#ifdef ENABLE_DECRYPTION
	assert(d->ctx.hHash != NULL);
	if (!d->ctx.hHash)
		return -EINVAL;

	// Hash the data.
	if (!CryptHashData(d->ctx.hHash, static_cast<const BYTE*>(pData), static_cast<DWORD>(len), 0)) {
		// Error hashing the data.
		return -w32err_to_posix(GetLastError());
	}

	return 0;
#else /* !ENABLE_DECRYPTION */
	return -ENOTSUP;
#endif /* ENABLE_DECRYPTION */
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
	static constexpr array<uint8_t, static_cast<size_t>(Algorithm::Max)> hash_length_tbl = {{
		0,			// Unknown

		sizeof(uint32_t),	// CRC32
#ifdef ENABLE_DECRYPTION
		16,			// MD5
		20,			// SHA1
		32,			// SHA256
		64,			// SHA512
#endif /* ENABLE_DECRYPTION */
	}};

	assert(d->algorithm > Algorithm::Unknown);
	assert(static_cast<size_t>(d->algorithm) < hash_length_tbl.size());
	if (d->algorithm > Algorithm::Unknown && static_cast<size_t>(d->algorithm) < hash_length_tbl.size()) {
		return hash_length_tbl[static_cast<size_t>(d->algorithm)];
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

	if (d->algorithm == Algorithm::CRC32) {
		// Using zlib for CRC32, not CryptoAPI.
		// NOTE: Converting to big-endian format.
		const uint32_t crc32_be = cpu_to_be32(d->ctx.crc32);
		memcpy(pHash, &crc32_be, sizeof(crc32_be));
		return 0;
	}

#ifdef ENABLE_DECRYPTION
	int ret = 0;
	DWORD cbHash = static_cast<DWORD>(hash_len);
	if (!CryptGetHashParam(d->ctx.hHash, HP_HASHVAL, pHash, &cbHash, 0)) {
		// Error getting the hash.
		ret = -w32err_to_posix(GetLastError());
	} else if (cbHash != static_cast<DWORD>(hash_len)) {
		// Wrong hash length.
		ret = -EINVAL;
	}

	return ret;
#else /* !ENABLE_DECRYPTION */
	return -ENOTSUP;
#endif /* ENABLE_DECRYPTION */
}

/**
 * Get the hash value as uint32_t. (32-bit hashes only!)
 * @return 32-bit hash on success; 0 on error.
 */
uint32_t Hash::getHash32(void) const
{
	RP_D(const Hash);
	assert(d->algorithm == Algorithm::CRC32);
	if (d->algorithm != Algorithm::CRC32)
		return 0;

	return d->ctx.crc32;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * HashNettle.cpp: Hash class. (GNU nettle implementation)                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Hash.hpp"

// zlib for crc32()
#include <zlib.h>

#ifdef ENABLE_DECRYPTION
// Nettle hash functions
#  include <nettle/md5.h>
#  include <nettle/sha1.h>
#  include <nettle/sha2.h>
#endif /* ENABLE_DECRYPTION */

namespace LibRpBase {

class HashPrivate
{
public:
	HashPrivate(Hash::Algorithm algorithm);

public:
	Hash::Algorithm algorithm;

	// Hash::Hash() initializes this by calling reset().
	union {
		uint32_t crc32;
#ifdef ENABLE_DECRYPTION
		struct md5_ctx md5;
		struct sha1_ctx sha1;
		struct sha256_ctx sha256;
		struct sha512_ctx sha512;
#endif /* ENABLE_DECRYPTION */
	} ctx;
};

/** HashPrivate **/

HashPrivate::HashPrivate(Hash::Algorithm algorithm)
	: algorithm(algorithm)
{}

/** Hash **/

Hash::Hash(Algorithm algorithm)
	: d_ptr(new HashPrivate(algorithm))
{
	// Initialize the appropriate hash struct.
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
	switch (d->algorithm) {
		default:
			assert(!"Invalid hash algorithm specified.");
			break;
		case Algorithm::CRC32:
			d->ctx.crc32 = 0U;
			break;
#ifdef ENABLE_DECRYPTION
		case Algorithm::MD5:
			md5_init(&d->ctx.md5);
			break;
		case Algorithm::SHA1:
			sha1_init(&d->ctx.sha1);
			break;
		case Algorithm::SHA256:
			sha256_init(&d->ctx.sha256);
			break;
		case Algorithm::SHA512:
			sha512_init(&d->ctx.sha512);
			break;
#endif /* ENABLE_DECRYPTION */
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
#ifdef ENABLE_DECRYPTION
	// TODO: Check supported Nettle versions and handle this properly.
	return (d->algorithm > Algorithm::Unknown && d->algorithm < Algorithm::Max);
#else /* !ENABLE_DECRYPTION */
	return (d->algorithm == Algorithm::CRC32);
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
	switch (d->algorithm) {
		default:
			assert(!"Invalid hash algorithm specified.");
			return -ENOTSUP;
		case Algorithm::CRC32:
			d->ctx.crc32 = crc32(d->ctx.crc32, static_cast<const uint8_t*>(pData), len);
			break;
#ifdef ENABLE_DECRYPTION
		case Algorithm::MD5:
			md5_update(&d->ctx.md5, len, static_cast<const uint8_t*>(pData));
			break;
		case Algorithm::SHA1:
			sha1_update(&d->ctx.sha1, len, static_cast<const uint8_t*>(pData));
			break;
		case Algorithm::SHA256:
			sha256_update(&d->ctx.sha256, len, static_cast<const uint8_t*>(pData));
			break;
		case Algorithm::SHA512:
			sha512_update(&d->ctx.sha512, len, static_cast<const uint8_t*>(pData));
			break;
#endif /* ENABLE_DECRYPTION */
	}

	return 0;
}

/**
 * Get the hash length, in bytes.
 * @return Hash length, in bytes. (0 on error)
 */
size_t Hash::hashLength(void) const
{
	RP_D(const Hash);

	// Lookup table
	static const uint8_t hash_length_tbl[] = {
		0,			// Unknown

		sizeof(uint32_t),	// CRC32
#ifdef ENABLE_DECRYPTION
		MD5_DIGEST_SIZE,	// MD5
		SHA1_DIGEST_SIZE,	// SHA1
		SHA256_DIGEST_SIZE,	// SHA256
		SHA512_DIGEST_SIZE,	// SHA512
#endif /* ENABLE_DECRYPTION */
	};

	assert(d->algorithm > Algorithm::Unknown);
	assert(d->algorithm < (Hash::Algorithm)ARRAY_SIZE(hash_length_tbl));
	if (d->algorithm > Algorithm::Unknown && d->algorithm < (Hash::Algorithm)ARRAY_SIZE(hash_length_tbl)) {
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

	switch (d->algorithm) {
		default:
			assert(!"Invalid hash algorithm specified.");
			return -EINVAL;
		case Algorithm::CRC32: {
			// NOTE: Converting to big-endian format.
			const uint32_t crc32_be = cpu_to_be32(d->ctx.crc32);
			memcpy(pHash, &crc32_be, sizeof(crc32_be));
			break;
		}
#ifdef ENABLE_DECRYPTION
		case Algorithm::MD5:
			md5_digest(&d->ctx.md5, hash_len, pHash);
			break;
		case Algorithm::SHA1:
			sha1_digest(&d->ctx.sha1, hash_len, pHash);
			break;
		case Algorithm::SHA256:
			sha256_digest(&d->ctx.sha256, hash_len, pHash);
			break;
		case Algorithm::SHA512:
			sha512_digest(&d->ctx.sha512, hash_len, pHash);
			break;
#endif /* ENABLE_DECRYPTION */
	}

	return 0;
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

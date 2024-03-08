/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * HashNettle.cpp: Hash class. (GNU nettle implementation)                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Hash.hpp"

// Nettle hash MD5 functions
#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>

namespace LibRpBase {

class HashPrivate
{
public:
	HashPrivate(Hash::Algorithm algorithm);

public:
	Hash::Algorithm algorithm;

	// Hash::Hash() initializes this by calling reset().
	union {
		struct md5_ctx md5;
		struct sha1_ctx sha1;
		struct sha256_ctx sha256;
		struct sha512_ctx sha512;
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
	// TODO: Check supported Nettle versions and handle this properly.
	return true;
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
			return -EINVAL;
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

		MD5_DIGEST_SIZE,	// MD5
		SHA1_DIGEST_SIZE,	// SHA1
		SHA256_DIGEST_SIZE,	// SHA256
		SHA512_DIGEST_SIZE,	// SHA512
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

	switch (d->algorithm) {
		default:
			assert(!"Invalid hash algorithm specified.");
			return -EINVAL;
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
	}

	return 0;
}

}

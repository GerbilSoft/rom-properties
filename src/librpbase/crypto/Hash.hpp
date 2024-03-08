/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * Hash.hpp: Hash class.                                                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

namespace LibRpBase {

class HashPrivate;
class RP_LIBROMDATA_PUBLIC Hash
{
public:
	enum class Algorithm {
		Unknown = 0,

		CRC32	= 1,
		MD5	= 2,
		SHA1	= 3,
		SHA256	= 4,
		SHA512	= 5,

		Max
	};

public:
	explicit Hash(Algorithm algorithm);
	~Hash();

private:
	RP_DISABLE_COPY(Hash)
protected:
	friend class HashPrivate;
	HashPrivate *const d_ptr;

public:
	/**
	 * Reset the internal hash state.
	 */
	void reset(void);

	/**
	 * Get the specified hash algorithm.
	 * @return Hash algorithm.
	 */
	Algorithm algorithm(void) const;

	/**
	 * Is the specified hash algorithm usable?
	 * @return True if it is; false if it isn't.
	 */
	bool isUsable(void) const;

	/**
	 * Process a block of data using the previously-specified hashing algorithm.
	 * @param pData		[in] Input data
	 * @param len		[in] Data length
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	int process(const void *pData, size_t len);

	/**
	 * Get the hash length, in bytes.
	 * @return Hash length, in bytes. (0 on error)
	 */
	size_t hashLength(void) const;

	/**
	 * Get the hash value.
	 * @param pHash		[out] Output buffer for the hash
	 * @param hash_len	[in] Size of the output buffer, in bytes
	 * @return 0 on success; negative POSIX error code on error.
	 */
	ATTR_ACCESS_SIZE(read_write, 2, 3)
	int getHash(uint8_t *pHash, size_t hash_len);

	/**
	 * Get the hash value as uint32_t. (32-bit hashes only!)
	 * @return 32-bit hash on success; 0 on error.
	 */
	uint32_t getHash32(void) const;
};

}

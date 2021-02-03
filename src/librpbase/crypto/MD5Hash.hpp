/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * MD5Hash.hpp: MD5 hash class.                                            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CRYPTO_MD5_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CRYPTO_MD5_HPP__

#include "common.h"

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

namespace LibRpBase {

class MD5Hash
{
	protected:
		MD5Hash() { }
		~MD5Hash() { }

	private:
		RP_DISABLE_COPY(MD5Hash)

	public:
		/**
		 * Calculate the MD5 hash of the specified data.
		 * @param pHash		[out] Output hash buffer. (Must be 16 bytes.)
		 * @param hash_len	[in] Size of hash buffer.
		 * @param pData		[in] Input data.
		 * @param len		[in] Data length.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		ATTR_ACCESS_SIZE(read_write, 1, 2)
		ATTR_ACCESS_SIZE(read_only, 3, 4)
		static int calcHash(uint8_t *pHash, size_t hash_len, const void *pData, size_t len);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CRYPTO_MD5_HPP__ */

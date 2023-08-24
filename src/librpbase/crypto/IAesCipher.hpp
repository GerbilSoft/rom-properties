/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IAesCipher.hpp: AES decryption interface.                               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

namespace LibRpBase {

class NOVTABLE IAesCipher
{
	protected:
		explicit IAesCipher() = default;
	public:
		virtual ~IAesCipher() = 0;

	private:
		RP_DISABLE_COPY(IAesCipher)

	public:
		/**
		 * Get the name of the AesCipher implementation.
		 * @return Name.
		 */
		virtual const char *name(void) const = 0;

		/**
		 * Has the cipher been initialized properly?
		 * @return True if initialized; false if not.
		 */
		virtual bool isInit(void) const = 0;

		/**
		 * Set the encryption key.
		 * @param pKey	[in] Key data.
		 * @param size	[in] Size of pKey, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		virtual int setKey(const uint8_t *RESTRICT pKey, size_t size) = 0;

		enum class ChainingMode {
			ECB,
			CBC,
			CTR,

			Max
		};

		/**
		 * Set the cipher chaining mode.
		 *
		 * Note that the IV/counter must be set *after* setting
		 * the chaining mode; otherwise, setIV() will fail.
		 *
		 * @param mode Cipher chaining mode.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setChainingMode(ChainingMode mode) = 0;

		/**
		 * Set the IV (CBC mode) or counter (CTR mode).
		 * @param pIV	[in] IV/counter data.
		 * @param size	[in] Size of pIV, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		virtual int setIV(const uint8_t *RESTRICT pIV, size_t size) = 0;

		/**
		 * Decrypt a block of data.
		 * Key and IV/counter must be set before calling this function.
		 *
		 * @param pData	[in/out] Data block.
		 * @param size	[in] Length of data block. (Must be a multiple of 16.)
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		ATTR_ACCESS_SIZE(read_write, 2, 3)
		virtual size_t decrypt(uint8_t *RESTRICT pData, size_t size) = 0;

		/**
		 * Decrypt a block of data.
		 * Key must be set before calling this function.
		 *
		 * @param pData		[in/out] Data block.
		 * @param size		[in] Length of data block. (Must be a multiple of 16.)
		 * @param pIV		[in] IV/counter for the data block.
		 * @param size_iv	[in] Size of pIV, in bytes.
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		ATTR_ACCESS_SIZE(read_write, 2, 3)
		ATTR_ACCESS_SIZE(read_only, 4, 5)
		inline size_t decrypt(uint8_t *RESTRICT pData, size_t size,
			const uint8_t *RESTRICT pIV, size_t size_iv)
		{
			int ret = setIV(pIV, size_iv);
			if (ret != 0) {
				return 0;
			}
			return decrypt(pData, size);
		}
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IAesCipher::~IAesCipher() { }

}

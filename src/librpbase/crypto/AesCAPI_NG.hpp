/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCAPI_NG.hpp: AES decryption class using Win32 CryptoAPI NG.          *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCAPI_NG_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCAPI_NG_HPP__

#include "IAesCipher.hpp"

namespace LibRpBase {

class AesCAPI_NG_Private;
class AesCAPI_NG : public IAesCipher
{
	public:
		AesCAPI_NG();
		virtual ~AesCAPI_NG();

	private:
		typedef IAesCipher super;
		RP_DISABLE_COPY(AesCAPI_NG)
	private:
		friend class AesCAPI_NG_Private;
		AesCAPI_NG_Private *const d_ptr;

	public:
		/**
		 * Is CryptoAPI NG usable on this system?
		 *
		 * If CryptoAPI NG is usable, this function will load
		 * bcrypt.dll and all required function pointers.
		 *
		 * @return True if this system supports CryptoAPI NG.
		 */
		static bool isUsable(void);

	public:
		/**
		 * Get the name of the AesCipher implementation.
		 * @return Name.
		 */
		const char *name(void) const final;

		/**
		 * Has the cipher been initialized properly?
		 * @return True if initialized; false if not.
		 */
		bool isInit(void) const final;

		/**
		 * Set the encryption key.
		 * @param pKey	[in] Key data.
		 * @param size	[in] Size of pKey, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int setKey(const uint8_t *RESTRICT pKey, size_t size) final;

		/**
		 * Set the cipher chaining mode.
		 *
		 * Note that the IV/counter must be set *after* setting
		 * the chaining mode; otherwise, setIV() will fail.
		 *
		 * @param mode Cipher chaining mode.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int setChainingMode(ChainingMode mode) final;

		/**
		 * Set the IV (CBC mode) or counter (CTR mode).
		 * @param pIV	[in] IV/counter data.
		 * @param size	[in] Size of pIV, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int setIV(const uint8_t *RESTRICT pIV, size_t size) final;

		/**
		 * Decrypt a block of data.
		 * Key and IV/counter must be set before calling this function.
		 *
		 * @param pData	[in/out] Data block.
		 * @param size	[in] Length of data block. (Must be a multiple of 16.)
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		size_t decrypt(uint8_t *RESTRICT pData, size_t size) final;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CRYPTO_AESCAPI_NG_HPP__ */

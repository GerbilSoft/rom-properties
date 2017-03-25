/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCAPI.hpp: AES decryption class using Win32 CryptoAPI.                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCAPI_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCAPI_HPP__

#include "IAesCipher.hpp"

namespace LibRomData {

class AesCAPIPrivate;
class AesCAPI : public IAesCipher
{
	public:
		AesCAPI();
		virtual ~AesCAPI();

	private:
		typedef IAesCipher super;
		RP_DISABLE_COPY(AesCAPI)
	private:
		friend class AesCAPIPrivate;
		AesCAPIPrivate *const d_ptr;

	public:
		/**
		 * Get the name of the AesCipher implementation.
		 * @return Name.
		 */
		virtual const rp_char *name(void) const override final;

		/**
		 * Has the cipher been initialized properly?
		 * @return True if initialized; false if not.
		 */
		virtual bool isInit(void) const override final;

		/**
		 * Set the encryption key.
		 * @param key Key data.
		 * @param len Key length, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setKey(const uint8_t *key, unsigned int len) override final;

		/**
		 * Set the cipher chaining mode.
		 *
		 * Note that the IV/counter must be set *after* setting
		 * the chaining mode; otherwise, setIV() will fail.
		 *
		 * @param mode Cipher chaining mode.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setChainingMode(ChainingMode mode) override final;

		/**
		 * Set the IV (CBC mode) or counter (CTR mode).
		 * @param iv IV/counter data.
		 * @param len IV/counter length, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setIV(const uint8_t *iv, unsigned int len) override final;

		/**
		 * Decrypt a block of data.
		 * @param data Data block.
		 * @param data_len Length of data block.
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		virtual unsigned int decrypt(uint8_t *data, unsigned int data_len) override final;

		/**
		 * Decrypt a block of data using the specified IV (CBC mode) or counter (CTR mode).
		 * @param data Data block.
		 * @param data_len Length of data block.
		 * @param iv IV/counter for the data block.
		 * @param iv_len Length of the IV/counter.
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		virtual unsigned int decrypt(uint8_t *data, unsigned int data_len,
			const uint8_t *iv, unsigned int iv_len) override final;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCAPI_HPP__ */

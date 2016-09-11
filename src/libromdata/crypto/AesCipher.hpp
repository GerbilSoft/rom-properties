/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCipher.hpp: AES decryption class.                                    *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHER_HPP__

// C includes.
#include <stdint.h>

namespace LibRomData {

class AesCipherPrivate;
class AesCipher
{
	public:
		AesCipher();
		virtual ~AesCipher();

	private:
		AesCipher(const AesCipher &);
		AesCipher &operator=(const AesCipher &);
	private:
		friend class AesCipherPrivate;
		AesCipherPrivate *const d;

	public:
		/**
		 * Has the cipher been initialized properly?
		 * @return True if initialized; false if not.
		 */
		virtual bool isInit(void) const;

		/**
		 * Set the encryption key.
		 * @param key Key data.
		 * @param len Key length, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setKey(const uint8_t *key, uint32_t len);

		enum ChainingMode {
			CM_EBC,
			CM_CBC,
		};

		/**
		 * Set the cipher chaining mode.
		 * @param mode Cipher chaining mode.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setChainingMode(ChainingMode mode);

		/**
		 * Set the IV.
		 * @param iv IV data.
		 * @param len IV length, in bytes.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int setIV(const uint8_t *iv, uint32_t len);

		/**
		 * Decrypt a block of data.
		 * @param data Data block.
		 * @param data_len Length of data block.
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		virtual uint32_t decrypt(uint8_t *data, uint32_t data_len);

		/**
		 * Decrypt a block of data using the specified IV.
		 * @param data Data block.
		 * @param data_len Length of data block.
		 * @param iv IV for the data block.
		 * @param iv_len Length of the IV.
		 * @return Number of bytes decrypted on success; 0 on error.
		 */
		virtual uint32_t decrypt(uint8_t *data, uint32_t data_len,
			const uint8_t *iv, uint32_t iv_len);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_AESCIPHER_HPP__ */

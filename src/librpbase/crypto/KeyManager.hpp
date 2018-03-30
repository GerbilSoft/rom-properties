/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * KeyManager.hpp: Encryption key manager.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_CRYPTO_KEYMANAGER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CRYPTO_KEYMANAGER_HPP__

#include "librpbase/config.librpbase.h"
#include "../config/ConfReader.hpp"

// C includes.
#include <stdint.h>

namespace LibRpBase {

class KeyManager : public ConfReader
{
	protected:
		/**
		 * KeyManager class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		KeyManager();

	private:
		typedef ConfReader super;
		RP_DISABLE_COPY(KeyManager)

	private:
		friend class KeyManagerPrivate;

	public:
		/**
		 * Key verification result.
		 */
		enum VerifyResult {
			VERIFY_UNKNOWN			= -1,	// Unknown status.
			VERIFY_OK			= 0,	// Key obtained/verified.
			VERIFY_INVALID_PARAMS		= 1,	// Parameters are invalid.
			VERIFY_NO_SUPPORT		= 2,	// Decryption is not supported.
			VERIFY_KEY_DB_NOT_LOADED	= 3,	// Key database is not loaded.
			VERIFY_KEY_DB_ERROR		= 4,	// Something's wrong with the key database.
			VERIFY_KEY_NOT_FOUND		= 5,	// Key was not found.
			VERIFY_KEY_INVALID		= 6,	// Key is not valid for this operation.
			VERFIY_IAESCIPHER_INIT_ERR	= 7,	// IAesCipher could not be created.
			VERIFY_IAESCIPHER_DECRYPT_ERR	= 8,	// IAesCipher::decrypt() failed.
			VERIFY_WRONG_KEY		= 9,	// The key did not decrypt the test string correctly.

			VERIFY_MAX
		};

		/**
		 * Get a description for a VerifyResult.
		 * @param res VerifyResult.
		 * @return Description, or nullptr if invalid.
		 */
		static const char *verifyResultToString(VerifyResult res);

#ifdef ENABLE_DECRYPTION
	public:
		/**
		 * Get the KeyManager instance.
		 * @return KeyManager instance.
		 */
		static KeyManager *instance(void);

	public:
		// Encryption key data.
		struct KeyData_t {
			const uint8_t *key;	// Key data.
			uint32_t length;	// Key length.
		};

		/**
		 * Get an encryption key.
		 * @param keyName	[in]  Encryption key name.
		 * @param pKeyData	[out,opt] Key data struct. (If nullptr, key will be checked but not loaded.)
		 * @return VerifyResult.
		 */
		VerifyResult get(const char *keyName, KeyData_t *pKeyData) const;

		/**
		 * Verify and retrieve an encryption key.
		 *
		 * This will decrypt the specified block of data
		 * using the key with AES-128-ECB, which will result
		 * in the 16-byte string "AES-128-ECB-TEST".
		 *
		 * If the key is valid, pKeyData will be populated
		 * with the key information, similar to get().
		 *
		 * @param keyName	[in] Encryption key name.
		 * @param pKeyData	[out,opt] Key data struct. (If nullptr, key will be checked but not loaded.)
		 * @param pVerifyData	[in] Verification data block.
		 * @param verifyLen	[in] Length of pVerifyData. (Must be 16.)
		 * @return VerifyResult.
		 */
		VerifyResult getAndVerify(const char *keyName, KeyData_t *pKeyData,
			const uint8_t *pVerifyData, unsigned int verifyLen) const;

		// Verification test string.
		// NOTE: This string is NOT NULL-terminated!
		static const char verifyTestString[16];

		/**
		 * Convert string data from hexadecimal to bytes.
		 * @param str	[in] String data. (Must be len*2 characters.)
		 * @param buf	[out] Output buffer.
		 * @param len	[in] Size of buf, in bytes.
		 * @return 0 on success; non-zero on error.
		 */
		template<typename Char>
		static int hexStringToBytes(const Char *str, uint8_t *buf, unsigned int len);
#endif /* ENABLE_DECRYPTION */
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CRYPTO_KEYMANAGER_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * KeyManager.hpp: Encryption key manager.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "../config/ConfReader.hpp"
#include "common.h"

// C includes (C++ namespace)
#include <cstdint>

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
		 * Key verification result
		 */
		enum class VerifyResult : int8_t {
			Unknown			= -1,	// Unknown status.
			OK			= 0,	// Key obtained/verified.
			InvalidParams		= 1,	// Parameters are invalid.
			NoSupport		= 2,	// Decryption is not supported.
			KeyDBNotLoaded		= 3,	// Key database is not loaded.
			KeyDBError		= 4,	// Something's wrong with the key database.
			KeyNotFound		= 5,	// Key was not found.
			KeyInvalid		= 6,	// Key is not valid for this operation.
			IAesCipherInitErr	= 7,	// IAesCipher could not be created.
			IAesCipherDecryptErr	= 8,	// IAesCipher::decrypt() failed.
			WrongKey		= 9,	// The key did not decrypt the test string correctly.
			IncrementingValues	= 10,	// Partition is incrementing value garbage.

			Max
		};

		/**
		 * Get a description for a VerifyResult.
		 * @param res VerifyResult.
		 * @return Description, or nullptr if invalid.
		 */
		RP_LIBROMDATA_PUBLIC
		static const char *verifyResultToString(VerifyResult res);

#ifdef ENABLE_DECRYPTION
	public:
		/**
		 * Get the KeyManager instance.
		 * @return KeyManager instance.
		 */
		RP_LIBROMDATA_PUBLIC
		static KeyManager *instance(void);

	public:
		// Encryption key data
		struct KeyData_t {
			const uint8_t *key;	// Key data
			size_t length;		// Key length
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
		ATTR_ACCESS_SIZE(read_only, 4, 5)
		RP_LIBROMDATA_PUBLIC
		VerifyResult getAndVerify(const char *keyName, KeyData_t *pKeyData,
			const uint8_t *pVerifyData, unsigned int verifyLen) const;

		// Verification test string.
		// NOTE: This string is NOT NULL-terminated!
		static const char verifyTestString[16];

		/**
		 * Convert string data from hexadecimal to bytes.
		 * @param str	[in] String data (Must be len*2 characters.)
		 * @param buf	[out] Output buffer
		 * @param len	[in] Size of buf, in bytes
		 * @return 0 on success; non-zero on error.
		 */
		template<typename Char>
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		static int hexStringToBytes(const Char *str, uint8_t *buf, size_t len);
#endif /* ENABLE_DECRYPTION */
};

}

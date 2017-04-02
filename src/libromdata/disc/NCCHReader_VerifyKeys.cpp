/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NCCHReader_VerifyKeys.cpp: Nintendo 3DS NCCH reader.                    *
 * Key verification data.                                                  *
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

#include "NCCHReader_p.hpp"
#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif /* !ENABLE_DECRYPTION */

#include "crypto/IAesCipher.hpp"
#include "crypto/AesCipherFactory.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRomData {

/**
 * Attempt to load an AES normal key.
 * @param pKeyOut		[out] Output key data.
 * @param keyNormal_name	[in,opt] KeyNormal slot name.
 * @param keyX_name		[in,opt] KeyX slot name.
 * @param keyY_name		[in,opt] KeyY slot name.
 * @param keyNormal_verify	[in,opt] KeyNormal verification data. (NULL or 16 bytes)
 * @param keyX_verify		[in,opt] KeyX verification data. (NULL or 16 bytes)
 * @param keyY_verify		[in,opt] KeyY verification data. (NULL or 16 bytes)
 * @return VerifyResult.
 */
KeyManager::VerifyResult NCCHReaderPrivate::loadKeyNormal(u128_t *pKeyOut,
	const char *keyNormal_name,
	const char *keyX_name,
	const char *keyY_name,
	const uint8_t *keyNormal_verify,
	const uint8_t *keyX_verify,
	const uint8_t *keyY_verify)
{
	KeyManager::VerifyResult res;

	// Get the Key Manager instance.
	KeyManager *keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager) {
		// TODO: Some other error?
		return KeyManager::VERIFY_KEY_DB_ERROR;
	}

	// Attempt to load the Normal key first.
	if (keyNormal_name) {
		KeyManager::KeyData_t keyNormal_data;
		if (keyNormal_verify) {
			res = keyManager->getAndVerify(
				keyNormal_name, &keyNormal_data,
				keyNormal_verify, 16);
		} else {
			res = keyManager->get(keyNormal_name, &keyNormal_data);
		}

		if (res == KeyManager::VERIFY_OK && keyNormal_data.length == 16) {
			// KeyNormal loaded and verified.
			memcpy(pKeyOut->u8, keyNormal_data.key, 16);
			return KeyManager::VERIFY_OK;
		}

		// Check for database errors.
		switch (res) {
			case KeyManager::VERIFY_INVALID_PARAMS:
			case KeyManager::VERIFY_KEY_DB_NOT_LOADED:
			case KeyManager::VERIFY_KEY_DB_ERROR:
				// Database error. Don't continue.
				return res;
			default:
				break;
		}
	}

	// Could not load the Normal key.
	// Load KeyX and KeyY.
	if (!keyX_name || !keyY_name) {
		// One of them is missing...
		return KeyManager::VERIFY_INVALID_PARAMS;
	}
	KeyManager::KeyData_t keyX_data, keyY_data;

	// Load KeyX.
	if (keyX_verify) {
		res = keyManager->getAndVerify(
			keyX_name, &keyX_data,
			keyX_verify, 16);
	} else {
		res = keyManager->get(keyX_name, &keyX_data);
	}

	if (res != KeyManager::VERIFY_OK || keyX_data.length != 16) {
		// Error loading KeyX.
		return res;
	}

	// Load KeyY.
	if (keyY_verify) {
		res = keyManager->getAndVerify(
			keyY_name, &keyY_data,
			keyY_verify, 16);
	} else {
		res = keyManager->get(keyY_name, &keyY_data);
	}

	if (res != KeyManager::VERIFY_OK || keyY_data.length != 16) {
		// Error loading KeyY.
		return res;
	}

	// Scramble the keys to get KeyNormal.
	int ret = CtrKeyScrambler::CtrScramble(pKeyOut,
		reinterpret_cast<const u128_t*>(keyX_data.key),
		reinterpret_cast<const u128_t*>(keyY_data.key));
	// TODO: Scrambling-specific error?
	if (ret != 0) {
		return KeyManager::VERIFY_KEY_INVALID;
	}

	if (keyNormal_verify) {
		// Verify the generated Normal key.
		// TODO: Make this a function in KeyManager, and share it
		// with KeyManager::getAndVerify().
		unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
		if (!cipher) {
			// Unable to create the IAesCipher.
			return KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		}
		// Set cipher parameters.
		ret = cipher->setChainingMode(IAesCipher::CM_ECB);
		if (ret != 0) {
			return KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		}
		ret = cipher->setKey(pKeyOut->u8, sizeof(*pKeyOut));
		if (ret != 0) {
			return KeyManager::VERFIY_IAESCIPHER_INIT_ERR;
		}

		// Decrypt the test data.
		// NOTE: IAesCipher decrypts in place, so we need to
		// make a temporary copy.
		unique_ptr<uint8_t[]> tmpData(new uint8_t[16]);
		memcpy(tmpData.get(), keyNormal_verify, 16);
		unsigned int size = cipher->decrypt(tmpData.get(), 16);
		if (size != 16) {
			// Decryption failed.
			return KeyManager::VERIFY_IAESCIPHER_DECRYPT_ERR;
		}

		// Verify the test data.
		if (memcmp(tmpData.get(), KeyManager::verifyTestString, 16) != 0) {
			// Verification failed.
			return KeyManager::VERIFY_WRONG_KEY;
		}
	}

	return (ret == 0 ? KeyManager::VERIFY_OK : KeyManager::VERIFY_KEY_INVALID);
}

/**
 * Verification data for debug Slot0x3DKeyX.
 * This is the string "AES-128-ECB-TEST"
 * encrypted using the key with AES-128-ECB.
 */
const uint8_t NCCHReaderPrivate::verifyData_ctr_dev_Slot0x3DKeyX[16] = {
	0x1A,0x62,0xA4,0x97,0x8F,0xBF,0xC0,0x86,
	0x06,0x2F,0x0F,0x1A,0x14,0x7E,0x9F,0xFE
};

/**
 * Verification data for Slot0x3DKeyY.
 * This is the string "AES-128-ECB-TEST"
 * encrypted using the key with AES-128-ECB.
 * Primary index is ticket->keyY_index.
 */
// TODO: Test before committing.
// TODO: Split encryption keys into a separate file,
// and maybe add some sort of interface to allow
// verifying keys.conf separately?
// keyNames(): Return a NULL-terminated array of key names.
// verifyKey(int): Verify the key at the given index.
const uint8_t NCCHReaderPrivate::verifyData_ctr_dev_Slot0x3DKeyY_tbl[6][16] = {
	// 0: eShop titles
	{0xE9,0x5D,0xBF,0x7F,0x91,0x63,0x5D,0x01,
	 0xF9,0x09,0x75,0x83,0x5C,0x86,0xAA,0x0C},
	// 1: System titles
	{0x02,0x4C,0x56,0x86,0x7A,0x37,0x17,0x04,
	 0x5B,0x86,0xE8,0x28,0xA6,0xEF,0x65,0x62},
	// 2
	{0xEC,0xFC,0x82,0x99,0xD4,0xD1,0x85,0x36,
	 0x43,0xC3,0xA9,0x3C,0x80,0x53,0xCF,0xF0},
	// 3
	{0x76,0x7C,0x02,0x8D,0xF0,0xE6,0xDA,0xCC,
	 0x54,0xC7,0xA7,0x21,0x9E,0xFF,0xAC,0xE0},
	// 4
	{0xC7,0xD2,0xD1,0x20,0xEB,0xE2,0xF8,0x3C,
	 0x76,0xDF,0xF6,0x32,0x8F,0x74,0xE8,0x94},
	// 5
	{0x9A,0x91,0x0F,0x20,0x06,0x22,0xE0,0x50,
	 0x80,0x2A,0xE1,0xA4,0x96,0x7D,0x2E,0x56},
};

/**
 * Verification data for Slot0x3DKeyNormal.
 * This is the string "AES-128-ECB-TEST"
 * encrypted using the key with AES-128-ECB.
 * Primary index is ticket->keyY_index.
 */
// TODO: Test before committing.
const uint8_t NCCHReaderPrivate::verifyData_ctr_dev_Slot0x3DKeyNormal_tbl[6][16] = {
	// 0: eShop titles
	{0x80,0x7E,0x4C,0x05,0x35,0x3F,0x4B,0x35,
	 0x5C,0xC3,0x96,0x0F,0x3F,0x26,0xD0,0xC1},
	// 1: System titles
	{0x74,0x57,0xB2,0x65,0xA8,0x4F,0x35,0xF0,
	 0x91,0x4F,0x76,0xD9,0x94,0x1E,0x80,0x5C},
	// 2
	{0x8A,0xD6,0xCA,0x13,0x5C,0x58,0xF8,0x71,
	 0x10,0xF0,0x72,0xB0,0x63,0x9B,0x4D,0xED},
	// 3
	{0x38,0xF6,0xD3,0x1D,0x18,0xF5,0x28,0xA9,
	 0x97,0x90,0x66,0xCC,0xD3,0x1C,0x09,0xC1},
	// 4
	{0x3A,0x59,0x0D,0x35,0x11,0x92,0x83,0x96,
	 0x33,0x4F,0xFF,0xBF,0x10,0x9C,0x9D,0xC4},
	// 5
	{0xAA,0xDA,0x4C,0xA8,0xF6,0xE5,0xA9,0x77,
	 0xE0,0xA0,0xF9,0xE4,0x76,0xCF,0x0D,0x63},
};

}

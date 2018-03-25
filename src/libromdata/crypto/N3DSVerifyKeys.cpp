/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * N3DSVerifyKeys.cpp: Nintendo 3DS key verification data.                 *
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

#include "N3DSVerifyKeys.hpp"

// librpbase
#include "librpbase/crypto/IAesCipher.hpp"
#include "librpbase/crypto/AesCipherFactory.hpp"
using LibRpBase::IAesCipher;
using LibRpBase::AesCipherFactory;
using LibRpBase::KeyManager;

// libromdata
#include "CtrKeyScrambler.hpp"

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
KeyManager::VerifyResult N3DSVerifyKeys::loadKeyNormal(u128_t *pKeyOut,
	const char *keyNormal_name,
	const char *keyX_name,
	const char *keyY_name,
	const uint8_t *keyNormal_verify,
	const uint8_t *keyX_verify,
	const uint8_t *keyY_verify)
{
	assert(pKeyOut);
	if (!pKeyOut) {
		// Invalid parameters.
		return KeyManager::VERIFY_INVALID_PARAMS;
	}

	// Get the Key Manager instance.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager) {
		// TODO: Some other error?
		return KeyManager::VERIFY_KEY_DB_ERROR;
	}

	// Attempt to load the Normal key first.
	KeyManager::VerifyResult res;
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
		size_t size = cipher->decrypt(tmpData.get(), 16);
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
 * Generate an AES normal key from a KeyX and an NCCH signature.
 * KeyX will be selected based on ncchflags[3].
 * The first 16 bytes of the NCCH signature is used as KeyY.
 *
 * NOTE: If the NCCH uses NoCrypto, this function will return
 * an error, since there's no keys that would work for it.
 * Check for NoCrypto before calling this function.
 *
 * TODO: SEED encryption is not supported, though it isn't needed
 * for "exefs:/icon" and "exefs:/banner".
 *
 * @param pKeyOut		[out] Output key data. (array of 2 keys)
 * @param pNcchHeader		[in] NCCH header, with signature.
 * @param issuer		[in] Issuer type. (N3DS_Ticket_TitleKey_KeyY)
 *                                   If unknown, will try Debug, then Retail.
 * @return VerifyResult.
 */
KeyManager::VerifyResult N3DSVerifyKeys::loadNCCHKeys(u128_t pKeyOut[2],
	const N3DS_NCCH_Header_t *pNcchHeader, uint8_t issuer)
{
	KeyManager::VerifyResult res;

	assert(pKeyOut != nullptr);
	assert(pNcchHeader != nullptr);
	if (!pKeyOut || !pNcchHeader) {
		// Invalid parameters.
		return KeyManager::VERIFY_INVALID_PARAMS;
	}

	// Initialize the Key Manager.
	KeyManager *keyManager = KeyManager::instance();

	// Determine the keyset to use.
	// TODO: If issuer is unknown, try Debug, then Retail.
	const bool isDebug = ((issuer & N3DS_TICKET_TITLEKEY_ISSUER_MASK) == N3DS_TICKET_TITLEKEY_ISSUER_DEBUG);

	// KeyX array.
	// - 0: Standard keyslot. (0x2C) Always used for "exefs:/icon" and "exefs:/banner".
	// - 1: Secondary keyslot. If nullptr, same as 0.
	const char *keyX_name[2] = {nullptr, nullptr};
	const uint8_t *keyX_verify[2] = {nullptr, nullptr};	// TODO

	bool isFixedKey = false;
	if (pNcchHeader->hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_NoCrypto) {
		// No encryption.
		// Zero the key anyway.
		memset(pKeyOut, 0, sizeof(*pKeyOut) * 2);
		return KeyManager::VERIFY_OK;
	} else if (pNcchHeader->hdr.flags[N3DS_NCCH_FLAG_BIT_MASKS] & N3DS_NCCH_BIT_MASK_FixedCryptoKey) {
		// Fixed key.
		if (le32_to_cpu(pNcchHeader->hdr.program_id.hi) & 0x10) {
			// Using the fixed debug key.
			// TODO: Is there a retail equivalent?
			keyX_name[0] = EncryptionKeyNames[Key_Debug_FixedCryptoKey];
			keyX_verify[0] = EncryptionKeyVerifyData[Key_Debug_FixedCryptoKey];
			isFixedKey = true;
		} else {
			// Zero-key.
			memset(pKeyOut, 0, sizeof(*pKeyOut) * 2);
			return KeyManager::VERIFY_OK;
		}
	} else {
		// Regular NCCH encryption.

		// Standard keyslot. (0x2C)
		if (isDebug) {
			keyX_name[0] = EncryptionKeyNames[Key_Debug_Slot0x2CKeyX];
			keyX_verify[0] = EncryptionKeyVerifyData[Key_Debug_Slot0x2CKeyX];
		} else {
			keyX_name[0] = EncryptionKeyNames[Key_Retail_Slot0x2CKeyX];
			keyX_verify[0] = EncryptionKeyVerifyData[Key_Retail_Slot0x2CKeyX];
		}

		// Check for a secondary keyslot.
		// TODO: Handle SEED encryption? (Not needed for "exefs:/icon" and "exefs:/banner".)
		int keyIdx;
		switch (pNcchHeader->hdr.flags[N3DS_NCCH_FLAG_CRYPTO_METHOD]) {
			case 0x00:
				// Standard (0x2C)
				// NOTE: Leave as nullptr, since we don't
				// need to load it twice.
				keyIdx = -1;
				break;
			case 0x01:
				// v7.x (0x25)
				keyIdx = (isDebug ? Key_Debug_Slot0x25KeyX : Key_Retail_Slot0x25KeyX);
				break;
			case 0x0A:
				// Secure3 (0x18)
				keyIdx = (isDebug ? Key_Debug_Slot0x18KeyX : Key_Retail_Slot0x18KeyX);
				break;
			case 0x0B:
				// Secure4 (0x1B)
				keyIdx = (isDebug ? Key_Debug_Slot0x1BKeyX : Key_Retail_Slot0x1BKeyX);
				break;
			default:
				// TODO: Unknown encryption method...
				// TODO: Better error code.
				assert(!"Unknown NCCH encryption method.");
				return KeyManager::VERIFY_WRONG_KEY;
		}

		if (keyIdx >= 0) {
			keyX_name[1] = EncryptionKeyNames[keyIdx];
			keyX_verify[1] = EncryptionKeyVerifyData[keyIdx];
		}
	}

	// FIXME: Allowing a missing secondary key for now,
	// since only the primary key is needed for headers.
	// Need to return an appropriate error in this case.

	// Load the two KeyX keys.
	KeyManager::KeyData_t keyX_data[2] = {{nullptr, 0}, {nullptr, 0}};
	for (int i = 0; i < 2; i++) {
		if (!keyX_name[i]) {
			// KeyX[1] is the same as KeyX[0];
			break;
		}

		if (keyX_verify[i]) {
			res = keyManager->getAndVerify(
				keyX_name[i], &keyX_data[i],
				keyX_verify[i], 16);
		} else {
			res = keyManager->get(keyX_name[i], &keyX_data[i]);
		}

		if (res != KeyManager::VERIFY_OK) {
			// KeyX error.
			if (i == 0)
				return res;
			// Secondary key. Ignore errors for now.
			memset(&keyX_data[i], 0, sizeof(keyX_data[i]));
			keyX_name[i] = nullptr;
		} else if (keyX_data[i].length != 16) {
			// KeyX is the wrong length.
			return KeyManager::VERIFY_KEY_INVALID;
		}
	}

	// If this is a fixed key, then we actually loaded
	// KeyNormal, not KeyX. Return immediately.
	if (isFixedKey) {
		// Copy the keys.
		assert(keyX_data[0].key != nullptr);
		if (!keyX_data[0].key) {
			// Should not happen...
			return KeyManager::VERIFY_KEY_DB_ERROR;
		}
		const int idx2 = (keyX_name[1] ? 1 : 0);
		memcpy(pKeyOut[0].u8, keyX_data[0].key, 16);
		memcpy(pKeyOut[1].u8, keyX_data[idx2].key, 16);
		return KeyManager::VERIFY_OK;
	}

	// Scramble the primary keyslot to get KeyNormal.
	int ret = CtrKeyScrambler::CtrScramble(&pKeyOut[0],
		reinterpret_cast<const u128_t*>(keyX_data[0].key),
		reinterpret_cast<const u128_t*>(pNcchHeader->signature));
	// TODO: Scrambling-specific error?
	if (ret != 0) {
		return KeyManager::VERIFY_KEY_INVALID;
	}

	// Do we have a secondary key?
	if (keyX_name[1]) {
		// Scramble the secondary keyslot to get KeyNormal.
		ret = CtrKeyScrambler::CtrScramble(&pKeyOut[1],
			reinterpret_cast<const u128_t*>(keyX_data[1].key),
			reinterpret_cast<const u128_t*>(pNcchHeader->signature));
		// TODO: Scrambling-specific error?
		if (ret != 0) {
			// FIXME: Ignoring errors for secondary keys for now.
			//return KeyManager::VERIFY_KEY_INVALID;
			memset(&pKeyOut[1], 0, sizeof(pKeyOut[1]));
		}
	} else {
		// Copy ncchKey0 to ncchKey1.
		memcpy(pKeyOut[1].u8, pKeyOut[0].u8, sizeof(pKeyOut[1].u8));
	}

	// NCCH keys generated.
	return KeyManager::VERIFY_OK;
}

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int N3DSVerifyKeys::encryptionKeyCount_static(void)
{
	return Key_Max;
}

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *N3DSVerifyKeys::encryptionKeyName_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return EncryptionKeyNames[keyIdx];
}

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *N3DSVerifyKeys::encryptionVerifyData_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < Key_Max);
	if (keyIdx < 0 || keyIdx >= Key_Max)
		return nullptr;
	return EncryptionKeyVerifyData[keyIdx];
}

// Verification key names.
const char *const N3DSVerifyKeys::EncryptionKeyNames[Key_Max] = {
	// Retail
	"ctr-Slot0x18KeyX",
	"ctr-Slot0x1BKeyX",
	"ctr-Slot0x25KeyX",
	"ctr-Slot0x2CKeyX",
	"ctr-Slot0x3DKeyX",
	"ctr-Slot0x3DKeyY-0",
	"ctr-Slot0x3DKeyY-1",
	"ctr-Slot0x3DKeyY-2",
	"ctr-Slot0x3DKeyY-3",
	"ctr-Slot0x3DKeyY-4",
	"ctr-Slot0x3DKeyY-5",
	"ctr-Slot0x3DKeyNormal-0",
	"ctr-Slot0x3DKeyNormal-1",
	"ctr-Slot0x3DKeyNormal-2",
	"ctr-Slot0x3DKeyNormal-3",
	"ctr-Slot0x3DKeyNormal-4",
	"ctr-Slot0x3DKeyNormal-5",

	// Debug
	"ctr-dev-FixedCryptoKey",
	"ctr-dev-Slot0x18KeyX",
	"ctr-dev-Slot0x1BKeyX",
	"ctr-dev-Slot0x25KeyX",
	"ctr-dev-Slot0x2CKeyX",
	"ctr-dev-Slot0x3DKeyX",
	"ctr-dev-Slot0x3DKeyY-0",
	"ctr-dev-Slot0x3DKeyY-1",
	"ctr-dev-Slot0x3DKeyY-2",
	"ctr-dev-Slot0x3DKeyY-3",
	"ctr-dev-Slot0x3DKeyY-4",
	"ctr-dev-Slot0x3DKeyY-5",
	"ctr-dev-Slot0x3DKeyNormal-0",
	"ctr-dev-Slot0x3DKeyNormal-1",
	"ctr-dev-Slot0x3DKeyNormal-2",
	"ctr-dev-Slot0x3DKeyNormal-3",
	"ctr-dev-Slot0x3DKeyNormal-4",
	"ctr-dev-Slot0x3DKeyNormal-5",
};

// Verification key data.
const uint8_t N3DSVerifyKeys::EncryptionKeyVerifyData[Key_Max][16] = {
	/** Retail **/

	// Key_Retail_Slot0x18KeyX
	{0xE6,0x2E,0x52,0x4A,0x3A,0x17,0x28,0xC8,
	 0xC0,0xFA,0x0C,0x3D,0x74,0x5D,0x74,0x41},

	// Key_Retail_Slot0x1BKeyX
	{0x8E,0x9D,0x8E,0xE5,0x10,0x31,0xF9,0x3C,
	 0x7C,0x77,0x13,0x91,0x33,0xC4,0xE3,0x45},

	// Key_Retail_Slot0x25KeyX
	{0x23,0x81,0x94,0x9A,0x56,0xC9,0xEC,0x25,
	 0xE1,0xA8,0xC7,0x52,0x49,0xE6,0x58,0x25},

	// Key_Retail_Slot0x2CKeyX
	{0x76,0x2D,0xA6,0x8D,0xD2,0xB5,0xC8,0xBB,
	 0xCB,0x43,0x0E,0x9B,0xC0,0x60,0x56,0x1E},

	// Key_Retail_Slot0x3DKeyX
	{0x7D,0xD5,0x76,0x3A,0x97,0x89,0xED,0xD9,
	 0x17,0x73,0x00,0x2A,0xA6,0xA5,0x3A,0x92},

	// Key_Retail_Slot0x3DKeyY
	// 0: eShop titles
	{0x0E,0xBD,0x7C,0x95,0x9B,0x76,0x23,0x46,
	 0xD7,0xF9,0xF0,0x0C,0x13,0xD9,0xA8,0x43},
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

	// Key_Retail_Slot0x3DKeyNormal
	// 0: eShop titles
	{0xD2,0x8B,0x76,0x6A,0xFD,0xD7,0x6F,0xC9,
	 0xB3,0x45,0xE8,0xA9,0x19,0x57,0x20,0x2E},
	// 1: System titles
	{0x8B,0xE8,0x86,0x10,0x44,0x88,0x93,0xC7,
	 0xE5,0x1E,0x75,0xF7,0x5F,0xD5,0x7F,0x54},
	// 2
	{0xCA,0x70,0x4D,0x49,0x3B,0x20,0x60,0xE3,
	 0xE6,0x07,0x98,0x75,0x4A,0xD6,0x9B,0x6D},
	// 3
	{0x49,0x2D,0xCA,0xD6,0x74,0xA0,0x03,0x69,
	 0x08,0x83,0x01,0x86,0x4B,0x2A,0xEC,0x67},
	// 4
	{0x4D,0x56,0x78,0x6B,0xC0,0x7C,0x16,0x65,
	 0x10,0x8D,0xF9,0x6D,0x56,0x24,0xBB,0x6E},
	// 5
	{0x58,0xFC,0x29,0xA7,0x26,0x2E,0x16,0x32,
	 0x92,0xF6,0x60,0x5A,0x93,0x0B,0x17,0x2E},

	/** Debug **/

	// Key_Debug_FixedCryptoKey
	{0x1E,0x95,0x82,0xCD,0x65,0x2A,0xE3,0x3F,
	 0x90,0xEB,0x91,0x3F,0x77,0xE0,0x0A,0x35},

	// Key_Debug_Slot0x18KeyX
	{0xF8,0x66,0x09,0x3A,0x7C,0x81,0x64,0x41,
	 0x14,0x17,0x43,0x5C,0xCD,0xA7,0xED,0x1B},

	// Key_Debug_Slot0x1BKeyX,
	{0x04,0x86,0xC2,0x87,0x60,0xE2,0x24,0x93,
	 0xAF,0x9D,0xF5,0x15,0x22,0x5A,0x09,0x2B},

	// Key_Debug_Slot0x25KeyX
	{0x81,0x01,0x31,0xFD,0xDC,0x08,0x9E,0x7D,
	 0x56,0xC9,0x62,0x37,0xAE,0x33,0x26,0xEE},

	// Key_Debug_Slot0x2CKeyX
	{0xB3,0xB7,0x34,0x02,0xF6,0xE0,0x6A,0x0B,
	 0xFB,0x51,0xED,0xFC,0x19,0x3B,0x4A,0x04},

	// Key_Debug_Slot0x3DKeyX
	{0x1A,0x62,0xA4,0x97,0x8F,0xBF,0xC0,0x86,
	 0x06,0x2F,0x0F,0x1A,0x14,0x7E,0x9F,0xFE},

	// Key_Debug_Slot0x3DKeyY
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

	// Key_Debug_Slot0x3DKeyNormal
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
	{0xF1,0x93,0x91,0x6D,0x05,0x27,0x91,0xBD,
	 0x6A,0x80,0x98,0x59,0x7B,0x16,0xD6,0x9C},
};

}

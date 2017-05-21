/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * KeyManager.cpp: Encryption key manager.                                 *
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

#include "librpbase/config.librpbase.h"

#include "KeyManager.hpp"
#include "config/ConfReader_p.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
using std::string;
using std::unique_ptr;
using std::unordered_map;

#include "IAesCipher.hpp"
#include "AesCipherFactory.hpp"

// Text conversion functions and macros.
#include "TextFuncs.hpp"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "../uvector.h"

namespace LibRpBase {

class KeyManagerPrivate : public ConfReaderPrivate
{
	public:
		KeyManagerPrivate();

	private:
		typedef ConfReaderPrivate super;
		RP_DISABLE_COPY(KeyManagerPrivate)

#ifdef ENABLE_DECRYPTION
	public:
		// Static KeyManager instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static KeyManager instance;
#endif /* ENABLE_DECRYPTION */

	public:
		/**
		 * Reset the configuration to the default values.
		 */
		void reset(void) override final;

		/**
		 * Process a configuration line.
		 * Virtual function; must be reimplemented by subclasses.
		 *
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		int processConfigLine(const char *section,
			const char *name, const char *value) override final;

	public:
#ifdef ENABLE_DECRYPTION
		// Encryption key data.
		// Managed as a single block in order to reduce
		// memory allocations.
		ao::uvector<uint8_t> vKeys;

		/**
		 * Map of key names to vKeys indexes.
		 * - Key: Key name.
		 * - Value: vKeys information.
		 *   - High byte: Key length.
		 *   - Low 3 bytes: Key index.
		 */
		unordered_map<string, uint32_t> mapKeyNames;

		/**
		 * Map of invalid key names to errors.
		 * These are stored for better error reporting.
		 * - Key: Key name.
		 * - Value: Verification result.
		 */
		unordered_map<string, uint8_t> mapInvalidKeyNames;
#endif /* ENABLE_DECRYPTION */
};

/** KeyManagerPrivate **/

#ifdef ENABLE_DECRYPTION
// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
KeyManager KeyManagerPrivate::instance;

// Verification test string.
// NOTE: This string is NOT NULL-terminated!
const char KeyManager::verifyTestString[] = {
	'A','E','S','-','1','2','8','-',
	'E','C','B','-','T','E','S','T'
};
#endif /* ENABLE_DECRYPTION */

KeyManagerPrivate::KeyManagerPrivate()
	: super(_RP("keys.conf"))
{ }

/**
 * Reset the configuration to the default values.
 */
void KeyManagerPrivate::reset(void)
{
#ifdef ENABLE_DECRYPTION
	// Clear the loaded keys.
	vKeys.clear();
	mapKeyNames.clear();
	mapInvalidKeyNames.clear();

	// Reserve 1 KB for the key store.
	vKeys.reserve(1024);
#if !defined(_MSC_VER) || _MSC_VER >= 1700
	// Reserve entries for the key names map.
	// NOTE: Not reserving entries for invalid key names.
	mapKeyNames.reserve(64);
#endif
#else /* !ENABLE_DECRYPTION */
	assert(!"Should not be called in no-decryption builds.");
#endif /* ENABLE_DECRYPTION */
}

/**
 * Process a configuration line.
 * Virtual function; must be reimplemented by subclasses.
 *
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 on success; 0 on error.
 */
int KeyManagerPrivate::processConfigLine(const char *section, const char *name, const char *value)
{
#ifdef ENABLE_DECRYPTION
	// NOTE: Invalid lines are ignored, so we're always returning 1.

	// Are we in the "Keys" section?
	if (!section || strcasecmp(section, "Keys") != 0) {
		// Not in the "Keys" section.
		return 1;
	}

	// Are the key name and/or value empty?
	if (!name || name[0] == 0) {
		// Empty key name.
		return 1;
	}

	// Is the value empty?
	if (!value || value[0] == 0) {
		// Value is empty.
		mapInvalidKeyNames.insert(std::make_pair(string(name), KeyManager::VERIFY_KEY_INVALID));
		return 1;
	}

	// Check the value length.
	int value_len = (int)strlen(value);
	if ((value_len % 2) != 0) {
		// Value is an odd length, which is invalid.
		// This means we have an extra nybble.
		mapInvalidKeyNames.insert(std::make_pair(string(name), KeyManager::VERIFY_KEY_INVALID));
		return 1;
	}
	const uint8_t len = (uint8_t)(value_len / 2);

	// Parse the value.
	unsigned int vKeys_start_pos = (unsigned int)vKeys.size();
	unsigned int vKeys_pos = vKeys_start_pos;
	// Reserve space for half of the key string.
	// Key string is ASCII hex, so two characters make up one byte.
	vKeys.resize(vKeys.size() + len);

	// ASCII to HEX lookup table.
	// Reference: http://codereview.stackexchange.com/questions/22757/char-hex-string-to-byte-array
	static const uint8_t ascii_to_hex[0x100] = {
		//0     1     2     3     4     5     6    7      8     9     A     B     C     D     E     F
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//1
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//2
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//3
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//4
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//5
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//6
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//7

		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//8
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//9
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//A
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//B
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//C
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//D
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//E
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF //F
	};

	for (; value_len > 0; value_len -= 2, vKeys_pos++, value += 2) {
		// Process two characters at a time.
		// Two hexadecimal digits == one byte.
		char chr0 = ascii_to_hex[(uint8_t)value[0]];
		char chr1 = ascii_to_hex[(uint8_t)value[1]];
		if (chr0 > 0x0F || chr1 > 0x0F) {
			// Invalid character.
			vKeys.resize(vKeys_start_pos);
			return 1;
		}

		vKeys[vKeys_pos] = (chr0 << 4 | chr1);
	}

	// Value parsed successfully.
	uint32_t keyIdx = vKeys_start_pos;
	keyIdx |= (len << 24);
	mapKeyNames.insert(std::make_pair(string(name), keyIdx));
	return 1;
#else /* !ENABLE_DECRYPTION */
	assert(!"Should not be called in no-decryption builds.");
	return 0;
#endif /* ENABLE_DECRYPTION */
}

/** KeyManager **/

KeyManager::KeyManager()
	: super(new KeyManagerPrivate())
{ }

/**
 * Get a description for a VerifyResult.
 * @param res VerifyResult.
 * @return Description, or nullptr if invalid.
 */
const rp_char *KeyManager::verifyResultToString(VerifyResult res)
{
	static const rp_char *const errTbl[] = {
		// VERIFY_OK
		_RP("Something happened."),
		// VERIFY_INVALID_PARAMS
		_RP("Invalid parameters. (THIS IS A BUG!)"),
		// VERIFY_NO_SUPPORT
		_RP("Decryption is not supported in this build."),
		// VERIFY_KEY_DB_NOT_LOADED
		_RP("keys.conf was not found."),
		// VERIFY_KEY_DB_ERROR
		_RP("keys.conf has an error and could not be loaded."),
		// VERIFY_KEY_NOT_FOUND
		_RP("Required key was not found in keys.conf."),
		// VERIFY_KEY_INVALID
		_RP("The key in keys.conf is not a valid key."),
		// VERFIY_IAESCIPHER_INIT_ERR
		_RP("AES decryption could not be initialized."),
		// VERIFY_IAESCIPHER_DECRYPT_ERR
		_RP("AES decryption failed."),
		// VERIFY_WRONG_KEY
		_RP("The key in keys.conf is incorrect."),
	};
	static_assert(ARRAY_SIZE(errTbl) == KeyManager::VERIFY_MAX, "Update errTbl[].");

	assert(res >= 0);
	assert(res < ARRAY_SIZE(errTbl));
	return ((res >= 0 && res < ARRAY_SIZE(errTbl)) ? errTbl[res] : nullptr);
}

#ifdef ENABLE_DECRYPTION
/**
 * Get the KeyManager instance.
 * @return KeyManager instance.
 */
KeyManager *KeyManager::instance(void)
{
	// Return the singleton instance.
	return &KeyManagerPrivate::instance;
}

/**
 * Get an encryption key.
 * @param keyName	[in]  Encryption key name.
 * @param pKeyData	[out] Key data struct.
 * @return VerifyResult.
 */
KeyManager::VerifyResult KeyManager::get(const char *keyName, KeyData_t *pKeyData) const
{
	assert(keyName != nullptr);
	assert(keyName[0] != 0);
	if (!keyName || keyName[0] == 0) {
		// Invalid parameters.
		return VERIFY_INVALID_PARAMS;
	}

	// Check if keys.conf needs to be reloaded.
	// This function won't do anything if the keys
	// have already been loaded and keys.conf hasn't
	// been changed.
	const_cast<KeyManager*>(this)->load();
	if (!isLoaded()) {
		// Keys are not loaded.
		return VERIFY_KEY_DB_NOT_LOADED;
	}

	// Attempt to get the key from the map.
	RP_D(const KeyManager);
	auto iter = d->mapKeyNames.find(keyName);
	if (iter == d->mapKeyNames.end()) {
		// Key was not parsed. Figure out why.
		auto iter = d->mapInvalidKeyNames.find(keyName);
		if (iter != d->mapInvalidKeyNames.end()) {
			// An error occurred when parsing the key.
			return (VerifyResult)iter->second;
		}

		// Key was not found.
		return VERIFY_KEY_NOT_FOUND;
	}

	// Found the key.
	const uint32_t keyIdx = iter->second;
	const uint32_t idx = (keyIdx & 0xFFFFFF);
	const uint8_t len = ((keyIdx >> 24) & 0xFF);

	// Make sure the key index is valid.
	assert(idx + len <= d->vKeys.size());
	if (idx + len > d->vKeys.size()) {
		// Should not happen...
		return VERIFY_KEY_DB_ERROR;
	}

	if (pKeyData) {
		pKeyData->key = d->vKeys.data() + idx;
		pKeyData->length = len;
	}
	return VERIFY_OK;
}

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
 * @param pKeyData	[out] Key data struct.
 * @param pVerifyData	[in] Verification data block.
 * @param verifyLen	[in] Length of pVerifyData. (Must be 16.)
 * @return VerifyResult.
 */
KeyManager::VerifyResult KeyManager::getAndVerify(const char *keyName, KeyData_t *pKeyData,
	const uint8_t *pVerifyData, unsigned int verifyLen) const
{
	assert(keyName);
	assert(pVerifyData);
	assert(verifyLen == 16);
	if (!keyName || !pVerifyData || verifyLen != 16) {
		// Invalid parameters.
		return VERIFY_INVALID_PARAMS;
	}

	// Temporary KeyData_t in case pKeyData is nullptr.
	KeyData_t tmp_key_data;
	if (!pKeyData) {
		pKeyData = &tmp_key_data;
	}

	// Get the key first.
	VerifyResult res = get(keyName, pKeyData);
	if (res != VERIFY_OK) {
		// Error obtaining the key.
		return res;
	} else if (!pKeyData->key || pKeyData->length == 0) {
		// Key is invalid.
		return VERIFY_KEY_INVALID;
	}

	// Verify the key length.
	if (pKeyData->length != 16 && pKeyData->length != 24 && pKeyData->length != 32) {
		// Key length is invalid.
		return VERIFY_KEY_INVALID;
	}

	// Decrypt the test data.
	// TODO: Keep this IAesCipher instance around?
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher) {
		// Unable to create the IAesCipher.
		return VERFIY_IAESCIPHER_INIT_ERR;
	}

	// Set cipher parameters.
	int ret = cipher->setChainingMode(IAesCipher::CM_ECB);
	if (ret != 0) {
		return VERFIY_IAESCIPHER_INIT_ERR;
	}
	ret = cipher->setKey(pKeyData->key, pKeyData->length);
	if (ret != 0) {
		return VERFIY_IAESCIPHER_INIT_ERR;
	}

	// Decrypt the test data.
	// NOTE: IAesCipher decrypts in place, so we need to
	// make a temporary copy.
	unique_ptr<uint8_t[]> tmpData(new uint8_t[verifyLen]);
	memcpy(tmpData.get(), pVerifyData, verifyLen);
	unsigned int size = cipher->decrypt(tmpData.get(), verifyLen);
	if (size != verifyLen) {
		// Decryption failed.
		return VERIFY_IAESCIPHER_DECRYPT_ERR;
	}

	// Verify the test data.
	if (memcmp(tmpData.get(), verifyTestString, verifyLen) != 0) {
		// Verification failed.
		return VERIFY_WRONG_KEY;
	}

	// Test data verified.
	return VERIFY_OK;
}
#endif /* ENABLE_DECRYPTION */

}

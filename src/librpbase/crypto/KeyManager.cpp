/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * KeyManager.cpp: Encryption key manager.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "KeyManager.hpp"
#include "config/ConfReader_p.hpp"
#include "libi18n/i18n.h"

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::unordered_map;

#include "IAesCipher.hpp"
#include "AesCipherFactory.hpp"

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
		void reset(void) final;

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
			const char *name, const char *value) final;

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
	: super("keys.conf")
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
#ifdef HAVE_UNORDERED_MAP_RESERVE
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
		// Treat it as if the key wasn't found.
		return 1;
	}

	// Check the value length.
	// TODO: Check for <= 0?
	const size_t value_len = strlen(value);
	if (value_len > 255) {
		// Key is long.
		return 1;
	}

	const bool is_odd_len = ((value_len % 2) != 0);
	uint8_t len = static_cast<uint8_t>(value_len / 2);

	// Parse the value.
	const uint32_t vKeys_start_pos = static_cast<uint32_t>(vKeys.size());
	const uint32_t vKeys_pos = vKeys_start_pos;
	// Reserve space for half of the key string.
	// Key string is ASCII hex, so two characters make up one byte.
	vKeys.resize(vKeys.size() + len);
	int ret = KeyManager::hexStringToBytes(value, &vKeys[vKeys_pos], len);
	if (ret != 0) {
		// Invalid character(s) encountered.
		vKeys.resize(vKeys_start_pos);
		return 1;
	}
	if (is_odd_len) {
		// Odd length. Parse the last nybble and append a 0.
		// This is better than simply discarding it entirely.
		char buf[2];
		buf[0] = value[value_len-1];
		buf[1] = '0';
		vKeys.resize(vKeys.size() + 2);
		ret = KeyManager::hexStringToBytes(buf, &vKeys[vKeys_pos+len], 1);
		if (ret != 0) {
			// Invalid character(s) encountered.
			vKeys.resize(vKeys_start_pos);
			return 1;
		}
		// Add the extra byte.
		len++;
	}

	// Value parsed successfully.
	uint32_t keyIdx = vKeys_start_pos;
	keyIdx |= (len << 24);
	mapKeyNames.emplace(name, keyIdx);
	return 1;
#else /* !ENABLE_DECRYPTION */
	RP_UNUSED(section);
	RP_UNUSED(name);
	RP_UNUSED(value);
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
const char *KeyManager::verifyResultToString(VerifyResult res)
{
	static const char *const errTbl[] = {
		// tr: VerifyResult::OK
		NOP_C_("KeyManager|VerifyResult", "Something happened."),
		// tr: VerifyResult::InvalidParams
		NOP_C_("KeyManager|VerifyResult", "Invalid parameters. (THIS IS A BUG!)"),
		// tr: VerifyResult::NoSupport
		NOP_C_("KeyManager|VerifyResult", "Decryption is not supported in this build."),
		// tr: VerifyResult::KeyDBNotLoaded
		NOP_C_("KeyManager|VerifyResult", "keys.conf was not found."),
		// tr: VerifyResult::KeyDBError
		NOP_C_("KeyManager|VerifyResult", "keys.conf has an error and could not be loaded."),
		// tr: VerifyResult::KeyNotFound
		NOP_C_("KeyManager|VerifyResult", "Required key was not found in keys.conf."),
		// tr: VerifyResult::KeyInvalid
		NOP_C_("KeyManager|VerifyResult", "The key in keys.conf is not a valid key."),
		// tr: VerifyResult::IAesCipherInitErr
		NOP_C_("KeyManager|VerifyResult", "AES decryption could not be initialized."),
		// tr: VerifyResult::IAesCipherDecryptErr
		NOP_C_("KeyManager|VerifyResult", "AES decryption failed."),
		// tr: VerifyResult::WrongKey
		NOP_C_("KeyManager|VerifyResult", "The key in keys.conf is incorrect."),
		// tr: VerifyResult::IncrementingValues
		NOP_C_("KeyManager|VerifyResult", "The partition contains incrementing values."),
	};
	static_assert(ARRAY_SIZE(errTbl) == (int)KeyManager::VerifyResult::Max, "Update errTbl[].");

	assert(res >= (KeyManager::VerifyResult)0);
	assert(res < (KeyManager::VerifyResult)ARRAY_SIZE(errTbl));
	return ((res >= (KeyManager::VerifyResult)0 && res < (KeyManager::VerifyResult)ARRAY_SIZE(errTbl))
		? dpgettext_expr(RP_I18N_DOMAIN, "KeyManager|VerifyResult", errTbl[(int)res])
		: nullptr);
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
		return VerifyResult::InvalidParams;
	}

	// Check if keys.conf needs to be reloaded.
	// This function won't do anything if the keys
	// have already been loaded and keys.conf hasn't
	// been changed.
	const_cast<KeyManager*>(this)->load();
	if (!isLoaded()) {
		// Keys are not loaded.
		return VerifyResult::KeyDBNotLoaded;
	}

	// Attempt to get the key from the map.
	RP_D(const KeyManager);
	auto iter = d->mapKeyNames.find(keyName);
	if (iter == d->mapKeyNames.end()) {
		// Key was not parsed. Figure out why.
		auto iter2 = d->mapInvalidKeyNames.find(keyName);
		if (iter2 != d->mapInvalidKeyNames.end()) {
			// An error occurred when parsing the key.
			return (VerifyResult)iter2->second;
		}

		// Key was not found.
		return VerifyResult::KeyNotFound;
	}

	// Found the key.
	const uint32_t keyIdx = iter->second;
	const uint32_t idx = (keyIdx & 0xFFFFFF);
	const uint8_t len = ((keyIdx >> 24) & 0xFF);

	// Make sure the key index is valid.
	assert(idx + len <= d->vKeys.size());
	if (idx + len > d->vKeys.size()) {
		// Should not happen...
		return VerifyResult::KeyDBError;
	}

	if (pKeyData) {
		pKeyData->key = d->vKeys.data() + idx;
		pKeyData->length = len;
	}
	return VerifyResult::OK;
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
		return VerifyResult::InvalidParams;
	}

	// Temporary KeyData_t in case pKeyData is nullptr.
	KeyData_t tmp_key_data = {nullptr, 0};
	if (!pKeyData) {
		pKeyData = &tmp_key_data;
	}

	// Get the key first.
	VerifyResult res = get(keyName, pKeyData);
	if (res != VerifyResult::OK) {
		// Error obtaining the key.
		return res;
	} else if (!pKeyData->key || pKeyData->length == 0) {
		// Key is invalid.
		return VerifyResult::KeyInvalid;
	}

	// Verify the key length.
	if (pKeyData->length != 16 && pKeyData->length != 24 && pKeyData->length != 32) {
		// Key length is invalid.
		return VerifyResult::KeyInvalid;
	}

	// Decrypt the test data.
	// TODO: Keep this IAesCipher instance around?
	unique_ptr<IAesCipher> cipher(AesCipherFactory::create());
	if (!cipher) {
		// Unable to create the IAesCipher.
		return VerifyResult::IAesCipherInitErr;
	}

	// Set cipher parameters.
	int ret = cipher->setChainingMode(IAesCipher::ChainingMode::ECB);
	if (ret != 0) {
		return VerifyResult::IAesCipherInitErr;
	}
	ret = cipher->setKey(pKeyData->key, pKeyData->length);
	if (ret != 0) {
		return VerifyResult::IAesCipherInitErr;
	}

	// Decrypt the test data.
	// NOTE: IAesCipher decrypts in place, so we need to
	// make a temporary copy.
	unique_ptr<uint8_t[]> tmpData(new uint8_t[verifyLen]);
	memcpy(tmpData.get(), pVerifyData, verifyLen);
	size_t size = cipher->decrypt(tmpData.get(), verifyLen);
	if (size != verifyLen) {
		// Decryption failed.
		return VerifyResult::IAesCipherDecryptErr;
	}

	// Verify the test data.
	if (memcmp(tmpData.get(), verifyTestString, verifyLen) != 0) {
		// Verification failed.
		return VerifyResult::WrongKey;
	}

	// Test data verified.
	return VerifyResult::OK;
}

/**
 * Convert string data from hexadecimal to bytes.
 * @param str	[in] String data. (Must be len*2 characters.)
 * @param buf	[out] Output buffer.
 * @param len	[in] Size of buf, in bytes.
 * @return 0 on success; non-zero on error.
 */
template<typename Char>
int KeyManager::hexStringToBytes(const Char *str, uint8_t *buf, unsigned int len)
{
	// ASCII to HEX lookup table.
	// Reference: http://codereview.stackexchange.com/questions/22757/char-hex-string-to-byte-array
	static const uint8_t ascii_to_hex[0x80] = {
		//0     1     2     3     4     5     6    7      8     9     A     B     C     D     E     F
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//0
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//1
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//2
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//3
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//4
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//5
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//6
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,//7
	};

	for (; len > 0; len--, str += 2, buf++) {
		// Process two characters at a time.
		// Two hexadecimal digits == one byte.
		if (static_cast<unsigned int>(str[0]) >= 0x80 ||
			static_cast<unsigned int>(str[1]) >= 0x80)
		{
			// Invalid character.
			return -EINVAL;
		}

		const uint8_t chr0 = ascii_to_hex[static_cast<uint8_t>(str[0])];
		const uint8_t chr1 = ascii_to_hex[static_cast<uint8_t>(str[1])];
		if (chr0 > 0x0F || chr1 > 0x0F) {
			// Invalid character.
			return -EINVAL;
		}

		*buf = (chr0 << 4 | chr1);
	}

	// String processed.
	return 0;
}

// Explicit instantiation of hexStringToBytes().
template int KeyManager::hexStringToBytes<char>(const char *str, uint8_t *buf, unsigned int len);

#endif /* ENABLE_DECRYPTION */

}

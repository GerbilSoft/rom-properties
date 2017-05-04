/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
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

#include "config.libromdata.h"
#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif /* ENABLE_DECRYPTION */

#include "KeyManager.hpp"

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

#include "file/FileSystem.hpp"
#include "file/RpFile.hpp"
#include "threads/Atomics.h"
#include "threads/Mutex.hpp"

#include "IAesCipher.hpp"
#include "AesCipherFactory.hpp"

// One-time initialization.
#ifdef _WIN32
#include "../threads/InitOnceExecuteOnceXP.h"
#else
#include <pthread.h>
#endif

// Text conversion functions and macros.
#include "TextFuncs.hpp"
#ifdef _WIN32
#include "../RpWin32.hpp"
#endif

// INI parser.
#include "ini.h"

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "../uvector.h"

namespace LibRomData {

class KeyManagerPrivate
{
	public:
		KeyManagerPrivate();

	private:
		RP_DISABLE_COPY(KeyManagerPrivate)

	public:
		// Static KeyManager instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static KeyManager instance;

	public:
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

		/**
		 * Initialize KeyManager.
		 */
		void init(void);

		/**
		 * Initialize Once function.
		 * Called by pthread_once() or InitOnceExecuteOnce().
		 */
#ifdef _WIN32
		static BOOL WINAPI initOnceFunc(_Inout_ PINIT_ONCE_XP once, _In_ PVOID param, _Out_opt_ LPVOID *context);
#else
		static void initOnceFunc(void);
#endif

		/**
		 * Process a configuration line.
		 *
		 * NOTE: This function is static because it is used as a
		 * C-style callback from inih.
		 *
		 * @param user KeyManagerPrivate object.
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		static int processConfigLine(void *user, const char *section, const char *name, const char *value);

		/**
		 * Load keys from the configuration file.
		 * @param force If true, force a reload, even if the timestamp hasn't changed.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadKeys(bool force = false);

#ifdef _WIN32
		// InitOnceExecuteOnce() control variable.
		INIT_ONCE once_control;
		static const INIT_ONCE ONCE_CONTROL_INIT = INIT_ONCE_STATIC_INIT;
#else
		// pthread_once() control variable.
		pthread_once_t once_control;
		static const pthread_once_t ONCE_CONTROL_INIT = PTHREAD_ONCE_INIT;
#endif

		// loadKeys() mutex.
		Mutex mtxLoadKeys;

		// keys.conf status.
		rp_string conf_filename;
		bool conf_was_found;
		time_t conf_mtime;
};

/** KeyManagerPrivate **/

// Verification test string.
// NOTE: This string is NOT NULL-terminated!
const char KeyManager::verifyTestString[] = {
	'A','E','S','-','1','2','8','-',
	'E','C','B','-','T','E','S','T'
};

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
KeyManager KeyManagerPrivate::instance;

KeyManagerPrivate::KeyManagerPrivate()
	: once_control(ONCE_CONTROL_INIT)
	, conf_was_found(false)
	, conf_mtime(0)
{ }

/**
 * Initialize KeyManager.
 * Called by initOnceFunc().
 */
void KeyManagerPrivate::init(void)
{
	// Reserve 1 KB for the key store.
	vKeys.reserve(1024);

	// Configuration filename.
	conf_filename = FileSystem::getConfigDirectory();
	if (!conf_filename.empty()) {
		if (conf_filename.at(conf_filename.size()-1) != _RP_CHR(DIR_SEP_CHR)) {
			conf_filename += _RP_CHR(DIR_SEP_CHR);
		}
		conf_filename += _RP("keys.conf");
	}

	// Make sure the configuration directory exists.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(conf_filename);
	if (ret != 0) {
		// rmkdir() failed.
		conf_filename.clear();
	}

	// Load the keys.
	loadKeys(true);
}

/**
 * Initialize Once function.
 * Called by pthread_once() or InitOnceExecuteOnce().
 * TODO: Add a pthread_once() wrapper for Windows?
 */
#ifdef _WIN32
BOOL WINAPI KeyManagerPrivate::initOnceFunc(_Inout_ PINIT_ONCE_XP once, _In_ PVOID param, _Out_opt_ LPVOID *context)
#else
void KeyManagerPrivate::initOnceFunc(void)
#endif
{
	instance.d_ptr->init();
#ifdef _WIN32
	return TRUE;
#endif
}

/**
 * Process a configuration line.
 *
 * NOTE: This function is static because it is used as a
 * C-style callback from inih.
 *
 * @param user KeyManagerPrivate object.
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 on success; 0 on error.
 */
int KeyManagerPrivate::processConfigLine(void *user, const char *section, const char *name, const char *value)
{
	KeyManagerPrivate *const d = static_cast<KeyManagerPrivate*>(user);

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
		d->mapInvalidKeyNames.insert(std::make_pair(string(name), KeyManager::VERIFY_KEY_INVALID));
		return 1;
	}

	// Check the value length.
	int value_len = (int)strlen(value);
	if ((value_len % 2) != 0) {
		// Value is an odd length, which is invalid.
		// This means we have an extra nybble.
		d->mapInvalidKeyNames.insert(std::make_pair(string(name), KeyManager::VERIFY_KEY_INVALID));
		return 1;
	}
	const uint8_t len = (uint8_t)(value_len / 2);

	// Parse the value.
	unsigned int vKeys_start_pos = (unsigned int)d->vKeys.size();
	unsigned int vKeys_pos = vKeys_start_pos;
	// Reserve space for half of the key string.
	// Key string is ASCII hex, so two characters make up one byte.
	d->vKeys.resize(d->vKeys.size() + len);

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
			d->vKeys.resize(vKeys_start_pos);
			return 1;
		}

		d->vKeys[vKeys_pos] = (chr0 << 4 | chr1);
	}

	// Value parsed successfully.
	uint32_t keyIdx = vKeys_start_pos;
	keyIdx |= (len << 24);
	d->mapKeyNames.insert(std::make_pair(string(name), keyIdx));
	return 1;
}

/**
 * Load keys from the configuration file.
 * @param force If true, force a reload, even if the timestamp hasn't changed.
 * @return 0 on success; negative POSIX error code on error.
 */
int KeyManagerPrivate::loadKeys(bool force)
{
	if (conf_filename.empty()) {
		// Configuration filename is invalid...
		return -ENOENT;
	}

	if (!force && conf_was_found) {
		// Check if the keys.conf timestamp has changed.
		// Initial check. (fast path)
		time_t mtime;
		int ret = FileSystem::get_mtime(conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// loadKeys() mutex.
	// NOTE: This may result in keys.conf being loaded twice
	// in some cases, but that's better than keys.conf being
	// loaded twice at the same time and causing collisions.
	MutexLocker mtxLocker(mtxLoadKeys);

	if (!force && conf_was_found) {
		// Check if the keys.conf timestamp has changed.
		// NOTE: Second check once the mutex is locked.
		time_t mtime;
		int ret = FileSystem::get_mtime(conf_filename, &mtime);
		if (ret != 0) {
			// Failed to retrieve the mtime.
			// Leave everything as-is.
			// TODO: Proper error code?
			return -EIO;
		}

		if (mtime == conf_mtime) {
			// Timestamp has not changed.
			return 0;
		}
	}

	// Clear the loaded keys.
	vKeys.clear();
	mapKeyNames.clear();
	mapInvalidKeyNames.clear();

	// Parse the configuration file.
	// NOTE: We're using the filename directly, since it's always
	// on the local file system, and it's easier to let inih
	// manage the file itself.
#ifdef _WIN32
	// Win32: Use ini_parse_w().
	int ret = ini_parse_w(RP2W_s(conf_filename), KeyManagerPrivate::processConfigLine, this);
#else /* !_WIN32 */
	// Linux or other systems: Use ini_parse().
	int ret = ini_parse(rp_string_to_utf8(conf_filename).c_str(), KeyManagerPrivate::processConfigLine, this);
#endif /* _WIN32 */
	if (ret != 0) {
		// Error parsing the INI file.
		vKeys.clear();
		mapKeyNames.clear();
		mapInvalidKeyNames.clear();

		if (ret == -2)
			return -ENOMEM;
		return -EIO;
	}

	// Save the mtime from the keys.conf file.
	// TODO: IRpFile::get_mtime()?
	time_t mtime;
	ret = FileSystem::get_mtime(conf_filename, &mtime);
	if (ret == 0) {
		conf_mtime = mtime;
	} else {
		// mtime error...
		// TODO: What do we do here?
		conf_mtime = 0;
	}

	// Keys loaded.
	conf_was_found = true;
	return 0;
}

/** KeyManager **/

KeyManager::KeyManager()
	: d_ptr(new KeyManagerPrivate())
{ }

KeyManager::~KeyManager()
{
	delete d_ptr;
}

/**
 * Get the KeyManager instance.
 * @return KeyManager instance.
 */
KeyManager *KeyManager::instance(void)
{
	// Initialize the singleton instance.
	KeyManager *const q = &KeyManagerPrivate::instance;

#ifdef _WIN32
	// TODO: Handle errors.
	InitOnceExecuteOnce(&q->d_ptr->once_control,
		q->d_ptr->initOnceFunc,
		(PVOID)q, nullptr);
#else
	pthread_once(&q->d_ptr->once_control, q->d_ptr->initOnceFunc);
#endif

	// Singleton instance.
	return &KeyManagerPrivate::instance;
}

/**
 * Have the encryption keys been loaded yet?
 *
 * This function will *not* load the keys.
 * To load the keys, call get() with the requested key name.
 *
 * If this function returns false after calling get(),
 * keys.conf is probably missing.
 *
 * @return True if keys have been loaded; false if not.
 */
bool KeyManager::areKeysLoaded(void) const
{
	RP_D(const KeyManager);
	return d->conf_was_found;
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
	RP_D(const KeyManager);
	const_cast<KeyManagerPrivate*>(d)->loadKeys();

	if (!areKeysLoaded()) {
		// Keys are not loaded.
		return VERIFY_KEY_DB_NOT_LOADED;
	}

	// Attempt to get the key from the map.
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

}

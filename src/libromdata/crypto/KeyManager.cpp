/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KeyManager.cpp: Encryption key manager.                                 *
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

#include "KeyManager.hpp"
#include "../file/FileSystem.hpp"
#include "../file/RpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_map>
using std::string;
using std::unique_ptr;
using std::unordered_map;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "../uvector.h"

namespace LibRomData {

class KeyManagerPrivate
{
	public:
		KeyManagerPrivate();

	private:
		KeyManagerPrivate(const KeyManagerPrivate &other);
		KeyManagerPrivate &operator=(const KeyManagerPrivate &other);

	public:
		// Singleton instance.
		static unique_ptr<KeyManager> instance;

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

		// If true, a load was attempted.
		bool loadAttempted;
		// Return value from the first loadKeys(0 call.
		int loadKeysRet;

		/**
		 * Process a configuration line.
		 * @param line_buf Configuration line.
		 */
		void processConfigLine(const string &line_buf);

		/**
		 * Load keys from the configuration file.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadKeys(void);
};

/** KeyManagerPrivate **/

// Singleton instance.
unique_ptr<KeyManager> KeyManagerPrivate::instance(new KeyManager());

KeyManagerPrivate::KeyManagerPrivate()
	: loadAttempted(false)
	, loadKeysRet(0)
{
	// Reserve 1 KB for the key store.
	vKeys.reserve(1024);
}

/**
 * Process a configuration line.
 * @param line_buf Configuration line.
 */
void KeyManagerPrivate::processConfigLine(const string &line_buf)
{
	// TODO: Parse the configuration line.
}

/**
 * Load keys from the configuration file.
 * @return 0 on success; negative POSIX error code on error.
 */
int KeyManagerPrivate::loadKeys(void)
{
	if (loadAttempted) {
		// loadKeys() was already called.
		return loadKeysRet;
	}

	// Attempting to load...
	loadAttempted = true;

	// Open the configuration file.
	rp_string config_path = FileSystem::getConfigDirectory();
	if (config_path.empty()) {
		// No configuration directory...
		loadKeysRet = -ENOENT;
		return loadKeysRet;
	}
	if (config_path.at(config_path.size()-1) != DIR_SEP_CHR)
		config_path += DIR_SEP_CHR;
	config_path += _RP("keys.conf");

	// Make sure the directories exist.
	// NOTE: The filename portion MUST be kept in config_path,
	// since the last component is ignored by rmkdir().
	int ret = FileSystem::rmkdir(config_path);
	if (ret != 0) {
		// rmkdir() failed.
		loadKeysRet = ret;
		return loadKeysRet;
	}

	// Open the configuration file.
	unique_ptr<IRpFile> file(new RpFile(config_path, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Error opening the file.
		loadKeysRet = file->lastError();
		if (loadKeysRet == 0) {
			// Unknown error...
			loadKeysRet = -EIO;
		}
		return loadKeysRet;
	}

	// Parse the file.
	// We're looking for the "[Keys]" section.
	string line_buf;
	line_buf.reserve(256);
	size_t sz;
	do {
		char buf[4096];
		sz = file->read(buf, sizeof(buf));
		if (sz == 0)
			break;

		int lastStartPos = 0;
		for (int pos = 0; pos < (int)sz; pos++) {
			// Find the first '\r' or '\n', starting at pos.
			if (buf[pos] == '\r' || buf[pos] == '\n') {
				// Found a newline.
				if (lastStartPos == pos) {
					// Empty line. Continue.
					// Next line starts at the next character.
					lastStartPos = pos + 1;
					continue;
				}

				// Add the string from lastStartPos up to the previous
				// character to the line buffer.
				line_buf.append(&buf[lastStartPos], pos - lastStartPos);
				if (!line_buf.empty()) {
					// Process the line.
					processConfigLine(line_buf);
					line_buf.clear();
				}

				// Next line starts at the next character.
				lastStartPos = pos + 1;
			}
		}

		// If anything is still left in buf[],
		// add it to the line buffer.
		if (lastStartPos < (int)sz) {
			line_buf.append(&buf[lastStartPos], (int)sz - lastStartPos);
		}
	} while (sz != 0);

	// If any data is left in the line buffer (possibly due
	// to a missing trailing newline), process it.
	if (!line_buf.empty()) {
		// Process the line.
		processConfigLine(line_buf);
	}

	// Keys loaded.
	loadKeysRet = 0;
	return loadKeysRet;
}

/** KeyManager **/

KeyManager::KeyManager()
	: d(new KeyManagerPrivate())
{ }

KeyManager::~KeyManager()
{
	delete d;
}

/**
 * Get the KeyManager instance.
 * @return KeyManager instance.
 */
KeyManager *KeyManager::instance(void)
{
	return KeyManagerPrivate::instance.get();
}

/**
 * Get an encryption key.
 * @param keyName	[in]  Encryption key name.
 * @param pKeyData	[out] Key data struct.
 * @return 0 on success; negative POSIX error code on error.
 */
int KeyManager::get(const char *keyName, KeyData_t *pKeyData) const
{
	if (!keyName || !pKeyData) {
		// Invalid parameters.
		return -EINVAL;
	}

	if (!d->loadAttempted) {
		// Load the keys.
		int ret = const_cast<KeyManagerPrivate*>(d)->loadKeys();
		if (ret != 0) {
			// Key load failed.
			return ret;
		}
	} else if (d->loadKeysRet != 0) {
		// Previous key load attempt failed.
		return d->loadKeysRet;
	}

	// Attempt to get the key from the map.
	unordered_map<string, uint32_t>::const_iterator iter = d->mapKeyNames.find(keyName);
	if (iter == d->mapKeyNames.end()) {
		// Key not found.
		return -ENOENT;
	}

	// Found the key.
	uint32_t keyIdx = iter->second;
	uint8_t len = ((keyIdx >> 24) & 0xFF);
	uint32_t idx = (keyIdx & 0xFFFFFF);

	// Make sure the key index is valid.
	assert(idx + len < d->vKeys.size());
	if (idx + len >= d->vKeys.size()) {
		// Should not happen...
		return -EFAULT;
	}

	pKeyData->length = len;
	pKeyData->key = d->vKeys.data() + idx;
	return 0;
}

}

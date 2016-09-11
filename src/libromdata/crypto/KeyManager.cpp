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
#include <cctype>

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

		// Temporary configuration loading variables.
		std::string cfg_curSection;
		bool cfg_isInKeysSection;
};

/** KeyManagerPrivate **/

// Singleton instance.
unique_ptr<KeyManager> KeyManagerPrivate::instance(new KeyManager());

KeyManagerPrivate::KeyManagerPrivate()
	: loadAttempted(false)
	, loadKeysRet(0)
	, cfg_isInKeysSection(false)
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
	bool foundNonSpace = false;
	const char *sect_start = nullptr;
	const char *equals_pos = nullptr;

	const char *chr = line_buf.data();
	for (int i = 0; i < (int)line_buf.size(); i++, chr++) {
		if (!foundNonSpace) {
			// Check if the current character is still a space.
			if (isspace(*chr)) {
				// Space character.
				continue;
			} else if (*chr == '[') {
				// Start of section.
				sect_start = chr+1;
				foundNonSpace = true;
			} else if (*chr == '=' || *chr == ';' || *chr == '#') {
				// Equals with no key name, or comment line.
				// Skip this line.
				return;
			} else {
				// Regular key line.
				foundNonSpace = true;
			}
		} else {
			if (sect_start != nullptr) {
				// Section header.
				if (*chr == ';' || *chr == '#') {
					// Comment. Skip this line.
					return;
				} else if (*chr == ']') {
					// End of section header.
					if (sect_start == chr) {
						// Empty section header.
						return;
					}

					// Check the section.
					cfg_curSection = string(sect_start, chr - sect_start);
					if (!strncasecmp(cfg_curSection.data(), "Keys", cfg_curSection.size())) {
						// This is the "Keys" section.
						cfg_isInKeysSection = true;
					} else {
						// This is not the "Keys" section.
						cfg_isInKeysSection = false;
					}

					// Skip the rest of the line.
					return;
				}
			} else {
				// Key name/value.
				if (!equals_pos) {
					// We haven't found the equals symbol yet.
					if (*chr == ';' || *chr == '#') {
						// Comment. Skip this line.
						return;
					} else if (*chr == '=') {
						// Found the equals symbol.
						equals_pos = chr;
					}
				} else {
					// We found the equals symbol.
					if (*chr == ';' || *chr == '#') {
						// Comment. Skip the rest of the line.
						break;
					}
				}
			}
		}
	}

	// Parse the key and value.
	if (!equals_pos) {
		// No equals sign.
		return;
	}

	const char *end = line_buf.data() + line_buf.size();
	if (chr > end) {
		// Out of bounds. Assume the end of the line.
		chr = end - 1;
	}

	string value(equals_pos + 1, end - 1 - equals_pos);
	if (value.empty() || (value.size() % 2 != 0)) {
		// Key is either empty, or is an odd length.
		// (Odd length means half a byte...)
		return;
	}

	string keyName(line_buf.data(), equals_pos - line_buf.data());
	if (keyName.empty()) {
		// Empty key name.
		return;
	}

	// Parse the value.
	size_t vKeys_start_pos = vKeys.size();
	size_t vKeys_pos = vKeys_start_pos;
	vKeys.resize(vKeys.size() + (value.size() / 2));

	// ASCII to HEX lookup table.
	// Reference: http://codereview.stackexchange.com/questions/22757/char-hex-string-to-byte-array
	const uint8_t ascii_to_hex[0x100] = {
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

	chr = value.data();
	for (int i = (int)value.size(); i > 0; i -= 2, vKeys_pos++) {
		// Process two characters at a time.
		// Two hexadecimal digits == one byte.
		char chr0 = ascii_to_hex[(uint8_t)chr[0]];
		char chr1 = ascii_to_hex[(uint8_t)chr[1]];
		if (chr0 > 0x0F || chr1 > 0x0F) {
			// Invalid character.
			vKeys.resize(vKeys_start_pos);
		}

		vKeys[vKeys_pos+0] = chr0;
		vKeys[vKeys_pos+1] = chr1;
	}

	// Value parsed successfully.
	uint32_t keyIdx = vKeys_start_pos;
	uint8_t len = (uint8_t)(vKeys_pos - vKeys_start_pos);
	keyIdx |= (len << 24);
	mapKeyNames.insert(std::make_pair(keyName, keyIdx));
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

	// We're not in the keys section initially.
	cfg_curSection.clear();
	cfg_isInKeysSection = false;

	// Parse the file.
	// We're looking for the "[Keys]" section.
	string line_buf;
	static const int LINE_LENGTH_MAX = 256;
	line_buf.reserve(LINE_LENGTH_MAX);
	size_t sz;
	bool skipLine = false;
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
				if (lastStartPos == pos || skipLine) {
					// Empty line, or the length limit has been exceeded.
					// Continue to the next line.
					// Next line starts at the next character.
					lastStartPos = pos + 1;
					skipLine = false;
					continue;
				}

				const int data_size = (pos - lastStartPos);

				// Add the string from lastStartPos up to the previous
				// character to the line buffer.
				if (line_buf.size() + data_size > LINE_LENGTH_MAX) {
					// Line length limit exceeded.
					line_buf.clear();
					// Next line starts at the next character.
					lastStartPos = pos + 1;
					continue;
				}

				line_buf.append(&buf[lastStartPos], data_size);
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
		if (!skipLine && lastStartPos < (int)sz) {
			const int data_size = ((int)sz - lastStartPos);
			if (line_buf.size() + data_size > LINE_LENGTH_MAX) {
				// Line length limit exceeded.
				skipLine = true;
				line_buf.clear();
			} else {
				line_buf.append(&buf[lastStartPos], data_size);
			}
		}
	} while (sz != 0);

	// If any data is left in the line buffer (possibly due
	// to a missing trailing newline), process it.
	if (!skipLine && !line_buf.empty()) {
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

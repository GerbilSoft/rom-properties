/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreUI.cpp: Key store object for Qt.                                *
 *                                                                         *
 * Copyright (c) 2012-2017 by David Korth.                                 *
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

#include "KeyStoreUI.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/crypto/KeyManager.hpp"
#include "librpbase/crypto/IAesCipher.hpp"
#include "librpbase/crypto/AesCipherFactory.hpp"
#include "librpbase/file/RpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// libromdata
#include "../disc/WiiPartition.hpp"
#include "../crypto/CtrKeyScrambler.hpp"
#include "../crypto/N3DSVerifyKeys.hpp"
using namespace LibRomData;

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>

// for Qt-style signal emission
#ifdef emit
#undef emit
#endif
#define emit

namespace LibRomData {

class KeyStoreUIPrivate
{
	public:
		explicit KeyStoreUIPrivate(KeyStoreUI *q);
		~KeyStoreUIPrivate();

	private:
		KeyStoreUI *const q_ptr;
		RP_DISABLE_COPY(KeyStoreUIPrivate)

	public:
		// Has the user changed anything?
		// This specifically refers to *user* settings.
		// reset() will emit dataChanged(), but changed
		// will be set back to false.
		bool changed;

	public:
		// Keys.
		vector<KeyStoreUI::Key> keys;

		// Sections.
		struct Section {
			string name;
			int keyIdxStart;	// Starting index in keys.
			int keyCount;		// Number of keys.
		};
		vector<Section> sections;

		// IAesCipher for verifying keys.
		IAesCipher *cipher;

	public:
		/**
		 * (Re-)Load the keys from keys.conf.
		 */
		void reset(void);

	public:
		/**
		 * Convert a sectIdx/keyIdx pair to a flat key index.
		 * @param sectIdx	[in] Section index.
		 * @param keyIdx	[in] Key index.
		 * @return Flat key index, or -1 if invalid.
		 */
		inline int sectKeyToIdx(int sectIdx, int keyIdx) const;

		/**
		 * Convert a flat key index to sectIdx/keyIdx.
		 * @param idx		[in] Flat key index.
		 * @param pSectIdx	[out] Section index.
		 * @param pKeyIdx	[out] Key index.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline int idxToSectKey(int idx, int *pSectIdx, int *pKeyIdx) const;

	public:
		/**
		 * Convert a string that may contain kanji to hexadecimal.
		 * @param str String.
		 * @return Converted string, or empty string on error.
		 */
		static string convertKanjiToHex(const string &str);

	public:
		/**
		 * Verify a key.
		 * @param keyData	[in] Key data. (16 bytes)
		 * @param verifyData	[in] Key verification data. (16 bytes)
		 * @param len		[in] Length of keyData and verifyData. (must be 16)
		 * @return 0 if the key is verified; non-zero if not.
		 */
		int verifyKeyData(const uint8_t *keyData, const uint8_t *verifyData, int len);

		/**
		 * Verify a key and update its status.
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 */
		void verifyKey(int sectIdx, int keyIdx);

	public:
		// Section enumeration.
		enum SectionIDs {
			Section_WiiPartition = 0,
			Section_CtrKeyScrambler,
			Section_N3DSVerifyKeys,
		};

		struct KeyBinAddress {
			uint32_t address;
			int keyIdx;		// -1 for end of list.
		};

		/**
		 * Import keys from a binary blob.
		 * @param sectIdx	[in] Section index.
		 * @param kba		[in] KeyBinAddress array.
		 * @param buf		[in] Key buffer.
		 * @param len		[in] Length of buf.
		 * @return Key import status.
		 */
		KeyStoreUI::ImportReturn importKeysFromBlob(int sectIdx,
			const KeyBinAddress *kba, const uint8_t *buf, unsigned int len);

		/**
		 * Get the encryption key required for aeskeydb.bin.
		 * @param pKey	[out] Key output.
		 * @return 0 on success; non-zero on error.
		 */
		int getAesKeyDB_key(u128_t *pKey) const;

	public:
		// TODO: Share with rpcli/verifykeys.cpp.
		// TODO: Central registration of key verification functions?
		typedef int (*pfnKeyCount_t)(void);
		typedef const char* (*pfnKeyName_t)(int keyIdx);
		typedef const uint8_t* (*pfnVerifyData_t)(int keyIdx);

		struct EncKeyFns_t {
			pfnKeyCount_t pfnKeyCount;
			pfnKeyName_t pfnKeyName;
			pfnVerifyData_t pfnVerifyData;
		};

		#define ENCKEYFNS(klass) { \
			klass::encryptionKeyCount_static, \
			klass::encryptionKeyName_static, \
			klass::encryptionVerifyData_static \
		}

		static const EncKeyFns_t encKeyFns[];

	public:
		// Hexadecimal lookup table.
		static const char hex_lookup[16];

		/**
		 * Convert a binary key to a hexadecimal string.
		 * @param data	[in] Binary key.
		 * @param len	[in] Length of binary key, in bytes.
		 * @return Hexadecimal string.
		 */
		static string binToHexStr(const uint8_t *data, unsigned int len);
};

/** KeyStoreUIPrivate **/

const KeyStoreUIPrivate::EncKeyFns_t KeyStoreUIPrivate::encKeyFns[] = {
	ENCKEYFNS(WiiPartition),
	ENCKEYFNS(CtrKeyScrambler),
	ENCKEYFNS(N3DSVerifyKeys),
};

// Hexadecimal lookup table.
const char KeyStoreUIPrivate::hex_lookup[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F',
};

KeyStoreUIPrivate::KeyStoreUIPrivate(KeyStoreUI *q)
	: q_ptr(q)
	, changed(false)
	, cipher(AesCipherFactory::create())
{
	// Make sure the cipher is usable.
	if (cipher->isInit()) {
		// Set the cipher parameters.
		cipher->setChainingMode(IAesCipher::CM_ECB);
	} else {
		// Cipher is not usable.
		// We won't be able to verify keys.
		delete cipher;
		cipher = nullptr;
	}

	// Load the key names from the various classes.
	// Values will be loaded later.
	sections.clear();
	sections.reserve(ARRAY_SIZE(encKeyFns));
	keys.clear();

	int keyIdxStart = 0;
	for (int encSysNum = 0; encSysNum < ARRAY_SIZE(encKeyFns); encSysNum++) {
		const EncKeyFns_t *const encSys = &encKeyFns[encSysNum];
		const int keyCount = encSys->pfnKeyCount();
		assert(keyCount > 0);
		if (keyCount <= 0)
			continue;

		// Set up the section.
		sections.resize(sections.size()+1);
		auto &section = sections[sections.size()-1];
		section.keyIdxStart = keyIdxStart;
		section.keyCount = keyCount;

		// Increment keyIdxStart for the next section.
		keyIdxStart += keyCount;

		// Get the keys.
		keys.reserve(keys.size() + keyCount);
		for (int i = 0; i < keyCount; i++) {
			// Key name.
			const char *const keyName = encSys->pfnKeyName(i);
			assert(keyName != nullptr);
			if (!keyName) {
				// Skip NULL key names. (This shouldn't happen...)
				continue;
			}

			keys.resize(keys.size()+1);
			auto &key = keys[keys.size()-1];
			key.name = latin1_to_utf8(keyName, -1);

			// Key is empty initially.
			key.status = KeyStoreUI::Key::Status_Empty;
			key.modified = false;

			// Allow kanji for twl-scrambler.
			key.allowKanji = (key.name == "twl-scrambler");
		}
	}

	// Keys will NOT be auto-loaded due to multiple inheritance issues.
	// The subclass must load the keys.
	//reset();
}

KeyStoreUIPrivate::~KeyStoreUIPrivate()
{
	delete cipher;
}

/**
 * (Re-)Load the keys from keys.conf.
 */
void KeyStoreUIPrivate::reset(void)
{
	if (keys.empty())
		return;

	// Get the KeyManager.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager)
		return;

	// Have any keys actually changed?
	bool hasChanged = false;

	int keyIdxStart = 0;
	KeyManager::KeyData_t keyData;
	for (int encSysNum = 0; encSysNum < ARRAY_SIZE(encKeyFns); encSysNum++) {
		const KeyStoreUIPrivate::EncKeyFns_t *const encSys = &encKeyFns[encSysNum];
		const int keyCount = encSys->pfnKeyCount();
		assert(keyCount > 0);
		if (keyCount <= 0)
			continue;

		// Starting key.
		KeyStoreUI::Key *pKey = &keys[keyIdxStart];
		// Increment keyIdxStart for the next section.
		keyIdxStart += keyCount;

		// Get the keys.
		for (int i = 0; i < keyCount; i++, pKey++) {
			// Key name.
			const char *const keyName = encSys->pfnKeyName(i);
			assert(keyName != nullptr);
			if (!keyName) {
				// Skip NULL key names. (This shouldn't happen...)
				continue;
			}

			// Get the key data without verifying.
			// NOTE: If we verify here, the key data won't be returned
			// if it isn't valid.
			KeyManager::VerifyResult res = keyManager->get(keyName, &keyData);
			switch (res) {
				case KeyManager::VERIFY_OK:
					// Convert the key to a string.
					assert(keyData.key != nullptr);
					assert(keyData.length > 0);
					assert(keyData.length <= 32);
					if (keyData.key != nullptr && keyData.length > 0 && keyData.length <= 32) {
						string value = binToHexStr(keyData.key, keyData.length);
						if (pKey->value != value) {
							pKey->value = value;
							hasChanged = true;
						}

						// Verify the key.
						verifyKey(encSysNum, i);
					} else {
						// Key is invalid...
						// TODO: Show an error message?
						if (!pKey->value.empty()) {
							pKey->value.clear();
							hasChanged = true;
						}
						pKey->status = KeyStoreUI::Key::Status_NotAKey;
					}
					break;

				case KeyManager::VERIFY_KEY_INVALID:
					// Key is invalid. (i.e. not in the correct format)
					if (!pKey->value.empty()) {
						pKey->value.clear();
						hasChanged = true;
					}
					pKey->status = KeyStoreUI::Key::Status_NotAKey;
					break;

				default:
					// Assume the key wasn't found.
					if (!pKey->value.empty()) {
						pKey->value.clear();
						hasChanged = true;
					}
					pKey->status = KeyStoreUI::Key::Status_Empty;
					break;
			}

			// Key is no longer modified.
			pKey->modified = false;
		}
	}

	if (hasChanged) {
		// Keys have changed.
		RP_Q(KeyStoreUI);
		emit q->allKeysChanged_int();
	}

	// Keys have been reset.
	changed = false;
}

/**
 * Convert a sectIdx/keyIdx pair to a flat key index.
 * @param sectIdx	[in] Section index.
 * @param keyIdx	[in] Key index.
 * @return Flat key index, or -1 if invalid.
 */
inline int KeyStoreUIPrivate::sectKeyToIdx(int sectIdx, int keyIdx) const
{
	assert(sectIdx >= 0);
	assert(sectIdx < (int)sections.size());
	if (sectIdx < 0 || sectIdx >= (int)sections.size())
		return -1;

	assert(keyIdx >= 0);
	assert(keyIdx < sections[sectIdx].keyCount);
	if (keyIdx < 0 || keyIdx >= sections[sectIdx].keyCount)
		return -1;

	return sections[sectIdx].keyIdxStart + keyIdx;
}

/**
 * Convert a flat key index to sectIdx/keyIdx.
 * @param idx		[in] Flat key index.
 * @param pSectIdx	[out] Section index.
 * @param pKeyIdx	[out] Key index.
 * @return 0 on success; negative POSIX error code on error.
 */
inline int KeyStoreUIPrivate::idxToSectKey(int idx, int *pSectIdx, int *pKeyIdx) const
{
	assert(pSectIdx != nullptr);
	assert(pKeyIdx != nullptr);
	if (!pSectIdx || !pKeyIdx)
		return -EINVAL;

	assert(idx >= 0);
	assert(idx < (int)keys.size());
	if (idx < 0 || idx >= (int)keys.size())
		return -ERANGE;

	// Figure out what section this key is in.
	for (int i = 0; i < (int)sections.size(); i++) {
		const KeyStoreUIPrivate::Section &section = sections[i];
		if (idx < (section.keyIdxStart + section.keyCount)) {
			// Found the section.
			*pSectIdx = i;
			*pKeyIdx = idx - section.keyIdxStart;
			return 0;
		}
	}

	// Not found.
	return -ENOENT;
}

/**
 * Import keys from a binary blob.
 * FIXME: More comprehensive error messages for the message bar.
 * @param sectIdx	[in] Section index.
 * @param kba		[in] KeyBinAddress array.
 * @param buf		[in] Key buffer.
 * @param len		[in] Length of buf.
 * @return Key import status.
 */
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importKeysFromBlob(
	int sectIdx, const KeyBinAddress *kba, const uint8_t *buf, unsigned int len)
{
	KeyStoreUI::ImportReturn iret = {0, 0, 0, 0, 0, 0, 0};

	assert(sectIdx >= 0);
	assert(sectIdx < (int)sections.size());
	assert(kba != nullptr);
	assert(buf != nullptr);
	assert(len != 0);
	if (sectIdx < 0 || sectIdx >= (int)sections.size() ||
	    !kba || !buf || len == 0)
	{
		iret.status = KeyStoreUI::Import_InvalidParams;
		return iret;
	}

	RP_Q(KeyStoreUI);
	bool wereKeysImported = false;
	const int keyIdxStart = sections[sectIdx].keyIdxStart;
	for (; kba->keyIdx >= 0; kba++) {
		KeyStoreUI::Key *const pKey = &keys[keyIdxStart + kba->keyIdx];
		if (pKey->status == KeyStoreUI::Key::Status_OK) {
			// Key is already OK. Don't bother with it.
			iret.keysExist++;
			continue;
		}
		assert(kba->address + 16 < len);
		if (kba->address + 16 > len) {
			// Out of range...
			// FIXME: Report an error?
			continue;
		}
		const uint8_t *const keyData = &buf[kba->address];

		// Check if the key in the binary file is correct.
		const uint8_t *const verifyData = encKeyFns[sectIdx].pfnVerifyData(kba->keyIdx);
		assert(verifyData != nullptr);
		if (!verifyData) {
			// Can't verify this key...
			// Import it anyway.
			const string new_value = binToHexStr(keyData, 16);
			if (pKey->value != new_value) {
				pKey->value = new_value;
				pKey->status = KeyStoreUI::Key::Status_Unknown;
				pKey->modified = true;
				iret.keysImportedNoVerify++;
				wereKeysImported = true;
				emit q->keyChanged_int(sectIdx, kba->keyIdx);
				emit q->keyChanged_int(keyIdxStart + kba->keyIdx);
			} else {
				// No change.
				iret.keysExist++;
			}
			continue;
		}

		// Verify the key.
		int ret = verifyKeyData(keyData, verifyData, 16);
		if (ret == 0) {
			// Found a match!
			const string new_value = binToHexStr(keyData, 16);
			if (pKey->value != new_value) {
				pKey->value = binToHexStr(keyData, 16);
				pKey->status = KeyStoreUI::Key::Status_OK;
				pKey->modified = true;
				iret.keysImportedVerify++;
				wereKeysImported = true;
				emit q->keyChanged_int(sectIdx, kba->keyIdx);
				emit q->keyChanged_int(keyIdxStart + kba->keyIdx);
			} else {
				// No change.
				iret.keysExist++;
			}
		} else {
			// Not a match.
			iret.keysInvalid++;
		}
	}

	if (wereKeysImported) {
		emit q->modified_int();
	}
	iret.status = (wereKeysImported
		? KeyStoreUI::Import_KeysImported
		: KeyStoreUI::Import_NoKeysImported);
	return iret;
}

/**
 * Get the encryption key required for aeskeydb.bin.
 * TODO: Support for Debug systems.
 * @param pKey	[out] Key output.
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUIPrivate::getAesKeyDB_key(u128_t *pKey) const
{
	assert(pKey != nullptr);
	if (!pKey) {
		return -EINVAL;
	}

	// Get the CTR scrambler constant.
	const Section &sectScrambler = sections[Section_CtrKeyScrambler];
	const KeyStoreUI::Key &ctr_scrambler = keys[sectScrambler.keyIdxStart + CtrKeyScrambler::Key_Ctr_Scrambler];
	if (ctr_scrambler.status != KeyStoreUI::Key::Status_OK) {
		// Key is not correct.
		return -ENOENT;
	}

	// Get Slot0x2CKeyX.
	const Section &sectN3DS = sections[Section_N3DSVerifyKeys];
	const KeyStoreUI::Key &key_slot0x2CKeyX = keys[sectN3DS.keyIdxStart + N3DSVerifyKeys::Key_Retail_Slot0x2CKeyX];
	if (key_slot0x2CKeyX.status != KeyStoreUI::Key::Status_OK) {
		// Key is not correct.
		return -ENOENT;
	}

	// Convert the keys to bytes.
	u128_t scrambler, keyX, keyY;
	int ret = KeyManager::hexStringToBytes(ctr_scrambler.value.c_str(), scrambler.u8, sizeof(scrambler.u8));
	if (ret != 0) {
		return -EIO;
	}
	ret = KeyManager::hexStringToBytes(key_slot0x2CKeyX.value.c_str(), keyX.u8, sizeof(keyX.u8));
	if (ret != 0) {
		return -EIO;
	}
	// Slot0x2CKeyY for aeskeydb.bin is all 0.
	memset(keyY.u8, 0, sizeof(keyY.u8));

	// Scramble the key.
	return CtrKeyScrambler::CtrScramble(pKey, &keyX, &keyY, &scrambler);
}

/**
 * Convert a string that may contain kanji to hexadecimal.
 * @param str String.
 * @return Converted string, or empty string on error.
 */
string KeyStoreUIPrivate::convertKanjiToHex(const string &str)
{
	// Check for non-ASCII characters.
	// TODO: Also check for non-hex digits?
	bool hasNonAscii = false;
	for (const char *p = str.c_str(); *p != 0; p++) {
		// The following check works for both UTF-8 and UTF-16.
		// If the character value is >= 128, it's non-ASCII.
		if (((unsigned int)*p) >= 128) {
			// Found a non-ASCII character.
			hasNonAscii = true;
			break;
		}
	}

	if (!hasNonAscii) {
		// No non-ASCII characters.
		return str;
	}

	// We're expecting 7 kanji symbols,
	// but we'll take any length.

	// Convert to UTF-16 first.
	// FIXME: std::string overload for utf8_to_utf16().
	const std::u16string u16str = utf8_to_utf16(str.data(), (int)str.size());

	// Convert to a UTF-16LE hex string, starting with U+FEFF.
	// TODO: Combine with the first loop?
	string hexstr;
	hexstr.reserve(4+(u16str.size()*4));
	hexstr += "FFFE";
	for (const char16_t *p = u16str.c_str(); *p != 0; p++) {
		const char16_t u16 = *p;
		hexstr += (char16_t)hex_lookup[(u16 >>  4) & 0x0F];
		hexstr += (char16_t)hex_lookup[(u16 >>  0) & 0x0F];
		hexstr += (char16_t)hex_lookup[(u16 >> 12) & 0x0F];
		hexstr += (char16_t)hex_lookup[(u16 >>  8) & 0x0F];
	}

	return hexstr;
}

/**
 * Verify a key.
 * @param keyData	[in] Key data. (16 bytes)
 * @param verifyData	[in] Key verification data. (16 bytes)
 * @param len		[in] Length of keyData and verifyData. (must be 16)
 * @return 0 if the key is verified; non-zero if not.
 */
int KeyStoreUIPrivate::verifyKeyData(const uint8_t *keyData, const uint8_t *verifyData, int len)
{
	assert(len == 16);
	if (len != 16) {
		// Invalid key data.
		return -EINVAL;
	}

	// Attempt to decrypt the verification data using the key.
	uint8_t testData[16];
	memcpy(testData, verifyData, sizeof(testData));
	int ret = cipher->setKey(keyData, len);
	if (ret != 0) {
		// Error setting the key.
		return -EIO;
	}
	size_t size = cipher->decrypt(testData, sizeof(testData));
	if (size != sizeof(testData)) {
		// Error decrypting the data.
		return -EIO;
	}

	// Check if the decrypted data is correct.
	return (memcmp(testData, KeyManager::verifyTestString, sizeof(testData)) != 0);
}

/**
 * Verify a key and update its status.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreUIPrivate::verifyKey(int sectIdx, int keyIdx)
{
	int idx = sectKeyToIdx(sectIdx, keyIdx);
	if (idx < 0)
		return;

	// Check the key length.
	KeyStoreUI::Key &key = keys[idx];
	if (key.value.empty()) {
		// Empty key.
		key.status = KeyStoreUI::Key::Status_Empty;
		return;
	} else if (key.value.size() != 16 && key.value.size() % 2 != 0) {
		// Invalid length.
		// TODO: Support keys that aren't 128-bit.
		key.status = KeyStoreUI::Key::Status_NotAKey;
		return;
	}

	if (!cipher) {
		// Cipher is unavailable.
		// Cannot verify the key.
		key.status = KeyStoreUI::Key::Status_Unknown;
		return;
	}

	// Get the key verification data. (16 bytes)
	const uint8_t *const verifyData = encKeyFns[sectIdx].pfnVerifyData(keyIdx);
	if (!verifyData) {
		// No key verification data is available.
		key.status = KeyStoreUI::Key::Status_Unknown;
		return;
	}

	// Convert the key to bytes.
	// TODO: Support keys that aren't 128-bit.
	uint8_t keyBytes[16];
	int ret = KeyManager::hexStringToBytes(key.value.c_str(), keyBytes, sizeof(keyBytes));
	if (ret != 0) {
		// Invalid character(s) encountered.
		key.status = KeyStoreUI::Key::Status_NotAKey;
		return;
	}

	// Verify the key.
	ret = verifyKeyData(keyBytes, verifyData, sizeof(keyBytes));
	if (ret == 0) {
		// Decrypted data is correct.
		key.status = KeyStoreUI::Key::Status_OK;
	} else {
		// Decrypted data is wrong.
		key.status = KeyStoreUI::Key::Status_Incorrect;
	}
}

/**
 * Convert a binary key to a hexadecimal string.
 * @param data	[in] Binary key.
 * @param len	[in] Length of binary key, in bytes.
 * @return Hexadecimal string.
 */
string KeyStoreUIPrivate::binToHexStr(const uint8_t *data, unsigned int len)
{
	assert(data != nullptr);
	assert(len > 0);
	assert(len <= 64);
	if (!data || len == 0 || len > 64)
		return string();

	string hexstr;
	hexstr.reserve(len*2);
	for (; len > 0; len--, data++) {
		hexstr += hex_lookup[*data >> 4];
		hexstr += hex_lookup[*data & 0x0F];
	}

	return hexstr;
}

/** KeyStore **/

/**
 * Create a new KeyStoreUI object.
 */
KeyStoreUI::KeyStoreUI()
	: d_ptr(new KeyStoreUIPrivate(this))
{ }

KeyStoreUI::~KeyStoreUI()
{
	delete d_ptr;
}

/**
 * (Re-)Load the keys from keys.conf.
 */
void KeyStoreUI::reset(void)
{
	RP_D(KeyStoreUI);
	d->reset();
}

/**
 * Convert a sectIdx/keyIdx pair to a flat key index.
 * @param sectIdx	[in] Section index.
 * @param keyIdx	[in] Key index.
 * @return Flat key index, or -1 if invalid.
 */
int KeyStoreUI::sectKeyToIdx(int sectIdx, int keyIdx) const
{
	RP_D(const KeyStoreUI);
	return d->sectKeyToIdx(sectIdx, keyIdx);
}

/**
 * Convert a flat key index to sectIdx/keyIdx.
 * @param idx		[in] Flat key index.
 * @param pSectIdx	[out] Section index.
 * @param pKeyIdx	[out] Key index.
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUI::idxToSectKey(int idx, int *pSectIdx, int *pKeyIdx) const
{
	RP_D(const KeyStoreUI);
	return d->idxToSectKey(idx, pSectIdx, pKeyIdx);
}

/** Accessors. **/

/**
 * Get the number of sections. (top-level)
 * @return Number of sections.
 */
int KeyStoreUI::sectCount(void) const
{
	RP_D(const KeyStoreUI);
	return (int)d->sections.size();
}

/**
 * Get a section name.
 * @param sectIdx Section index.
 * @return Section name, or nullptr on error.
 */
const char *KeyStoreUI::sectName(int sectIdx) const
{
	RP_D(const KeyStoreUI);
	assert(sectIdx >= 0);
	assert(sectIdx < (int)d->sections.size());
	assert(sectIdx < ARRAY_SIZE(d->encKeyFns));
	if (sectIdx < 0 || sectIdx >= (int)d->sections.size() || sectIdx >= ARRAY_SIZE(d->encKeyFns))
		return nullptr;

	static const char *const sectNames[] = {
		NOP_C_("KeyStoreUI|Section", "Nintendo Wii AES Keys"),
		NOP_C_("KeyStoreUI|Section", "Nintendo 3DS Key Scrambler Constants"),
		NOP_C_("KeyStoreUI|Section", "Nintendo 3DS AES Keys"),
	};
	static_assert(ARRAY_SIZE(sectNames) == ARRAY_SIZE(d->encKeyFns),
		"sectNames[] is out of sync with d->encKeyFns[].");

	return dpgettext_expr(RP_I18N_DOMAIN, "KeyStoreUI|Section", sectNames[sectIdx]);
}

/**
 * Get the number of keys in a given section.
 * @param sectIdx Section index.
 * @return Number of keys in the section, or -1 on error.
 */
int KeyStoreUI::keyCount(int sectIdx) const
{
	RP_D(const KeyStoreUI);
	assert(sectIdx >= 0);
	assert(sectIdx < (int)d->sections.size());
	if (sectIdx < 0 || sectIdx >= (int)d->sections.size())
		return -1;
	return d->sections[sectIdx].keyCount;
}

/**
 * Get the total number of keys.
 * @return Total number of keys.
 */
int KeyStoreUI::totalKeyCount(void) const
{
	RP_D(const KeyStoreUI);
	int ret = 0;
	for (int i = (int)d->sections.size()-1; i >= 0; i--) {
		ret += d->sections[i].keyCount;
	}
	return ret;
}

/**
 * Is the KeyStore empty?
 * @return True if empty; false if not.
 */
bool KeyStoreUI::isEmpty(void) const
{
	RP_D(const KeyStoreUI);
	// TODO: Check each section to make sure they're not empty?
	return d->sections.empty();
}

/**
 * Get a Key object.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 * @return Key object, or nullptr on error.
 */
const KeyStoreUI::Key *KeyStoreUI::getKey(int sectIdx, int keyIdx) const
{
	RP_D(const KeyStoreUI);
	int idx = d->sectKeyToIdx(sectIdx, keyIdx);
	if (idx < 0)
		return nullptr;
	return &d->keys[idx];
}

/**
 * Get a Key object using a linear key count.
 * TODO: Remove this once we switch to a Tree model.
 * @param idx Key index.
 * @return Key object, or nullptr on error.
 */
const KeyStoreUI::Key *KeyStoreUI::getKey(int idx) const
{
	RP_D(const KeyStoreUI);
	assert(idx >= 0);
	assert(idx < (int)d->keys.size());
	if (idx < 0 || idx >= (int)d->keys.size())
		return nullptr;
	return &d->keys[idx];
}

/**
 * Set a key's value.
 * If successful, and the new value is different,
 * keyChanged() will be emitted.
 *
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 * @param value New value.
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUI::setKey(int sectIdx, int keyIdx, const string &value)
{
	RP_D(KeyStoreUI);
	int idx = d->sectKeyToIdx(sectIdx, keyIdx);
	if (idx < 0)
		return -ERANGE;

	// If allowKanji is true, check if the key is kanji
	// and convert it to UTF-16LE hexadecimal.
	Key &key = d->keys[idx];
	string new_value;
	if (key.allowKanji) {
		// Convert kanji to hexadecimal if needed.
		// NOTE: convertKanjiToHex() returns an empty string on error,
		// so if the original string is empty, don't do anything.
		if (!value.empty()) {
			string convKey = d->convertKanjiToHex(value);
			if (convKey.empty()) {
				// Invalid kanji key.
				return -EINVAL;
			}
			new_value = convKey;
		}
	} else {
		// Hexadecimal only.
		// TODO: Validate it here? We're already
		// using a validator in the UI...
		new_value = value;
		// Convert ASCII characters to uppercase.
		for (auto iter = new_value.begin(); iter != new_value.end(); ++iter) {
			if ((unsigned int)*iter < 128) {
				*iter = toupper(*iter);
			}
		}
	}

	if (key.value != new_value) {
		key.value = new_value;
		key.modified = true;
		// Verify the key.
		d->verifyKey(sectIdx, keyIdx);
		// Key has changed.
		emit keyChanged_int(sectIdx, keyIdx);
		emit keyChanged_int(idx);
		d->changed = true;
		emit modified_int();
	}
	return 0;
}

/**
 * Set a key's value.
 * If successful, and the new value is different,
 * keyChanged() will be emitted.
 *
 * @param idx Flat key index.
 * @param value New value.
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUI::setKey(int idx, const string &value)
{
	RP_D(KeyStoreUI);
	assert(idx >= 0);
	assert(idx < (int)d->keys.size());
	if (idx < 0 || idx >= (int)d->keys.size())
		return -ERANGE;

	Key &key = d->keys[idx];
	string new_value;
	if (key.allowKanji) {
		// Convert kanji to hexadecimal if needed.
		// NOTE: convertKanjiToHex() returns an empty string on error,
		// so if the original string is empty, don't do anything.
		if (!value.empty()) {
			string convKey = KeyStoreUIPrivate::convertKanjiToHex(value);
			if (convKey.empty()) {
				// Invalid kanji key.
				return -EINVAL;
			}
			new_value = convKey;
		}
	} else {
		// Hexadecimal only.
		// TODO: Validate it here? We're already
		// using a validator in the UI...
		new_value = value;
		// Convert ASCII characters to uppercase.
		for (auto iter = new_value.begin(); iter != new_value.end(); ++iter) {
			if ((unsigned int)*iter < 128) {
				*iter = toupper(*iter);
			}
		}
	}

	if (key.value != new_value) {
		key.value = new_value;
		key.modified = true;
		int sectIdx = -1, keyIdx = -1;
		int ret = d->idxToSectKey(idx, &sectIdx, &keyIdx);
		assert(ret == 0);
		assert(sectIdx >= 0);
		assert(keyIdx >= 0);
		if (ret == 0 && sectIdx >= 0 && keyIdx >= 0) {
			// Verify the key.
			d->verifyKey(sectIdx, keyIdx);
			emit keyChanged_int(sectIdx, keyIdx);
		}
		emit keyChanged_int(idx);
		d->changed = true;
		emit modified_int();
	}
	return 0;
}

/**
 * Mark all keys as saved.
 * This clears the "modified" field.
 */
void KeyStoreUI::allKeysSaved(void)
{
	RP_D(KeyStoreUI);
	for (auto iter = d->keys.begin(); iter != d->keys.end(); ++iter) {
		iter->modified = false;
	}

	// KeyStore is no longer changed.
	// NOTE: Not emitting modified() here.
	d->changed = false;
}

/**
 * Has KeyStore been changed by the user?
 * @return True if it has; false if it hasn't.
 */
bool KeyStoreUI::hasChanged(void) const
{
	RP_D(const KeyStoreUI);
	return d->changed;
}

/**
 * Import keys from Wii keys.bin. (BootMii format)
 * @param filename keys.bin filename.
 * @return Number of keys imported if the file is valid; negative POSIX error code on error.
 */
KeyStoreUI::ImportReturn KeyStoreUI::importWiiKeysBin(const char *filename)
{
	ImportReturn iret = {0, 0, 0, 0, 0, 0, 0};

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file) {
		iret.status = Import_OpenError;
		return iret;
	}

	// File must be 1,024 bytes.
	if (file->size() != 1024) {
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Read the entire 1,024 bytes.
	uint8_t buf[1024];
	size_t size = file->read(buf, sizeof(buf));
	if (size != 1024) {
		// Read error.
		// TODO: file->lastError()?
		iret.status = Import_ReadError;
		return iret;
	}
	file->close();

	// Verify the BootMii (BackupMii) header.
	// TODO: Is there a v0? If this shows v0, show a different message.
	static const char BackupMii_magic[] = "BackupMii v1";
	if (memcmp(buf, BackupMii_magic, sizeof(BackupMii_magic)-1) != 0) {
		// TODO: Check for v0.
		iret.status = Import_InvalidFile;
		return iret;
	}

	// TODO:
	// - rvl-korean may be in keys.bin files dumped from Korean systems.
	// - SD keys are not present in keys.bin.

	static const KeyStoreUIPrivate::KeyBinAddress keyBinAddress[] = {
		{0x114, WiiPartition::Key_Rvl_Common},
		{0x114, WiiPartition::Key_Rvt_Debug},

		{0, -1}
	};

	// Import the keys.
	RP_D(KeyStoreUI);
	return d->importKeysFromBlob(KeyStoreUIPrivate::Section_WiiPartition,
		keyBinAddress, buf, sizeof(buf));
}

/**
 * Import keys from Wii U otp.bin.
 * @param filename otp.bin filename.
 * @return Key import status.
 */
KeyStoreUI::ImportReturn KeyStoreUI::importWiiUOtpBin(const char *filename)
{
	ImportReturn iret = {0, 0, 0, 0, 0, 0, 0};

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file) {
		iret.status = Import_OpenError;
		return iret;
	}

	// File must be 1,024 bytes.
	if (file->size() != 1024) {
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Read the entire 1,024 bytes.
	uint8_t buf[1024];
	size_t size = file->read(buf, sizeof(buf));
	if (size != 1024) {
		// Read error.
		// TODO: file->lastError()?
		iret.status = Import_ReadError;
		return iret;
	}
	file->close();

	// Verify the vWii Boot1 hash.
	// TODO: Are there multiple variants of vWii Boot1?
	static const uint8_t vWii_Boot1_hash[20] = {
		0xF8,0xB1,0x8E,0xC3,0xFE,0x26,0xB9,0xB1,
		0x1A,0xD4,0xA4,0xED,0xD3,0xB7,0xA0,0x31,
		0x11,0x9A,0x79,0xF8
	};
	if (memcmp(buf, vWii_Boot1_hash, sizeof(vWii_Boot1_hash)-1) != 0) {
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Key addresses and indexes.
	// TODO:
	// - Verify if Wii debug keys are present in otp.bin from CAT-R units.
	// - SD keys are not present in otp.bin.
	static const KeyStoreUIPrivate::KeyBinAddress keyBinAddress[] = {
		{0x014, WiiPartition::Key_Rvl_Common},
		{0x014, WiiPartition::Key_Rvt_Debug},
		{0x348, WiiPartition::Key_Rvl_Korean},
		{0x0D0, WiiPartition::Key_Wup_vWii_Common},

		// TODO: Import Wii U keys.
#if 0
		{0x090, /* Wii U ancast key */},
		{0x0E0, /* Wii U common key */},
#endif

		{0, -1}
	};

	// Import the keys.
	RP_D(KeyStoreUI);
	return d->importKeysFromBlob(KeyStoreUIPrivate::Section_WiiPartition,
		keyBinAddress, buf, sizeof(buf));
}

/**
 * Import keys from 3DS boot9.bin.
 * @param filename boot9.bin filename.
 * @return Number of keys imported if the file is valid; negative POSIX error code on error.
 */
KeyStoreUI::ImportReturn KeyStoreUI::import3DSboot9bin(const char *filename)
{
	ImportReturn iret = {0, 0, 0, 0, 0, 0, 0};

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file) {
		iret.status = Import_OpenError;
		return iret;
	}

	// File may be:
	// - 65,536 bytes: Unprotected + Protected boot9
	// - 32,768 bytes: Protected boot9
	const int64_t fileSize = file->size();
	if (fileSize != 65536 && fileSize != 32768) {
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Read the protected section into memory.
	unique_ptr<uint8_t[]> buf(new uint8_t[32768]);
	if (fileSize == 65536) {
		// Seek to the second half.
		int ret = file->seek(32768);
		if (ret != 0) {
			// Seek error.
			iret.status = Import_ReadError;
			return iret;
		}
	}
	size_t size = file->read(buf.get(), 32768);
	if (size != 32768) {
		// Read error.
		iret.status = Import_ReadError;
		return iret;
	}
	file->close();

	// Check the CRC32.
	// NOTE: CRC32 isn't particularly strong, so we'll still
	// verify the keys before importing them.
	const uint32_t crc = crc32(0, buf.get(), 32768);
	if (crc != 0x9D50A525) {
		// Incorrect CRC32.
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Key addresses and indexes.
	static const KeyStoreUIPrivate::KeyBinAddress keyBinAddress[] = {
		{0x59D0, N3DSVerifyKeys::Key_Retail_Slot0x2CKeyX},
		{0x5A20, N3DSVerifyKeys::Key_Retail_Slot0x3DKeyX},
		{0x5DD0, N3DSVerifyKeys::Key_Debug_Slot0x2CKeyX},
		{0x5E20, N3DSVerifyKeys::Key_Debug_Slot0x3DKeyX},

		{0, -1}
	};

	// Import the keys.
	RP_D(KeyStoreUI);
	return d->importKeysFromBlob(KeyStoreUIPrivate::Section_N3DSVerifyKeys,
		keyBinAddress, buf.get(), 32768);
}

/**
 * Import keys from 3DS aeskeydb.bin.
 * @param filename aeskeydb.bin filename.
 * @return Key import status.
 */
KeyStoreUI::ImportReturn KeyStoreUI::import3DSaeskeydb(const char *filename)
{
	ImportReturn iret = {0, 0, 0, 0, 0, 0, 0};

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file) {
		iret.status = Import_OpenError;
		return iret;
	}

	// File must be <= 64 KB and a multiple of 32 bytes.
	const int64_t fileSize = file->size();
	if (fileSize == 0 || fileSize > 65536 || (fileSize % 32 != 0)) {
		iret.status = Import_InvalidFile;
		return iret;
	}

	// Read the entire file into memory.
	unique_ptr<uint8_t[]> buf(new uint8_t[(size_t)fileSize]);
	size_t size = file->read(buf.get(), (size_t)fileSize);
	if (size != (unsigned int)fileSize) {
		// Read error.
		// TODO: file->lastError()?
		iret.status = Import_ReadError;
		return iret;
	}
	file->close();

	// aeskeydb keyslot from Decrypt9WIP.
	// NOTE: Decrypt9WIP and SafeB9SInstaller interpret the "keyUnitType" field differently.
	// - Decrypt9WIP: isDevkitKey == 0 for retail, non-zero for debug
	// - SafeB9SInstaller: keyUnitType == 0 for ALL units, 1 for retail only, 2 for debug only
	// To prevent issues, we'll check both retail and debug for all keys.
	typedef struct PACKED _AesKeyInfo {
		uint8_t slot;		// keyslot, 0x00...0x3F 
		char type;		// type 'X' / 'Y' / 'N' for normalKey / 'I' for IV
		char id[10];		// key ID for special keys, all zero for standard keys
		uint8_t reserved[2];	// reserved space
		uint8_t keyUnitType;	// see above
		uint8_t isEncrypted;	// if non-zero, key is encrypted using Slot0x2C, with KeyY = 0
		uint8_t key[16];
	} AesKeyInfo;

	// Slot0x2CKeyX is needed to decrypt keys if the
	// aeskeydb.bin file is encrypted.
	RP_D(KeyStoreUI);
	unique_ptr<IAesCipher> cipher;
	u128_t aeskeydb_key;
	if (d->getAesKeyDB_key(&aeskeydb_key) == 0) {
		// TODO: Support for debug-encrypted aeskeydb.bin?
		cipher.reset(AesCipherFactory::create());
		cipher->setChainingMode(IAesCipher::CM_CTR);
		cipher->setKey(aeskeydb_key.u8, sizeof(aeskeydb_key.u8));
	}

	AesKeyInfo *aesKey = reinterpret_cast<AesKeyInfo*>(buf.get());
	const AesKeyInfo *const aesKeyEnd = reinterpret_cast<const AesKeyInfo*>(buf.get() + fileSize);
	const int keyIdxStart = d->sections[KeyStoreUIPrivate::Section_N3DSVerifyKeys].keyIdxStart;
	bool wereKeysImported = false;
	do {
		// Check if this is a supported keyslot.
		// Key indexes: 0 == retail, 1 == debug
		// except for Slot0x3DKeyY, which has 6 of each
		// TODO: Use static const arrays and assign an array pointer?
		int8_t keyIdx[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 ,-1};
		switch (aesKey->slot) {
			case 0x18:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					keyIdx[0] = N3DSVerifyKeys::Key_Retail_Slot0x18KeyX;
					keyIdx[1] = N3DSVerifyKeys::Key_Debug_Slot0x18KeyX;
				}
				break;

			case 0x1B:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					keyIdx[0] = N3DSVerifyKeys::Key_Retail_Slot0x1BKeyX;
					keyIdx[1] = N3DSVerifyKeys::Key_Debug_Slot0x1BKeyX;
				}
				break;

			case 0x25:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					keyIdx[0] = N3DSVerifyKeys::Key_Retail_Slot0x25KeyX;
					keyIdx[1] = N3DSVerifyKeys::Key_Debug_Slot0x25KeyX;
				}
				break;

			case 0x2C:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					keyIdx[0] = N3DSVerifyKeys::Key_Retail_Slot0x2CKeyX;
					keyIdx[1] = N3DSVerifyKeys::Key_Debug_Slot0x2CKeyX;
				}
				break;

			case 0x3D:
				// KeyX, KeyY, and KeyNormal are available.
				// TODO: Optimize Y/N cases by setting an array pointer?
				switch (aesKey->type) {
					case 'X':
						keyIdx[0] = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyX;
						keyIdx[1] = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyX;
						break;
					case 'Y':
						keyIdx[0]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_0;
						keyIdx[1]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_1;
						keyIdx[2]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_2;
						keyIdx[3]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_3;
						keyIdx[4]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_4;
						keyIdx[5]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyY_5;
						keyIdx[6]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_0;
						keyIdx[7]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_1;
						keyIdx[8]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_2;
						keyIdx[9]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_3;
						keyIdx[10] = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_4;
						keyIdx[11] = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyY_5;
						break;
					case 'N':
						keyIdx[0]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_0;
						keyIdx[1]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_1;
						keyIdx[2]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_2;
						keyIdx[3]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_3;
						keyIdx[4]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_4;
						keyIdx[5]  = N3DSVerifyKeys::Key_Retail_Slot0x3DKeyNormal_5;
						keyIdx[6]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_0;
						keyIdx[7]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_1;
						keyIdx[8]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_2;
						keyIdx[9]  = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_3;
						keyIdx[10] = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_4;
						keyIdx[11] = N3DSVerifyKeys::Key_Debug_Slot0x3DKeyNormal_5;
						break;
					default:
						break;
				}
				break;

			default:
				// Key is not supported.
				break;
		}

		if (keyIdx[0] < 0) {
			// Key is not supported.
			iret.keysNotUsed++;
			continue;
		}

		if (aesKey->isEncrypted) {
			// Key is encrypted.
			size = 0;
			if (cipher) {
				// Counter is the first 12 bytes of the AesKeyInfo struct.
				u128_t ctr;
				memcpy(ctr.u8, aesKey, 12);
				memset(&ctr.u8[12], 0, 4);
				size = cipher->decrypt(aesKey->key, sizeof(aesKey->key),
					ctr.u8, sizeof(ctr.u8));
			}
			if (size != sizeof(aesKey->key)) {
				// Unable to decrypt the key.
				// FIXME: This might result in the wrong number of
				// keys being reported in total.
				iret.keysCantDecrypt++;
				continue;
			}
		}

		// Check if the key is OK.
		bool keyChecked = false;
		for (int i = 0; i < ARRAY_SIZE(keyIdx); i++) {
			if (keyIdx[i] < 0)
				break;

			Key *const pKey = &d->keys[keyIdxStart + keyIdx[i]];
			if (pKey->status == Key::Status_OK) {
				// Key is already OK. Don't bother with it.
				iret.keysExist++;
				keyChecked = true;
				continue;
			}

			// Check if this key matches.
			const uint8_t *const verifyData = N3DSVerifyKeys::encryptionVerifyData_static(keyIdx[i]);
			if (verifyData) {
				// Verify the key.
				int ret = d->verifyKeyData(aesKey->key, verifyData, 16);
				if (ret == 0) {
					// Found a match!
					const string new_value = d->binToHexStr(aesKey->key, sizeof(aesKey->key));
					if (pKey->value != new_value) {
						pKey->value = new_value;
						pKey->status = KeyStoreUI::Key::Status_OK;
						pKey->modified = true;
						iret.keysImportedVerify++;
						wereKeysImported = true;
						emit keyChanged_int(KeyStoreUIPrivate::Section_N3DSVerifyKeys, keyIdx[i]);
						emit keyChanged_int(keyIdxStart + keyIdx[i]);
					} else {
						// No change.
						iret.keysExist++;
					}
					// Key can only match either Retail or Debug,
					// so we're done here.
					keyChecked = true;
					break;
				}
			} else {
				// Can't verify this key...
				// Import it anyway.
				const string new_value = d->binToHexStr(aesKey->key, sizeof(aesKey->key));
				if (pKey->value != new_value) {
					pKey->value = new_value;
					pKey->status = KeyStoreUI::Key::Status_Unknown;
					pKey->modified = true;
					iret.keysImportedNoVerify++;
					wereKeysImported = true;
					emit keyChanged_int(KeyStoreUIPrivate::Section_N3DSVerifyKeys, keyIdx[i]);
					emit keyChanged_int(keyIdxStart + keyIdx[i]);
				} else {
					// No change.
					iret.keysExist++;
				}
				// Can't determine if this is Retail or Debug,
				// so continue anyway.
				keyChecked = true;
			}
		}

		if (!keyChecked) {
			// Key didn't match.
			iret.keysInvalid++;
		}
	} while (++aesKey != aesKeyEnd);

	if (wereKeysImported) {
		emit modified_int();
	}
	iret.status = (wereKeysImported
		? KeyStoreUI::Import_KeysImported
		: KeyStoreUI::Import_NoKeysImported);
	return iret;
}

}

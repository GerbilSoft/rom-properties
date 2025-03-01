/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KeyStoreUI.cpp: Key store UI base class.                                *
 *                                                                         *
 * Copyright (c) 2012-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyStoreUI.hpp"

// Other rom-properties libraries
#include "librpbase/crypto/KeyManager.hpp"
#include "librpbase/crypto/IAesCipher.hpp"
#include "librpbase/crypto/AesCipherFactory.hpp"
#include "librpbase/crypto/Hash.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;

// libromdata
#include "../Console/WiiTicket.hpp"
#include "../crypto/CtrKeyScrambler.hpp"
#include "../crypto/N3DSVerifyKeys.hpp"
#include "../Console/Xbox360_XEX.hpp"
using namespace LibRomData;

// C++ STL classes
using std::array;
using std::string;
using std::u16string;
using std::unique_ptr;
using std::vector;

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
	// Keys
	vector<KeyStoreUI::Key> keys;

	// Sections
	struct Section {
		//string name;		// NOTE: Not actually used...
		int keyIdxStart;	// Starting index in keys
		int keyCount;		// Number of keys
	};
	vector<Section> sections;

	// IAesCipher for verifying keys
	unique_ptr<IAesCipher> cipher;

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
	 * @param str UTF-8 string [NULL-terminated]
	 * @return Converted string [ASCII], or empty string on error.
	 */
	static string convertKanjiToHex(const char *str);

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
	// Section enumeration
	enum class SectionID {
		WiiTicket	= 0,
		CtrKeyScrambler	= 1,
		N3DSVerifyKeys	= 2,
		Xbox360_XEX	= 3,
	};

	struct KeyBinAddress {
		uint32_t address;
		int keyIdx;		// -1 for end of list.
	};

	/**
	 * Import keys from a binary blob.
	 * @param sectIdx	[in] Section index
	 * @param kba		[in] KeyBinAddress array
	 * @param kba_len	[in] Length of kba (in KeyBinAddress units)
	 * @param buf		[in] Key buffer
	 * @param buf_len	[in] Length of buf (in bytes)
	 * @return Key import status
	 */
	ATTR_ACCESS_SIZE(read_only, 3, 4)
	ATTR_ACCESS_SIZE(read_only, 5, 6)
	KeyStoreUI::ImportReturn importKeysFromBlob(SectionID sectIdx,
		const KeyBinAddress *kba, size_t kba_len,
		const uint8_t *buf, size_t buf_len);

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

	static const array<KeyStoreUIPrivate::EncKeyFns_t, 4> encKeyFns;

public:
	// Hexadecimal lookup table
	static const array<char, 16> hex_lookup;

	/**
	 * Convert a binary key to a hexadecimal string.
	 * @param data	[in] Binary key
	 * @param len	[in] Length of binary key, in bytes
	 * @return Hexadecimal string
	 */
	static string binToHexStr(const uint8_t *data, unsigned int len);

public:
	/** Key import functions **/

	/**
	 * Import keys from Wii keys.bin. (BootMii format)
	 * @param file Opened keys.bin file
	 * @return Key import status
	 */
	KeyStoreUI::ImportReturn importWiiKeysBin(IRpFile *file);

	/**
	 * Import keys from Wii U otp.bin.
	 * @param file Opened otp.bin file
	 * @return Key import status
	 */
	KeyStoreUI::ImportReturn importWiiUOtpBin(IRpFile *file);

	/**
	 * Import keys from 3DS boot9.bin.
	 * @param file Opened boot9.bin file
	 * @return Key import status
	 */
	KeyStoreUI::ImportReturn importN3DSboot9bin(IRpFile *file);

	/**
	 * Import keys from 3DS aeskeydb.bin.
	 * @param file Opened aeskeydb.bin file
	 * @return Key import status
	 */
	KeyStoreUI::ImportReturn importN3DSaeskeydb(IRpFile *file);
};

/** KeyStoreUIPrivate **/

const array<KeyStoreUIPrivate::EncKeyFns_t, 4> KeyStoreUIPrivate::encKeyFns = {{
	ENCKEYFNS(WiiTicket),
	ENCKEYFNS(CtrKeyScrambler),
	ENCKEYFNS(N3DSVerifyKeys),
	ENCKEYFNS(Xbox360_XEX),
}};

// Hexadecimal lookup table
const array<char, 16> KeyStoreUIPrivate::hex_lookup = {{
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F',
}};

KeyStoreUIPrivate::KeyStoreUIPrivate(KeyStoreUI *q)
	: q_ptr(q)
	, changed(false)
	, cipher(AesCipherFactory::create())
{
	// Make sure the cipher is usable.
	if (cipher->isInit()) {
		// Set the cipher parameters.
		cipher->setChainingMode(IAesCipher::ChainingMode::ECB);
	} else {
		// Cipher is not usable.
		// We won't be able to verify keys.
		cipher.reset();
	}

	// Load the key names from the various classes.
	// Values will be loaded later.
	sections.clear();
	sections.resize(encKeyFns.size());
	keys.clear();

	int keyIdxStart = 0;
	auto sectIter = sections.begin();
	for (auto encSysIter = encKeyFns.cbegin(); encSysIter != encKeyFns.cend(); ++encSysIter, ++sectIter) {
		const EncKeyFns_t &encSys = *encSysIter;
		const int keyCount = encSys.pfnKeyCount();
		assert(keyCount > 0);
		if (keyCount <= 0)
			continue;

		// Set up the section.
		auto &section = *sectIter;
		section.keyIdxStart = keyIdxStart;
		section.keyCount = keyCount;

		// Increment keyIdxStart for the next section.
		keyIdxStart += keyCount;

		// Get the keys.
		const size_t prevKeyCount = keys.size();
		size_t totalKeyCount = prevKeyCount + keyCount;
		keys.resize(totalKeyCount);
		auto keyIter = keys.begin() + prevKeyCount;
		for (int i = 0; i < keyCount; i++, ++keyIter) {
			// Key name (ASCII)
			const char *const keyName = encSys.pfnKeyName(i);
			assert(keyName != nullptr);
			if (!keyName) {
				// Skip NULL key names. (This shouldn't happen...)
				totalKeyCount--;
				continue;
			}

			auto &key = *keyIter;
			key.name = latin1_to_utf8(keyName, -1);

			// Key is empty initially.
			key.status = KeyStoreUI::Key::Status::Empty;
			key.modified = false;

			// Allow kanji for twl-scrambler.
			key.allowKanji = (key.name == "twl-scrambler");
		}

		// If any keys were skipped, shrink the key vector.
		// NOTE: This shouldn't happen...
		if (keys.size() != totalKeyCount) {
			keys.resize(totalKeyCount);
		}
	}

	// Keys will NOT be auto-loaded due to multiple inheritance issues.
	// The subclass must load the keys.
	//reset();
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
	for (int encSysNum = 0; encSysNum < static_cast<int>(encKeyFns.size()); encSysNum++) {
		const KeyStoreUIPrivate::EncKeyFns_t &encSys = encKeyFns[encSysNum];
		const int keyCount = encSys.pfnKeyCount();
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
			const char *const keyName = encSys.pfnKeyName(i);
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
				case KeyManager::VerifyResult::OK:
					// Convert the key to a string.
					assert(keyData.key != nullptr);
					assert(keyData.length > 0);
					assert(keyData.length <= 32);
					if (keyData.key != nullptr && keyData.length > 0 && keyData.length <= 32) {
						const string value = binToHexStr(keyData.key, keyData.length);
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
						pKey->status = KeyStoreUI::Key::Status::NotAKey;
					}
					break;

				case KeyManager::VerifyResult::KeyInvalid:
					// Key is invalid. (i.e. not in the correct format)
					if (!pKey->value.empty()) {
						pKey->value.clear();
						hasChanged = true;
					}
					pKey->status = KeyStoreUI::Key::Status::NotAKey;
					break;

				default:
					// Assume the key wasn't found.
					if (!pKey->value.empty()) {
						pKey->value.clear();
						hasChanged = true;
					}
					pKey->status = KeyStoreUI::Key::Status::Empty;
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
	assert(sectIdx < static_cast<int>(sections.size()));
	if (sectIdx < 0 || sectIdx >= static_cast<int>(sections.size()))
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
	assert(idx < static_cast<int>(keys.size()));
	if (idx < 0 || idx >= static_cast<int>(keys.size()))
		return -ERANGE;

	// Figure out what section this key is in.
	for (int i = 0; i < static_cast<int>(sections.size()); i++) {
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
 * @param sectIdx	[in] Section index
 * @param kba		[in] KeyBinAddress array
 * @param kba_len	[in] Length of kba (in KeyBinAddress units)
 * @param buf		[in] Key buffer
 * @param buf_len	[in] Length of buf (in bytes)
 * @return Key import status
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
ATTR_ACCESS_SIZE(read_only, 5, 6)
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importKeysFromBlob(SectionID sectIdx,
	const KeyBinAddress *kba, size_t kba_len,
	const uint8_t *buf, size_t buf_len)
{
	KeyStoreUI::ImportReturn iret = {KeyStoreUI::ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	assert(static_cast<int>(sectIdx) >= 0);
	assert(static_cast<size_t>(sectIdx) < sections.size());
	assert(kba != nullptr);
	assert(kba_len != 0);
	assert(buf != nullptr);
	assert(buf_len != 0);
	if (static_cast<int>(sectIdx) < 0 || static_cast<size_t>(sectIdx) >= sections.size() ||
	    !kba || kba_len == 0 || !buf || buf_len == 0)
	{
		iret.status = KeyStoreUI::ImportStatus::InvalidParams;
		return iret;
	}

	RP_Q(KeyStoreUI);
	bool wereKeysImported = false;
	const int keyIdxStart = sections[static_cast<size_t>(sectIdx)].keyIdxStart;
	const KeyBinAddress *const kba_end = &kba[kba_len];
	for (; kba < kba_end; kba++) {
		KeyStoreUI::Key *const pKey = &keys[keyIdxStart + kba->keyIdx];
		if (pKey->status == KeyStoreUI::Key::Status::OK) {
			// Key is already OK. Don't bother with it.
			iret.keysExist++;
			continue;
		}
		assert(kba->address + 16 < buf_len);
		if (kba->address + 16 > buf_len) {
			// Out of range...
			// FIXME: Report an error?
			continue;
		}
		const uint8_t *const keyData = &buf[kba->address];

		// Check if the key in the binary file is correct.
		const uint8_t *const verifyData = encKeyFns[static_cast<size_t>(sectIdx)].pfnVerifyData(kba->keyIdx);
		assert(verifyData != nullptr);
		if (!verifyData) {
			// Can't verify this key...
			// Import it anyway.
			const string new_value = binToHexStr(keyData, 16);
			if (pKey->value != new_value) {
				pKey->value = new_value;
				pKey->status = KeyStoreUI::Key::Status::Unknown;
				pKey->modified = true;
				iret.keysImportedNoVerify++;
				wereKeysImported = true;
				emit q->keyChanged_int(static_cast<int>(sectIdx), kba->keyIdx);
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
				pKey->status = KeyStoreUI::Key::Status::OK;
				pKey->modified = true;
				iret.keysImportedVerify++;
				wereKeysImported = true;
				emit q->keyChanged_int(static_cast<int>(sectIdx), kba->keyIdx);
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
		this->changed = true;
		emit q->modified_int();
	}
	iret.status = (wereKeysImported
		? KeyStoreUI::ImportStatus::KeysImported
		: KeyStoreUI::ImportStatus::NoKeysImported);
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
	const Section &sectScrambler = sections[static_cast<size_t>(SectionID::CtrKeyScrambler)];
	const KeyStoreUI::Key &ctr_scrambler = keys[sectScrambler.keyIdxStart + static_cast<size_t>(CtrKeyScrambler::EncryptionKeys::Key_Ctr_Scrambler)];
	if (ctr_scrambler.status != KeyStoreUI::Key::Status::OK) {
		// Key is not correct.
		return -ENOENT;
	}

	// Get Slot0x2CKeyX.
	const Section &sectN3DS = sections[static_cast<size_t>(SectionID::N3DSVerifyKeys)];
	const KeyStoreUI::Key &key_slot0x2CKeyX = keys[sectN3DS.keyIdxStart + static_cast<size_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x2CKeyX)];
	if (key_slot0x2CKeyX.status != KeyStoreUI::Key::Status::OK) {
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
 * @param str UTF-8 string [NULL-terminated]
 * @return Converted string [ASCII], or empty string on error.
 */
string KeyStoreUIPrivate::convertKanjiToHex(const char *str)
{
	// Check for non-ASCII characters.
	// TODO: Also check for non-hex digits?
	bool hasNonAscii = false;
	for (const char *p = str; *p != '\0'; p++) {
		// The following check works for both UTF-8 and UTF-16.
		// If the character value is >= 128, it's non-ASCII.
		if (static_cast<unsigned int>(*p) >= 128) {
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
	const u16string u16str = utf8_to_utf16(str, -1);

	// Convert to a UTF-16LE hex string, starting with U+FEFF.
	// TODO: Combine with the first loop?
	const size_t hexlen = 4+(u16str.size()*4);
	unique_ptr<char[]> hexstr(new char[hexlen]);
	char *pHex = hexstr.get();
	memcpy(pHex, "FFFE", 4);
	pHex += 4;
	for (const char16_t *pu16 = u16str.c_str(); *pu16 != '\0'; pu16++, pHex += 4) {
		const char16_t u16 = *pu16;
		pHex[0] = hex_lookup[(u16 >>  4) & 0x0F];
		pHex[1] = hex_lookup[(u16 >>  0) & 0x0F];
		pHex[2] = hex_lookup[(u16 >> 12) & 0x0F];
		pHex[3] = hex_lookup[(u16 >>  8) & 0x0F];
	}

	return {hexstr.get(), hexlen};
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
	array<uint8_t, 16> testData;
	memcpy(testData.data(), verifyData, testData.size());
	int ret = cipher->setKey(keyData, len);
	if (ret != 0) {
		// Error setting the key.
		return -EIO;
	}
	size_t size = cipher->decrypt(testData.data(), testData.size());
	if (size != testData.size()) {
		// Error decrypting the data.
		return -EIO;
	}

	// Check if the decrypted data is correct.
	return (memcmp(testData.data(), KeyManager::verifyTestString, testData.size()) != 0);
}

/**
 * Verify a key and update its status.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreUIPrivate::verifyKey(int sectIdx, int keyIdx)
{
	const int idx = sectKeyToIdx(sectIdx, keyIdx);
	if (idx < 0)
		return;

	// Check the key length.
	KeyStoreUI::Key &key = keys[idx];
	if (key.value.empty()) {
		// Empty key.
		key.status = KeyStoreUI::Key::Status::Empty;
		return;
	} else if (key.value.size() != 16 && key.value.size() % 2 != 0) {
		// Invalid length.
		// TODO: Support keys that aren't 128-bit.
		key.status = KeyStoreUI::Key::Status::NotAKey;
		return;
	}

	if (!cipher) {
		// Cipher is unavailable.
		// Cannot verify the key.
		key.status = KeyStoreUI::Key::Status::Unknown;
		return;
	}

	// Get the key verification data. (16 bytes)
	const uint8_t *const verifyData = encKeyFns[sectIdx].pfnVerifyData(keyIdx);
	if (!verifyData) {
		// No key verification data is available.
		key.status = KeyStoreUI::Key::Status::Unknown;
		return;
	}

	// Convert the key to bytes.
	// TODO: Support keys that aren't 128-bit.
	array<uint8_t, 16> keyBytes;
	int ret = KeyManager::hexStringToBytes(key.value.c_str(), keyBytes.data(), keyBytes.size());
	if (ret != 0) {
		// Invalid character(s) encountered.
		key.status = KeyStoreUI::Key::Status::NotAKey;
		return;
	}

	// Verify the key.
	ret = verifyKeyData(keyBytes.data(), verifyData, keyBytes.size());
	if (ret == 0) {
		// Decrypted data is correct.
		key.status = KeyStoreUI::Key::Status::OK;
	} else {
		// Decrypted data is wrong.
		key.status = KeyStoreUI::Key::Status::Incorrect;
	}
}

/**
 * Convert a binary key to a hexadecimal string.
 * @param data	[in] Binary key
 * @param len	[in] Length of binary key, in bytes
 * @return Hexadecimal string
 */
string KeyStoreUIPrivate::binToHexStr(const uint8_t *data, unsigned int len)
{
	assert(data != nullptr);
	assert(len > 0);
	assert(len <= 64);
	if (!data || len == 0 || len > 64)
		return {};

	const unsigned int hexlen = len*2;
	unique_ptr<char[]> hexstr(new char[hexlen]);
	char *pHex = hexstr.get();
	for (; len > 0; len--, data++, pHex += 2) {
		pHex[0] = hex_lookup[*data >> 4];
		pHex[1] = hex_lookup[*data & 0x0F];
	}

	return {hexstr.get(), hexlen};
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
	return static_cast<int>(d->sections.size());
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
	assert(sectIdx < static_cast<int>(d->sections.size()));
	assert(sectIdx < static_cast<int>(d->encKeyFns.size()));
	if (sectIdx < 0 || sectIdx >= static_cast<int>(d->sections.size()) ||
		sectIdx >= static_cast<int>(d->encKeyFns.size()))
	{
		return nullptr;
	}

	static const array<const char*, 4> sectNames = {{
		NOP_C_("KeyStoreUI|Section", "Nintendo Wii AES Keys"),
		NOP_C_("KeyStoreUI|Section", "Nintendo 3DS Key Scrambler Constants"),
		NOP_C_("KeyStoreUI|Section", "Nintendo 3DS AES Keys"),
		NOP_C_("KeyStoreUI|Section", "Microsoft Xbox 360 AES Keys"),
	}};
	static_assert(sectNames.size() == KeyStoreUIPrivate::encKeyFns.size(),
		"sectNames[] is out of sync with d->encKeyFns[].");

	return pgettext_expr("KeyStoreUI|Section", sectNames[sectIdx]);
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
	assert(sectIdx < static_cast<int>(d->sections.size()));
	if (sectIdx < 0 || sectIdx >= static_cast<int>(d->sections.size()))
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
	int ret = std::accumulate(d->sections.cbegin(), d->sections.cend(), 0,
		[](int a, const KeyStoreUIPrivate::Section &section) noexcept -> int {
			return a + section.keyCount;
		}
	);
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
	const int idx = d->sectKeyToIdx(sectIdx, keyIdx);
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
	assert(idx < static_cast<int>(d->keys.size()));
	if (idx < 0 || idx >= static_cast<int>(d->keys.size()))
		return nullptr;
	return &d->keys[idx];
}

/**
 * Set a key's value.
 * If successful, and the new value is different,
 * keyChanged() will be emitted.
 *
 * @param sectIdx Section index
 * @param keyIdx Key index
 * @param value New value [NULL-terminated UTF-8 string]
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUI::setKey(int sectIdx, int keyIdx, const char *value)
{
	assert(value != nullptr);
	if (!value) {
		return -EINVAL;
	}

	RP_D(KeyStoreUI);
	const int idx = d->sectKeyToIdx(sectIdx, keyIdx);
	if (idx < 0) {
		return -ERANGE;
	}

	// Expected key length, in hex digits.
	// TODO: Support more than 128-bit keys.
	static constexpr size_t expected_key_len = 16*2;

	// If allowKanji is true, check if the key is kanji
	// and convert it to UTF-16LE hexadecimal.
	Key &key = d->keys[idx];
	string new_value;
	if (key.allowKanji) {
		// Convert kanji to hexadecimal if needed.
		// NOTE: convertKanjiToHex() returns an empty string on error,
		// so if the original string is empty, don't do anything.
		// NOTE 2: convertKanjiToHex() only errors if the string is
		// non-ASCII and cannot be converted properly, so valid hex
		// strings will always return the original string.
		if (value[0] != '\0') {
			string convKey = d->convertKanjiToHex(value);
			if (convKey.empty()) {
				// Invalid kanji key.
				return -EINVAL;
			}

			// Truncate the key if necessary.
			if (convKey.size() > expected_key_len) {
				convKey.resize(expected_key_len);
			}
			new_value = convKey;
		}
	}

	if (new_value.empty()) {
		// Hexadecimal only.
		// NOTE: We only want up to expected_key_len.
		size_t value_len = strlen(value);
		if (unlikely(value_len > expected_key_len)) {
			value_len = expected_key_len;
		}
		new_value.resize(value_len);

		// Validate hex digits and convert to uppercase.
		auto iter_dest = new_value.begin();
		const auto new_value_end = new_value.end();
		for (; *value != '\0' && iter_dest != new_value_end; ++value, ++iter_dest) {
			const char chr = *value;
			if (!ISXDIGIT(chr)) {
				// Not a hex digit.
				return -EINVAL;
			}
			*iter_dest = TOUPPER(chr);
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
 * @param idx Flat key index
 * @param value New value [NULL-terminated UTF-8 string]
 * @return 0 on success; non-zero on error.
 */
int KeyStoreUI::setKey(int idx, const char *value)
{
	// Convert to section/key index format first.
	// NOTE: The other setKey() overload converts it
	// back to flat index format. Maybe we should
	// make a third (protected) function that takes
	// all three...
	RP_D(KeyStoreUI);
	int sectIdx = -1, keyIdx = -1;
	int ret = d->idxToSectKey(idx, &sectIdx, &keyIdx);
	assert(ret == 0);
	assert(sectIdx >= 0);
	assert(keyIdx >= 0);
	if (ret == 0 && sectIdx >= 0 && keyIdx >= 0) {
		return setKey(sectIdx, keyIdx, value);
	}
	return -ERANGE;
}

/**
 * Mark all keys as saved.
 * This clears the "modified" field.
 */
void KeyStoreUI::allKeysSaved(void)
{
	RP_D(KeyStoreUI);
	for (KeyStoreUI::Key &key : d->keys) {
		key.modified = false;
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
 * @param file Opened keys.bin file
 * @return Key import status
 */
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importWiiKeysBin(IRpFile *file)
{
	KeyStoreUI::ImportReturn iret = {KeyStoreUI::ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	// File must be 1,024 bytes.
	if (file->size() != 1024) {
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// Read the entire 1,024 bytes.
	uint8_t buf[1024];
	size_t size = file->seekAndRead(0, buf, sizeof(buf));
	if (size != 1024) {
		// Read error.
		// TODO: file->lastError()?
		iret.status = KeyStoreUI::ImportStatus::ReadError;
		iret.error_code = static_cast<uint8_t>(file->lastError());
		return iret;
	}

	// Verify the BootMii (BackupMii) header.
	// TODO: Is there a v0? If this shows v0, show a different message.
	static constexpr char BackupMii_magic[] = "BackupMii v1";
	if (memcmp(buf, BackupMii_magic, sizeof(BackupMii_magic)-1) != 0) {
		// TODO: Check for v0.
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// TODO:
	// - SD keys are not present in keys.bin.

	static const array<KeyStoreUIPrivate::KeyBinAddress, 3> keyBinAddress = {{
		{0x114, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVL_Common)},
		{0x114, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVT_Debug)},
		{0x274, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVL_Korean)},
	}};

	// Import the keys.
	return importKeysFromBlob(KeyStoreUIPrivate::SectionID::WiiTicket,
		keyBinAddress.data(), keyBinAddress.size(), buf, sizeof(buf));
}

/**
 * Import keys from Wii U otp.bin.
 * @param file Opened otp.bin file
 * @return Key import status
 */
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importWiiUOtpBin(IRpFile *file)
{
	KeyStoreUI::ImportReturn iret = {KeyStoreUI::ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	// File must be 1,024 bytes.
	if (file->size() != 1024) {
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// Read the entire 1,024 bytes.
	uint8_t buf[1024];
	size_t size = file->seekAndRead(0, buf, sizeof(buf));
	if (size != 1024) {
		// Read error.
		iret.status = KeyStoreUI::ImportStatus::ReadError;
		iret.error_code = static_cast<uint8_t>(file->lastError());
		return iret;
	}

	// Verify the vWii Boot1 hash.
	// TODO: Are there multiple variants of vWii Boot1?
	bool isDebug;
	static constexpr array<uint8_t, 20> vWii_Boot1_hash_retail = {{
		0xF8,0xB1,0x8E,0xC3,0xFE,0x26,0xB9,0xB1,
		0x1A,0xD4,0xA4,0xED,0xD3,0xB7,0xA0,0x31,
		0x11,0x9A,0x79,0xF8
	}};
	static constexpr array<uint8_t, 20> vWii_Boot1_hash_debug = {{
		0x9C,0x43,0x35,0x08,0x0C,0xC7,0x57,0x4F,
		0xCD,0xDE,0x85,0xBF,0x21,0xF6,0xC9,0x7C,
		0x6C,0xAF,0xC1,0xDB
	}};
	if (!memcmp(&buf[0], vWii_Boot1_hash_retail.data(), vWii_Boot1_hash_retail.size())) {
		// Retail Boot1 hash matches.
		isDebug = false;
	} else if (!memcmp(&buf[0], vWii_Boot1_hash_debug.data(), vWii_Boot1_hash_debug.size())) {
		// Debug Boot1 hash matches.
		isDebug = true;
	} else {
		// Not a match.
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// Key addresses and indexes.
	// TODO:
	// - SD keys are not present in otp.bin.
	static const array<KeyStoreUIPrivate::KeyBinAddress, 3> keyBinAddress_retail = {{
		{0x014, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVL_Common)},
		{0x348, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVL_Korean)},
		{0x0D0, static_cast<int>(WiiTicket::EncryptionKeys::Key_WUP_Starbuck_vWii_Common)},

		// TODO: Import Wii U keys.
#if 0
		{0x090, /* Wii U ancast key */},
		{0x0E0, /* Wii U common key */},
#endif
	}};

	static const array<KeyStoreUIPrivate::KeyBinAddress, 3> keyBinAddress_debug = {{
		{0x014, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVT_Debug)},
		{0x348, static_cast<int>(WiiTicket::EncryptionKeys::Key_RVT_Korean)},
		{0x0D0, static_cast<int>(WiiTicket::EncryptionKeys::Key_CAT_Starbuck_vWii_Common)},

		// TODO: Import Wii U keys.
#if 0
		{0x090, /* Wii U ancast key */},
		{0x0E0, /* Wii U common key */},
#endif
	}};

	// Import the keys.
	if (likely(!isDebug)) {
		return importKeysFromBlob(KeyStoreUIPrivate::SectionID::WiiTicket,
			keyBinAddress_retail.data(), keyBinAddress_retail.size(), buf, sizeof(buf));
	} else {
		return importKeysFromBlob(KeyStoreUIPrivate::SectionID::WiiTicket,
			keyBinAddress_debug.data(), keyBinAddress_debug.size(), buf, sizeof(buf));
	}
}

/**
 * Import keys from 3DS boot9.bin.
 * @param file Opened boot9.bin file
 * @return Key import status
 */
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importN3DSboot9bin(IRpFile *file)
{
	KeyStoreUI::ImportReturn iret = {KeyStoreUI::ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	// File may be:
	// - 65,536 bytes: Unprotected + Protected boot9
	// - 32,768 bytes: Protected boot9 only
	const off64_t fileSize = file->size();
	if (fileSize != 65536 && fileSize != 32768) {
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// Read the protected section into memory.
	unique_ptr<array<uint8_t, 32768> > buf(new array<uint8_t, 32768>);
	if (fileSize == 65536) {
		// 64 KiB (Unprotected + Protected boot9)
		// Seek to the second half.
		int ret = file->seek(32768);
		if (ret != 0) {
			// Seek error.
			iret.status = KeyStoreUI::ImportStatus::ReadError;
			iret.error_code = static_cast<uint8_t>(file->lastError());
			return iret;
		}
	} else {
		// 32 KiB (Protected boot9.bin only)
		// Rewind to the beginning of the file.
		file->rewind();
	}
	size_t size = file->read(buf->data(), buf->size());
	if (size != 32768) {
		// Read error.
		iret.status = KeyStoreUI::ImportStatus::ReadError;
		iret.error_code = static_cast<uint8_t>(file->lastError());
		return iret;
	}

	Hash crc32Hash(Hash::Algorithm::CRC32);
	if (crc32Hash.isUsable()) {
		// Check the CRC32.
		// NOTE: CRC32 isn't particularly strong, so we'll still
		// verify the keys before importing them.
		crc32Hash.process(buf->data(), buf->size());
		const uint32_t crc = crc32Hash.getHash32();
		if (crc != 0x9D50A525) {
			// Incorrect CRC32.
			iret.status = KeyStoreUI::ImportStatus::InvalidFile;
			return iret;
		}
	}

	// Key addresses and indexes.
	static const array<KeyStoreUIPrivate::KeyBinAddress, 6> keyBinAddress = {{
		{0x5720, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_SpiBoot)},
		{0x59D0, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x2CKeyX)},
		{0x5A20, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyX)},

		{0x5740, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_SpiBoot)},
		{0x5DD0, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x2CKeyX)},
		{0x5E20, static_cast<int>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyX)},
	}};

	// Import the keys.
	return importKeysFromBlob(KeyStoreUIPrivate::SectionID::N3DSVerifyKeys,
		keyBinAddress.data(), keyBinAddress.size(), buf->data(), buf->size());
}

/**
 * Import keys from 3DS aeskeydb.bin.
 * @param file Opened aeskeydb.bin file
 * @return Key import status
 */
KeyStoreUI::ImportReturn KeyStoreUIPrivate::importN3DSaeskeydb(IRpFile *file)
{
	KeyStoreUI::ImportReturn iret = {KeyStoreUI::ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	// File must be <= 64 KB and a multiple of 32 bytes.
	const off64_t fileSize = file->size();
	if (fileSize == 0 || fileSize > 65536 || (fileSize % 32 != 0)) {
		iret.status = KeyStoreUI::ImportStatus::InvalidFile;
		return iret;
	}

	// Read the entire file into memory.
	unique_ptr<uint8_t[]> buf(new uint8_t[static_cast<size_t>(fileSize)]);
	size_t size = file->seekAndRead(0, buf.get(), static_cast<size_t>(fileSize));
	if (size != static_cast<size_t>(fileSize)) {
		// Read error.
		iret.status = KeyStoreUI::ImportStatus::ReadError;
		iret.error_code = static_cast<uint8_t>(file->lastError());
		return iret;
	}

	// aeskeydb keyslot from Decrypt9WIP.
	// NOTE: Decrypt9WIP and SafeB9SInstaller interpret the "keyUnitType" field differently.
	// - Decrypt9WIP: isDevkitKey == 0 for retail, non-zero for debug
	// - SafeB9SInstaller: keyUnitType == 0 for ALL units, 1 for retail only, 2 for debug only
	// To prevent issues, we'll check both retail and debug for all keys.
	struct AesKeyInfo {
		uint8_t slot;		// keyslot, 0x00...0x3F 
		char type;		// type 'X' / 'Y' / 'N' for normalKey / 'I' for IV
		char id[10];		// key ID for special keys, all zero for standard keys
		uint8_t reserved[2];	// reserved space
		uint8_t keyUnitType;	// see above
		uint8_t isEncrypted;	// if non-zero, key is encrypted using Slot0x2C, with KeyY = 0
		uint8_t key[16];
	};
	ASSERT_STRUCT(AesKeyInfo, 32);

	// Slot0x2CKeyX is needed to decrypt keys if the
	// aeskeydb.bin file is encrypted.
	unique_ptr<IAesCipher> cipher;
	u128_t aeskeydb_key;
	if (getAesKeyDB_key(&aeskeydb_key) == 0) {
		// TODO: Support for debug-encrypted aeskeydb.bin?
		cipher.reset(AesCipherFactory::create());
		cipher->setChainingMode(IAesCipher::ChainingMode::CTR);
		cipher->setKey(aeskeydb_key.u8, sizeof(aeskeydb_key.u8));
	}

	RP_Q(KeyStoreUI);
	AesKeyInfo *aesKey = reinterpret_cast<AesKeyInfo*>(buf.get());
	const AesKeyInfo *const aesKeyEnd = reinterpret_cast<const AesKeyInfo*>(buf.get() + fileSize);
	const int keyIdxStart = sections[static_cast<size_t>(KeyStoreUIPrivate::SectionID::N3DSVerifyKeys)].keyIdxStart;
	bool wereKeysImported = false;
	do {
		// Check if this is a supported keyslot.
		// Key indexes: 0 == retail, 1 == debug
		// except for Slot0x3DKeyY, which has 6 of each
		size_t keyCount = 0;
		const uint8_t *pKeyIdx = nullptr;
		switch (aesKey->slot) {
			case 0x18:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					static constexpr array<uint8_t, 2> keys_Slot0x18KeyX = {{
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x18KeyX),
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x18KeyX),
					}};
					keyCount = keys_Slot0x18KeyX.size();
					pKeyIdx = keys_Slot0x18KeyX.data();
				}
				break;

			case 0x1B:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					static constexpr array<uint8_t, 2> keys_Slot0x1BKeyX = {{
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x1BKeyX),
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x1BKeyX),
					}};
					keyCount = keys_Slot0x1BKeyX.size();
					pKeyIdx = keys_Slot0x1BKeyX.data();
				}
				break;

			case 0x25:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					static constexpr array<uint8_t, 2> keys_Slot0x25KeyX = {{
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x25KeyX),
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x25KeyX),
					}};
					keyCount = keys_Slot0x25KeyX.size();
					pKeyIdx = keys_Slot0x25KeyX.data();
				}
				break;

			case 0x2C:
				// Only KeyX is available for this key.
				// KeyY is taken from the title's RSA signature.
				if (aesKey->type == 'X') {
					static constexpr array<uint8_t, 2> keys_Slot0x2CKeyX = {{
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x2CKeyX),
						static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x2CKeyX),
					}};
					keyCount = keys_Slot0x2CKeyX.size();
					pKeyIdx = keys_Slot0x2CKeyX.data();
				}
				break;

			case 0x3D:
				// KeyX, KeyY, and KeyNormal are available.
				switch (aesKey->type) {
					case 'X': {
						static constexpr array<uint8_t, 2> keys_Slot0x3DKeyX = {{
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyX),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyX),
						}};
						keyCount = keys_Slot0x3DKeyX.size();
						pKeyIdx = keys_Slot0x3DKeyX.data();
						break;
					}
					case 'Y': {
						static constexpr array<uint8_t, 12> keys_Slot0x3DKeyY = {{
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_0),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_1),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_2),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_3),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_4),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyY_5),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_0),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_1),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_2),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_3),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_4),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyY_5),
						}};
						keyCount = keys_Slot0x3DKeyY.size();
						pKeyIdx = keys_Slot0x3DKeyY.data();
						break;
					}
					case 'N': {
						static constexpr array<uint8_t, 12> keys_Slot0x3DKeyNormal = {{
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_0),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_1),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_2),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_3),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_4),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Retail_Slot0x3DKeyNormal_5),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_0),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_1),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_2),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_3),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_4),
							static_cast<uint8_t>(N3DSVerifyKeys::EncryptionKeys::Key_Debug_Slot0x3DKeyNormal_5),
						}};
						keyCount = keys_Slot0x3DKeyNormal.size();
						pKeyIdx = keys_Slot0x3DKeyNormal.data();
						break;
					}
					default:
						break;
				}
				break;

			default:
				// Key is not supported.
				break;
		}

		if (!pKeyIdx) {
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
		for (size_t i = keyCount; i > 0; i--, pKeyIdx++) {
			KeyStoreUI::Key *const pKey = &keys[keyIdxStart + *pKeyIdx];
			if (pKey->status == KeyStoreUI::Key::Status::OK) {
				// Key is already OK. Don't bother with it.
				iret.keysExist++;
				keyChecked = true;
				continue;
			}

			// Check if this key matches.
			const uint8_t *const verifyData = N3DSVerifyKeys::encryptionVerifyData_static(*pKeyIdx);
			if (verifyData) {
				// Verify the key.
				int ret = verifyKeyData(aesKey->key, verifyData, 16);
				if (ret == 0) {
					// Found a match!
					const string new_value = binToHexStr(aesKey->key, sizeof(aesKey->key));
					if (pKey->value != new_value) {
						pKey->value = new_value;
						pKey->status = KeyStoreUI::Key::Status::OK;
						pKey->modified = true;
						iret.keysImportedVerify++;
						wereKeysImported = true;
						emit q->keyChanged_int(static_cast<int>(KeyStoreUIPrivate::SectionID::N3DSVerifyKeys), *pKeyIdx);
						emit q->keyChanged_int(keyIdxStart + *pKeyIdx);
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
				const string new_value = binToHexStr(aesKey->key, sizeof(aesKey->key));
				if (pKey->value != new_value) {
					pKey->value = new_value;
					pKey->status = KeyStoreUI::Key::Status::Unknown;
					pKey->modified = true;
					iret.keysImportedNoVerify++;
					wereKeysImported = true;
					emit q->keyChanged_int(static_cast<int>(KeyStoreUIPrivate::SectionID::N3DSVerifyKeys), *pKeyIdx);
					emit q->keyChanged_int(keyIdxStart + *pKeyIdx);
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
		emit q->modified_int();
	}
	iret.status = (wereKeysImported
		? KeyStoreUI::ImportStatus::KeysImported
		: KeyStoreUI::ImportStatus::NoKeysImported);
	return iret;
}

/**
 * Import keys from a binary file.
 * @param fileID	[in] Type of file
 * @param file		[in] Opened file
 * @return ImportReturn
 */
KeyStoreUI::ImportReturn KeyStoreUI::importKeysFromBin(ImportFileID fileID, IRpFile *file)
{
	RP_D(KeyStoreUI);
	switch (fileID) {
		case ImportFileID::WiiKeysBin:
			return d->importWiiKeysBin(file);
		case ImportFileID::WiiUOtpBin:
			return d->importWiiUOtpBin(file);
		case ImportFileID::N3DSboot9bin:
			return d->importN3DSboot9bin(file);
		case ImportFileID::N3DSaeskeydb:
			return d->importN3DSaeskeydb(file);
		default:
			assert(!"Invalid file ID");
			break;
	}

	// Unknown key ID.
	// TODO: Mark this as static const? Not sure if that's
	// actually beneficial here.
	ImportReturn iret = {KeyStoreUI::ImportStatus::UnknownKeyID, 0, 0, 0, 0, 0, 0, 0};
	return iret;
}

/**
 * Import keys from a binary file.
 * @param fileID	[in] Type of file
 * @param filename	[in] Filename (UTF-8)
 * @return ImportReturn
 */
KeyStoreUI::ImportReturn KeyStoreUI::importKeysFromBin(ImportFileID fileID, const char *filename)
{
	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file->isOpen()) {
		const ImportReturn iret = {ImportStatus::OpenError, static_cast<uint8_t>(file->lastError()), 0, 0, 0, 0, 0, 0};
		return iret;
	}

	return importKeysFromBin(fileID, file.get());
}

#ifdef _WIN32
/**
 * Import keys from a binary file.
 * @param fileID	[in] Type of file
 * @param filenameW	[in] Filename (UTF-16)
 * @return ImportReturn
 */
KeyStoreUI::ImportReturn KeyStoreUI::importKeysFromBin(ImportFileID fileID, const wchar_t *filenameW)
{
	ImportReturn iret = {ImportStatus::InvalidParams, 0, 0, 0, 0, 0, 0, 0};

	unique_ptr<RpFile> file(new RpFile(filenameW, RpFile::FM_OPEN_READ));
	if (!file->isOpen()) {
		// TODO: file->lastError()?
		iret.status = ImportStatus::OpenError;
		iret.error_code = static_cast<uint8_t>(file->lastError());
		return iret;
	}

	return importKeysFromBin(fileID, file.get());
}
#endif /* _WIN32 */

}

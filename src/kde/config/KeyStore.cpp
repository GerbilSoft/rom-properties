/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStore.cpp: Key store object for Qt.                                  *
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

#include "KeyStore.hpp"
#include "RpQt.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/crypto/KeyManager.hpp"
#include "librpbase/crypto/IAesCipher.hpp"
#include "librpbase/crypto/AesCipherFactory.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/crypto/N3DSVerifyKeys.hpp"
using namespace LibRomData;

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// Qt includes.
#include <QtCore/QVector>

class KeyStorePrivate
{
	public:
		explicit KeyStorePrivate(KeyStore *q);
		~KeyStorePrivate();

	private:
		KeyStore *const q_ptr;
		Q_DECLARE_PUBLIC(KeyStore)
		Q_DISABLE_COPY(KeyStorePrivate)

	public:
		// Has the user changed anything?
		// This specifically refers to *user* settings.
		// reset() will emit dataChanged(), but changed
		// will be set back to false.
		bool changed;

	public:
		// Keys.
		QVector<KeyStore::Key> keys;

		// Sections.
		struct Section {
			QString name;
			int keyIdxStart;	// Starting index in keys.
			int keyCount;		// Number of keys.
		};
		QVector<Section> sections;

		// IAesCipher for verifying keys.
		IAesCipher *cipher;

		/**
		 * Convert a flat key index to sectIdx/keyIdx.
		 * @param idx		[in] Flat key index.
		 * @param sectIdx	[out] Section index.
		 * @param keyIdx	[out] Key index.
		 * @return True on success; false on error.
		 */
		bool flatKeyToSectKey(int idx, int &sectIdx, int &keyIdx);

	public:
		/**
		 * (Re-)Load the keys from keys.conf.
		 */
		void reset(void);

	public:
		/**
		 * Convert a string that may contain kanji to hexadecimal.
		 * @param str String.
		 * @return Converted string, or empty string on error.
		 */
		static QString convertKanjiToHex(const QString &str);

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

	private:
		// TODO: Share with rpcli/verifykeys.cpp.
		// TODO: Central registration of key verification functions?
		typedef int (*pfnKeyCount_t)(void);
		typedef const char* (*pfnKeyName_t)(int keyIdx);
		typedef const uint8_t* (*pfnVerifyData_t)(int keyIdx);

		struct EncKeyFns_t {
			pfnKeyCount_t pfnKeyCount;
			pfnKeyName_t pfnKeyName;
			pfnVerifyData_t pfnVerifyData;
			const rp_char *sectName;
		};

		#define ENCKEYFNS(klass, sectName) { \
			klass::encryptionKeyCount_static, \
			klass::encryptionKeyName_static, \
			klass::encryptionVerifyData_static, \
			sectName \
		}

		static const EncKeyFns_t encKeyFns[];

		// Hexadecimal lookup table.
		static const char hex_lookup[16];
};

/** KeyStorePrivate **/

const KeyStorePrivate::EncKeyFns_t KeyStorePrivate::encKeyFns[] = {
	ENCKEYFNS(WiiPartition,    _RP("Nintendo Wii AES Keys")),
	ENCKEYFNS(CtrKeyScrambler, _RP("Nintendo 3DS Key Scrambler Constants")),
	ENCKEYFNS(N3DSVerifyKeys,  _RP("Nintendo 3DS AES Keys")),
};

// Hexadecimal lookup table.
const char KeyStorePrivate::hex_lookup[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F',
};

KeyStorePrivate::KeyStorePrivate(KeyStore *q)
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

	Section section;
	KeyStore::Key key;
	int keyIdxStart = 0;
	for (int encSysNum = 0; encSysNum < ARRAY_SIZE(encKeyFns); encSysNum++) {
		const EncKeyFns_t *const encSys = &encKeyFns[encSysNum];
		const int keyCount = encSys->pfnKeyCount();
		assert(keyCount > 0);
		if (keyCount <= 0)
			continue;

		// Set up the section.
		section.name = RP2Q(encSys->sectName);
		section.keyIdxStart = keyIdxStart;
		section.keyCount = keyCount;
		sections.append(section);

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
			key.name = QLatin1String(keyName);

			// Key is empty initially.
			key.status = KeyStore::Key::Status_Empty;

			// Allow kanji for twl-scrambler.
			key.allowKanji = (key.name == QLatin1String("twl-scrambler"));

			// Save the key.
			// TODO: Edit a Key struct directly in the QVector?
			keys.append(key);
		}
	}

	// Load the keys.
	reset();
}

KeyStorePrivate::~KeyStorePrivate()
{
	delete cipher;
}

/**
 * Convert a flat key index to sectIdx/keyIdx.
 * @param idx		[in] Flat key index.
 * @param sectIdx	[out] Section index.
 * @param keyIdx	[out] Key index.
 * @return True on success; false on error.
 */
bool KeyStorePrivate::flatKeyToSectKey(int idx, int &sectIdx, int &keyIdx)
{
	assert(idx >= 0);
	assert(idx < keys.size());
	if (idx < 0 || idx >= keys.size())
		return false;

	// Figure out what section this key is in.
	for (int i = 0; i < sections.size(); i++) {
		const KeyStorePrivate::Section &section = sections[i];
		if (idx < (section.keyIdxStart + section.keyCount)) {
			// Found the section.
			sectIdx = i;
			keyIdx = idx - section.keyIdxStart;
			return true;
		}
	}

	// Not found.
	return false;
}

/**
 * (Re-)Load the keys from keys.conf.
 */
void KeyStorePrivate::reset(void)
{
	if (keys.isEmpty())
		return;

	// Get the KeyManager.
	KeyManager *const keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager)
		return;

	// Have any keys actually changed?
	bool hasChanged = false;

	// TODO: Move conversion to KeyManager?
	char buf[64+1];

	int keyIdxStart = 0;
	KeyManager::KeyData_t keyData;
	for (int encSysNum = 0; encSysNum < ARRAY_SIZE(encKeyFns); encSysNum++) {
		const KeyStorePrivate::EncKeyFns_t *const encSys = &encKeyFns[encSysNum];
		const int keyCount = encSys->pfnKeyCount();
		assert(keyCount > 0);
		if (keyCount <= 0)
			continue;

		// Starting key.
		KeyStore::Key *key = &keys[keyIdxStart];
		// Increment keyIdxStart for the next section.
		keyIdxStart += keyCount;

		// Get the keys.
		for (int i = 0; i < keyCount; i++, key++) {
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
						const uint8_t *pKey = keyData.key;
						char *pBuf = buf;
						for (unsigned int i = keyData.length; i > 0; i--, pKey++, pBuf += 2) {
							pBuf[0] = hex_lookup[*pKey >> 4];
							pBuf[1] = hex_lookup[*pKey & 0x0F];
						}
						*pBuf = 0;
						if (key->value != QLatin1String(buf)) {
							key->value = QLatin1String(buf);
							hasChanged = true;
						}

						// Verify the key.
						verifyKey(encSysNum, i);
					} else {
						// Key is invalid...
						// TODO: Show an error message?
						if (!key->value.isEmpty()) {
							key->value.clear();
							hasChanged = true;
						}
						key->status = KeyStore::Key::Status_NotAKey;
					}
					break;

				case KeyManager::VERIFY_KEY_INVALID:
					// Key is invalid. (i.e. not in the correct format)
					if (!key->value.isEmpty()) {
						key->value.clear();
						hasChanged = true;
					}
					key->status = KeyStore::Key::Status_NotAKey;
					break;

				default:
					// Assume the key wasn't found.
					if (!key->value.isEmpty()) {
						key->value.clear();
						hasChanged = true;
					}
					key->status = KeyStore::Key::Status_Empty;
					break;
			}
		}
	}

	if (hasChanged) {
		// Keys have changed.
		Q_Q(KeyStore);
		emit q->allKeysChanged();
	}

	// Keys have been reset.
	changed = false;
}

/**
 * Convert a string that may contain kanji to hexadecimal.
 * @param str String.
 * @return Converted string, or empty string on error.
 */
QString KeyStorePrivate::convertKanjiToHex(const QString &str)
{
	// Check for non-ASCII characters.
	// TODO: Also check for non-hex digits?
	bool hasNonAscii = false;
	foreach (QChar chr, str) {
		if (chr.unicode() >= 128) {
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

	// Convert to a UTF-16LE hex string, starting with U+FEFF.
	// TODO: Combine with the first loop?
	QString hexstr;
	hexstr.reserve(4+(hexstr.size()*4));
	hexstr += QLatin1String("FFFE");
	foreach (const QChar chr, str) {
		const ushort u16 = chr.unicode();
		hexstr += QChar((ushort)hex_lookup[(u16 >>  4) & 0x0F]);
		hexstr += QChar((ushort)hex_lookup[(u16 >>  0) & 0x0F]);
		hexstr += QChar((ushort)hex_lookup[(u16 >> 12) & 0x0F]);
		hexstr += QChar((ushort)hex_lookup[(u16 >>  8) & 0x0F]);
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
int KeyStorePrivate::verifyKeyData(const uint8_t *keyData, const uint8_t *verifyData, int len)
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
	unsigned int size = cipher->decrypt(testData, sizeof(testData));
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
void KeyStorePrivate::verifyKey(int sectIdx, int keyIdx)
{
	assert(sectIdx >= 0);
	assert(sectIdx < sections.size());
	if (sectIdx < 0 || sectIdx >= sections.size())
		return;

	assert(keyIdx >= 0);
	assert(keyIdx < sections[sectIdx].keyCount);
	if (keyIdx < 0 || keyIdx >= sections[sectIdx].keyCount)
		return;

	// Check the key length.
	KeyStore::Key &key = keys[sections[sectIdx].keyIdxStart + keyIdx];
	if (key.value.isEmpty()) {
		// Empty key.
		key.status = KeyStore::Key::Status_Empty;
		return;
	} else if (key.value.size() != 16 && key.value.size() % 2 != 0) {
		// Invalid length.
		// TODO: Support keys that aren't 128-bit.
		key.status = KeyStore::Key::Status_NotAKey;
		return;
	}

	if (!cipher) {
		// Cipher is unavailable.
		// Cannot verify the key.
		key.status = KeyStore::Key::Status_Unknown;
		return;
	}

	// Get the key verification data. (16 bytes)
	const uint8_t *const verifyData = encKeyFns[sectIdx].pfnVerifyData(keyIdx);
	if (!verifyData) {
		// No key verification data is available.
		key.status = KeyStore::Key::Status_Unknown;
		return;
	}

	// Convert the key to bytes.
	// TODO: Support keys that aren't 128-bit.
	uint8_t keyBytes[16];
	int ret = KeyManager::hexStringToBytes(Q2RP(key.value), keyBytes, sizeof(keyBytes));
	if (ret != 0) {
		// Invalid character(s) encountered.
		key.status = KeyStore::Key::Status_NotAKey;
		return;
	}

	// Verify the key.
	ret = verifyKeyData(keyBytes, verifyData, sizeof(keyBytes));
	if (ret == 0) {
		// Decrypted data is correct.
		key.status = KeyStore::Key::Status_OK;
	} else {
		// Decrypted data is wrong.
		key.status = KeyStore::Key::Status_Incorrect;
	}
}

/** KeyStore **/

/**
 * Create a new KeyStore object.
 * @param parent Parent object.
 */
KeyStore::KeyStore(QObject *parent)
	: super(parent)
	, d_ptr(new KeyStorePrivate(this))
{ }

KeyStore::~KeyStore()
{
	delete d_ptr;
}

/**
 * (Re-)Load the keys from keys.conf.
 */
void KeyStore::reset(void)
{
	Q_D(KeyStore);
	d->reset();
}

/** Accessors. **/

/**
 * Get the number of sections. (top-level)
 * @return Number of sections.
 */
int KeyStore::sectCount(void) const
{
	Q_D(const KeyStore);
	return d->sections.size();
}

/**
 * Get a section name.
 * @param sectIdx Section index.
 * @return Section name, or empty string on error.
 */
QString KeyStore::sectName(int sectIdx) const
{
	Q_D(const KeyStore);
	assert(sectIdx >= 0);
	assert(sectIdx < d->sections.size());
	if (sectIdx < 0 || sectIdx >= d->sections.size())
		return QString();
	return d->sections[sectIdx].name;
}

/**
 * Get the number of keys in a given section.
 * @param sectIdx Section index.
 * @return Number of keys in the section, or -1 on error.
 */
int KeyStore::keyCount(int sectIdx) const
{
	Q_D(const KeyStore);
	assert(sectIdx >= 0);
	assert(sectIdx < d->sections.size());
	if (sectIdx < 0 || sectIdx >= d->sections.size())
		return -1;
	return d->sections[sectIdx].keyCount;
}

/**
 * Get the total number of keys.
 * @return Total number of keys.
 */
int KeyStore::totalKeyCount(void) const
{
	Q_D(const KeyStore);
	int ret = 0;
	foreach (const KeyStorePrivate::Section &section, d->sections) {
		ret += section.keyCount;
	}
	return ret;
}

/**
 * Is the KeyStore empty?
 * @return True if empty; false if not.
 */
bool KeyStore::isEmpty(void) const
{
	Q_D(const KeyStore);
	return d->sections.isEmpty();
}

/**
 * Get a Key object.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 * @return Key object, or nullptr on error.
 */
const KeyStore::Key *KeyStore::getKey(int sectIdx, int keyIdx) const
{
	Q_D(const KeyStore);
	assert(sectIdx >= 0);
	assert(sectIdx < d->sections.size());
	if (sectIdx < 0 || sectIdx >= d->sections.size())
		return nullptr;
	assert(keyIdx >= 0);
	assert(keyIdx < d->sections[sectIdx].keyCount);
	if (keyIdx < 0 || keyIdx >= d->sections[sectIdx].keyCount)
		return nullptr;

	return &d->keys[d->sections[sectIdx].keyIdxStart + keyIdx];
}

/**
 * Get a Key object using a linear key count.
 * TODO: Remove this once we switch to a Tree model.
 * @param idx Key index.
 * @return Key object, or nullptr on error.
 */
const KeyStore::Key *KeyStore::getKey(int idx) const
{
	Q_D(const KeyStore);
	assert(idx >= 0);
	assert(idx < d->keys.size());
	if (idx < 0 || idx >= d->keys.size())
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
int KeyStore::setKey(int sectIdx, int keyIdx, const QString &value)
{
	Q_D(KeyStore);
	assert(sectIdx >= 0);
	assert(sectIdx < d->sections.size());
	if (sectIdx < 0 || sectIdx >= d->sections.size())
		return -ERANGE;
	assert(keyIdx >= 0);
	assert(keyIdx < d->sections[sectIdx].keyCount);
	if (keyIdx < 0 || keyIdx >= d->sections[sectIdx].keyCount)
		return -ERANGE;

	// If allowKanji is true, check if the key is kanji
	// and convert it to UTF-16LE hexadecimal.
	const KeyStorePrivate::Section &section = d->sections[sectIdx];
	Key &key = d->keys[section.keyIdxStart + keyIdx];
	QString new_value;
	if (key.allowKanji) {
		// Convert kanji to hexadecimal if needed.
		// NOTE: convertKanjiToHex() returns an empty string on error,
		// so if the original string is empty, don't do anything.
		if (!value.isEmpty()) {
			QString convKey = KeyStorePrivate::convertKanjiToHex(value);
			if (convKey.isEmpty()) {
				// Invalid kanji key.
				return -EINVAL;
			}
			new_value = convKey;
		}
	} else {
		// Hexadecimal only.
		// TODO: Validate it here? We're already
		// using a validator in the UI...
		new_value = value.toUpper();
	}

	if (key.value != new_value) {
		key.value = new_value;
		// Verify the key.
		d->verifyKey(sectIdx, keyIdx);
		// Key has changed.
		emit keyChanged(sectIdx, keyIdx);
		emit keyChanged(section.keyIdxStart + keyIdx);
		d->changed = true;
		emit modified();
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
int KeyStore::setKey(int idx, const QString &value)
{
	Q_D(KeyStore);
	assert(idx >= 0);
	assert(idx < d->keys.size());
	if (idx < 0 || idx >= d->keys.size())
		return -ERANGE;

	Key &key = d->keys[idx];
	QString new_value;
	if (key.allowKanji) {
		// Convert kanji to hexadecimal if needed.
		// NOTE: convertKanjiToHex() returns an empty string on error,
		// so if the original string is empty, don't do anything.
		if (!value.isEmpty()) {
			QString convKey = KeyStorePrivate::convertKanjiToHex(value);
			if (convKey.isEmpty()) {
				// Invalid kanji key.
				return -EINVAL;
			}
			new_value = convKey;
		}
	} else {
		// Hexadecimal only.
		// TODO: Validate it here? We're already
		// using a validator in the UI...
		new_value = value.toUpper();
	}

	if (key.value != new_value) {
		key.value = new_value;
		int sectIdx, keyIdx;
		bool bRet = d->flatKeyToSectKey(idx, sectIdx, keyIdx);
		assert(bRet);
		if (bRet) {
			// Verify the key.
			d->verifyKey(sectIdx, keyIdx);
			emit keyChanged(sectIdx, keyIdx);
		}
		emit keyChanged(idx);
		d->changed = true;
		emit modified();
	}
	return 0;
}

/**
 * Has KeyStore been changed by the user?
 * @return True if it has; false if it hasn't.
 */
bool KeyStore::changed(void) const
{
	Q_D(const KeyStore);
	return d->changed;
}

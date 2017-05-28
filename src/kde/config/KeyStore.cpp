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
using LibRpBase::KeyManager;

// libromdata
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/crypto/N3DSVerifyKeys.hpp"
using namespace LibRomData;

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cassert>

// Qt includes.
#include <QtCore/QVector>

class KeyStorePrivate
{
	public:
		explicit KeyStorePrivate(KeyStore *q);

	private:
		KeyStore *const q_ptr;
		Q_DECLARE_PUBLIC(KeyStore)
		Q_DISABLE_COPY(KeyStorePrivate)

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

	public:
		/**
		 * (Re-)Load the keys from keys.conf.
		 */
		void reset(void);

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
};

/** KeyStorePrivate **/

const KeyStorePrivate::EncKeyFns_t KeyStorePrivate::encKeyFns[] = {
	ENCKEYFNS(WiiPartition,    _RP("Nintendo Wii AES Keys")),
	ENCKEYFNS(CtrKeyScrambler, _RP("Nintendo 3DS Key Scrambler Constants")),
	ENCKEYFNS(N3DSVerifyKeys,  _RP("Nintendo 3DS AES Keys")),
};

KeyStorePrivate::KeyStorePrivate(KeyStore *q)
	: q_ptr(q)
{
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
			if (key.name == QLatin1String("twl-scrambler")) {
				key.allowKanji = true;
			}

			keys.append(key);
		}
	}

	// Load the keys.
	reset();
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

	// Hexadecimal lookup table.
	static const char hex_lookup[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','A','B','C','D','E','F',
	};

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

						// TODO: Verify the key.
						key->status = KeyStore::Key::Status_Unknown;
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

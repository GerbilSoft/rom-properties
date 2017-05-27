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

		// Set up the section.
		section.name = RP2Q(encSys->sectName);
		section.keyIdxStart = keyIdxStart;
		section.keyCount = keyCount;
		sections.append(section);

		// Get the keys.
		keys.reserve(keys.size() + keyCount);
		for (int i = 0; i < keyCount; i++) {
			// Key name.
			const char *const keyName = encSys->pfnKeyName(i);
			assert(keyName != nullptr);
			if (keyName) {
				key.name = QLatin1String(keyName);
			} else {
				key.name.clear();
			}

			// Key is empty initially.
			key.status = KeyStore::Key::Status_Empty;

			// Allow kanji for twl-scrambler.
			if (key.name == QLatin1String("twl-scrambler")) {
				key.allowKanji = true;
			}

			keys.append(key);
		}
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

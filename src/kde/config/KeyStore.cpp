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

		// Map of QListView indexes to encKeyFns[] / KeyIndex value.
		// - Index: QListView index.
		// - Value:
		//   - LOWORD: KeyIndex for the specific system.
		//   - HIWORD: encKeyFns[] index.
		QVector<uint32_t> lvKeyMapping;

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
		};

		#define ENCKEYFNS(klass) { \
			klass::encryptionKeyCount_static, \
			klass::encryptionKeyName_static, \
			klass::encryptionVerifyData_static, \
		}

		static const EncKeyFns_t encKeyFns[];
};

// Based on the Win32 macros.
#ifndef LOWORD
static inline uint16_t LOWORD(uint32_t dw) {
	return (dw & 0xFFFF);
}
#endif
#ifndef HIWORD
static inline uint16_t HIWORD(uint32_t dw) {
	return (dw >> 16);
}
#endif
#ifndef MAKELONG
static inline uint32_t MAKELONG(uint16_t low, uint16_t high) {
	return (low | (high << 16));
}
#endif

/** KeyStorePrivate **/

const KeyStorePrivate::EncKeyFns_t KeyStorePrivate::encKeyFns[] = {
	ENCKEYFNS(WiiPartition),
	ENCKEYFNS(CtrKeyScrambler),
	ENCKEYFNS(N3DSVerifyKeys),
};

KeyStorePrivate::KeyStorePrivate(KeyStore *q)
	: q_ptr(q)
{
	// Load the key names from the various classes.
	// Values will be loaded later.
	// TODO: Display a "separator" between systems?
	// (May need to use a tree layout...)
	keys.clear();
	lvKeyMapping.clear();
	KeyStore::Key key;
	for (int encSysNum = 0; encSysNum < ARRAY_SIZE(encKeyFns); encSysNum++) {
		const EncKeyFns_t *const encSys = &encKeyFns[encSysNum];
		int keyCount = encSys->pfnKeyCount();
		keys.reserve(keys.size() + keyCount);
		lvKeyMapping.reserve(lvKeyMapping.size() + keyCount);
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
			lvKeyMapping.append(MAKELONG((uint16_t)i, (uint16_t)encSysNum));
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
 * Get the number of keys.
 * @return Number of keys.
 */
int KeyStore::count(void) const
{
	Q_D(const KeyStore);
	return d->keys.size();
}

/**
 * Is the KeyStore empty?
 * @return True if empty; false if not.
 */
bool KeyStore::isEmpty(void) const
{
	Q_D(const KeyStore);
	return d->keys.isEmpty();
}

/**
 * Get a Key object.
 * @param idx Key index.
 * @return Key object, or nullptr on error.
 */
const KeyStore::Key *KeyStore::getKey(int idx) const
{
	Q_D(const KeyStore);
	if (idx < 0 || idx >= d->keys.size())
		return nullptr;
	return &d->keys[idx];
}

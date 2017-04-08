/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * verifykeys.hpp: Verify encryption keys.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
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

#include "config.rpcli.h"

#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif

#include "verifykeys.hpp"

// libromdata
#include "libromdata/crypto/KeyManager.hpp"
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/disc/NCCHReader.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <iostream>
using std::cerr;
using std::endl;

typedef int (*pfnKeyCount_t)(void);
typedef const char* (*pfnKeyName_t)(int keyIdx);
typedef const uint8_t* (*pfnVerifyData_t)(int keyIdx);

typedef struct _EncKeyFns_t {
	const char *name;
	pfnKeyCount_t pfnKeyCount;
	pfnKeyName_t pfnKeyName;
	pfnVerifyData_t pfnVerifyData;
} EncKeyFns_t;

#define ENCKEYFNS(klass) { \
	#klass, \
	klass::encryptionKeyCount_static, \
	klass::encryptionKeyName_static, \
	klass::encryptionVerifyData_static, \
}

static const EncKeyFns_t encKeyFns[] = {
	ENCKEYFNS(WiiPartition),
	ENCKEYFNS(CtrKeyScrambler),
	ENCKEYFNS(NCCHReader),

	{nullptr, nullptr, nullptr, nullptr}
};

/**
 * Verify encryption keys.
 * @return 0 on success; non-zero on error.
 */
int VerifyKeys(void)
{
	// TODO: Add decryption key functions to Nintendo3DS
	// and stop using NCCHReader.

	// Initialize the key manager.
	// Get the Key Manager instance.
	KeyManager *keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager) {
		cerr << "*** ERROR initializing KeyManager. Cannot verify keys." << endl;
		return 1;
	}

	// Get keys from supported classes.
	int ret = 0;
	bool printedOne = false;
	for (const EncKeyFns_t *p = encKeyFns; p->name != nullptr; p++) {
		if (printedOne) {
			cerr << endl;
		}
		printedOne = true;

		cerr << "*** Checking encryption keys from " << p->name << "..." << endl;
		int keyCount = p->pfnKeyCount();
		for (int i = 0; i < keyCount; i++) {
			const char *const keyName = p->pfnKeyName(i);
			assert(keyName != nullptr);
			if (!keyName) {
				cerr << "WARNING: Key " << i << " has no name. Skipping..." << endl;
				ret = 1;
				continue;
			}

			// Verification data. (16 bytes)
			const uint8_t *const verifyData = p->pfnVerifyData(i);
			assert(verifyData != nullptr);
			if (!verifyData) {
				cerr << "WARNING: Key '" << keyName << "' has no verification data. Skipping..." << endl;
				ret = 1;
				continue;
			}

			// Verify the key.
			KeyManager::VerifyResult res = keyManager->getAndVerify(keyName, nullptr, verifyData, 16);
			cerr << keyName << ": ";
			if (res == KeyManager::VERIFY_OK) {
				cerr << "OK" << endl;
			} else {
				cerr << "ERROR: ";
				switch (res) {
					case KeyManager::VERIFY_INVALID_PARAMS:
						cerr << "Invalid parameters.";
						break;
					case KeyManager::VERIFY_KEY_DB_NOT_LOADED:
						cerr << "Key database is not loaded.";
						break;
					case KeyManager::VERIFY_KEY_DB_ERROR:
						cerr << "Key database error.";
						break;
					case KeyManager::VERIFY_KEY_NOT_FOUND:
						cerr << "Key was not found.";
						break;
					case KeyManager::VERIFY_KEY_INVALID:
						cerr << "Key is not valid for this operation.";
						break;
					case KeyManager::VERFIY_IAESCIPHER_INIT_ERR:
						cerr << "AES initialization failed";
						break;
					case KeyManager::VERIFY_IAESCIPHER_DECRYPT_ERR:
						cerr << "AES decryption failed.";
						break;
					case KeyManager::VERIFY_WRONG_KEY:
						cerr << "Key is incorrect.";
						break;
					default:
						cerr << "Unknown error code " << res << ".";
						break;
				}
				cerr << endl;
				ret = 1;
			}
		}
	}

	return ret;
}

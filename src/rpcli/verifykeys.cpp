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

#include "stdafx.h"
#include "config.rpcli.h"

#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif

#include "verifykeys.hpp"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"
#include "librpbase/TextFuncs.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// libromdata
#include "libromdata/disc/WiiPartition.hpp"
#include "libromdata/crypto/CtrKeyScrambler.hpp"
#include "libromdata/crypto/N3DSVerifyKeys.hpp"
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
	ENCKEYFNS(N3DSVerifyKeys),

	{nullptr, nullptr, nullptr, nullptr}
};

/**
 * Verify encryption keys.
 * @return 0 on success; non-zero on error.
 */
int VerifyKeys(void)
{
	// Initialize the key manager.
	// Get the Key Manager instance.
	KeyManager *keyManager = KeyManager::instance();
	assert(keyManager != nullptr);
	if (!keyManager) {
		cerr << "*** " << C_("rpcli", "ERROR initializing KeyManager. Cannot verify keys.") << endl;
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

		cerr << "*** " << rp_sprintf(C_("rpcli", "Checking encryption keys from '%s'..."), p->name) << endl;
		int keyCount = p->pfnKeyCount();
		for (int i = 0; i < keyCount; i++) {
			const char *const keyName = p->pfnKeyName(i);
			assert(keyName != nullptr);
			if (!keyName) {
				cerr << rp_sprintf(C_("rpcli", "WARNING: Key %d has no name. Skipping..."), i) << endl;
				ret = 1;
				continue;
			}

			// Verification data. (16 bytes)
			const uint8_t *const verifyData = p->pfnVerifyData(i);
			assert(verifyData != nullptr);
			if (!verifyData) {
				cerr << rp_sprintf(C_("rpcli", "WARNING: Key '%s' has no verification data. Skipping..."), keyName) << endl;
				ret = 1;
				continue;
			}

			// Verify the key.
			KeyManager::VerifyResult res = keyManager->getAndVerify(keyName, nullptr, verifyData, 16);
			cerr << keyName << ": ";
			if (res == KeyManager::VERIFY_OK) {
				cerr << C_("rpcli", "OK") << endl;
			} else {
				cerr << rp_sprintf(C_("rpcli", "ERROR: %s"),
					KeyManager::verifyResultToString(res)) << endl;
				ret = 1;
			}
		}
	}

	return ret;
}

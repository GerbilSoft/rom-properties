/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * verifykeys.hpp: Verify encryption keys.                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rpcli.h"

#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif

#include "verifykeys.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"

// libromdata
#include "libromdata/crypto/KeyStoreUI.hpp"
using namespace LibRomData;

// C includes (C++ namespace)
#include <cassert>

/**
 * Simple implementation of KeyStoreUI with no signal handling.
 */
class KeyStoreCLI final : public KeyStoreUI
{
protected: /*signals:*/
	void keyChanged_int(int, int) final {}
	void keyChanged_int(int) final {}
	void allKeysChanged_int(void) final {}
	void modified_int(void) final {}
};

/**
 * Verify encryption keys.
 * @return 0 on success; non-zero on error.
 */
int VerifyKeys(void)
{
	// Instantiate the key store.
	// TODO: Check if load failed?
	KeyStoreUI *const keyStore = new KeyStoreCLI();
	keyStore->reset();

	// Get keys from supported classes.
	int ret = 0;
	bool printedOne = false;
	const int sectCount = keyStore->sectCount();
	for (int sectIdx = 0; sectIdx < sectCount; sectIdx++) {
		if (printedOne) {
			putchar('\n');
		}
		printedOne = true;

		fputs("*** ", stdout);
		fmt::print(C_("rpcli", "Checking encryption keys: {:s}"), keyStore->sectName(sectIdx));
		putchar('\n');

		const int keyCount = keyStore->keyCount(sectIdx);
		for (int keyIdx = 0; keyIdx < keyCount; keyIdx++) {
			const KeyStoreUI::Key *const key = keyStore->getKey(sectIdx, keyIdx);
			assert(key != nullptr);
			if (!key) {
				fmt::print(C_("rpcli", "WARNING: Key [{:d},{:d}] has no Key object. Skipping..."), sectIdx, keyIdx);
				putchar('\n');
				ret = 1;
				continue;
			}
			assert(!key->name.empty());
			if (key->name.empty()) {
				fmt::print(C_("rpcli", "WARNING: Key [{:d},{:d}] has no name. Skipping..."), sectIdx, keyIdx);
				putchar('\n');
				ret = 1;
				continue;
			}

			// Verification status.
			// TODO: Make this a function in KeyStoreUI?
			// NOTE: Not a table because only 'OK' is valid; others are errors.
			bool isOK = false;
			const char *s_err = nullptr;
			switch (key->status) {
				case KeyStoreUI::Key::Status::Empty:
					s_err = C_("rpcli|KeyVerifyStatus", "Empty key");
					break;
				case KeyStoreUI::Key::Status::Unknown:
				default:
					s_err = C_("rpcli|KeyVerifyStatus", "Unknown status");
					break;
				case KeyStoreUI::Key::Status::NotAKey:
					s_err = C_("rpcli|KeyVerifyStatus", "Not a key");
					break;
				case KeyStoreUI::Key::Status::Incorrect:
					s_err = C_("rpcli|KeyVerifyStatus", "Key is incorrect");
					break;
				case KeyStoreUI::Key::Status::OK:
					isOK = true;
					s_err = C_("rpcli|KeyVerifyStatus", "OK");
					break;
			}

			fmt::print(FSTR("{:s}: "), key->name.c_str());
			if (isOK) {
				fmt::print(FSTR("{:s}\n"), s_err);
			} else {
				fmt::print(C_("rpcli", "ERROR: {:s}"), s_err);
				putchar('\n');
				ret = 1;
			}
		}
	}

	delete keyStore;
	fflush(stdout);
	return ret;
}

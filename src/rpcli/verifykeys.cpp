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

// libgsvt for VT handling
#include "gsvt.h"

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
	KeyStoreCLI keyStore;
	keyStore.reset();

	// Get keys from supported classes.
	int ret = 0;
	bool printedOne = false;
	const int sectCount = keyStore.sectCount();
	for (int sectIdx = 0; sectIdx < sectCount; sectIdx++) {
		if (printedOne) {
			gsvt_newline(gsvt_stdout);
		}
		printedOne = true;

		gsvt_text_color_set8(gsvt_stdout, 6, true);	// cyan
		gsvt_fputs("*** ", gsvt_stdout);
		gsvt_fputs(fmt::format(FRUN(C_("rpcli", "Checking encryption keys: {:s}")), keyStore.sectName(sectIdx)), gsvt_stdout);
		gsvt_text_color_reset(gsvt_stdout);
		gsvt_newline(gsvt_stdout);
		fflush(stdout);

		const int keyCount = keyStore.keyCount(sectIdx);
		for (int keyIdx = 0; keyIdx < keyCount; keyIdx++) {
			const KeyStoreUI::Key *const key = keyStore.getKey(sectIdx, keyIdx);
			assert(key != nullptr);
			if (!key) {
				gsvt_text_color_set8(gsvt_stdout, 3, true);	// yellow
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "WARNING: Key [{:d},{:d}] has no Key object. Skipping...")), sectIdx, keyIdx), gsvt_stdout);
				gsvt_text_color_reset(gsvt_stdout);
				gsvt_newline(gsvt_stdout);
				fflush(stdout);
				ret = 1;
				continue;
			}
			assert(!key->name.empty());
			if (key->name.empty()) {
				gsvt_text_color_set8(gsvt_stdout, 3, true);	// yellow
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "WARNING: Key [{:d},{:d}] has no name. Skipping...")), sectIdx, keyIdx), gsvt_stdout);
				gsvt_text_color_reset(gsvt_stdout);
				gsvt_newline(gsvt_stdout);
				fflush(stdout);
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

			gsvt_fputs(fmt::format(FSTR("{:s}: "), key->name), gsvt_stdout);
			if (isOK) {
				gsvt_text_color_set8(gsvt_stdout, 2, true);	// green
				gsvt_fputs(s_err, gsvt_stdout);
			} else {
				gsvt_text_color_set8(gsvt_stdout, 1, true);	// red
				gsvt_fputs(fmt::format(FRUN(C_("rpcli", "ERROR: {:s}")), s_err), gsvt_stdout);
				ret = 1;
			}
			gsvt_text_color_reset(gsvt_stdout);
			gsvt_newline(gsvt_stdout);
			fflush(stdout);
		}
	}

	fflush(stdout);
	return ret;
}

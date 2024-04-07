/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CtrKeyScrambler.hpp: Nintendo 3DS key scrambler.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#ifndef ENABLE_DECRYPTION
#  error This file should only be included if decryption is enabled.
#endif /* ENABLE_DECRYPTION */

#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// for u128_t
#include "N3DSVerifyKeys.hpp"

namespace LibRomData { namespace CtrKeyScrambler {

/**
 * CTR key scrambler. (for keyslots 0x04-0x3F)
 * @param keyNormal	[out] Normal key.
 * @param keyX[		in] KeyX.
 * @param keyY		[in] KeyY.
 * @param ctr_scrambler	[in] Scrambler constant.
 * @return 0 on success; negative POSIX error code on error.
 */
RP_LIBROMDATA_PUBLIC
int CtrScramble(u128_t *keyNormal,
	const u128_t *keyX, const u128_t *keyY,
	const u128_t *ctr_scrambler);

/**
 * CTR key scrambler. (for keyslots 0x04-0x3F)
 *
 * "ctr-scrambler" is retrieved from KeyManager and is
 * used as the scrambler constant.
 *
 * @param keyNormal	[out] Normal key.
 * @param keyX		[in] KeyX.
 * @param keyY		[in] KeyY.
 * @return 0 on success; negative POSIX error code on error.
 */
RP_LIBROMDATA_PUBLIC
int CtrScramble(u128_t *keyNormal, const u128_t *keyX, const u128_t *keyY);

// Encryption key indexes.
enum class EncryptionKeys {
	// Retail
	Key_Twl_Scrambler,
	Key_Ctr_Scrambler,

	Key_Max
};

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int encryptionKeyCount_static(void);

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *encryptionKeyName_static(int keyIdx);

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *encryptionVerifyData_static(int keyIdx);

} }

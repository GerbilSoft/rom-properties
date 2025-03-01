/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CtrKeyScrambler.cpp: Nintendo 3DS key scrambler.                        *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#ifndef ENABLE_DECRYPTION
#error This file should only be compiled if decryption is enabled.
#endif /* ENABLE_DECRYPTION */

#include "CtrKeyScrambler.hpp"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"
using LibRpBase::KeyManager;

// C++ STL classes
using std::array;

namespace LibRomData { namespace CtrKeyScrambler {

namespace Private {

// Verification key names.
static const array<const char*, static_cast<size_t>(EncryptionKeys::Key_Max)> EncryptionKeyNames = {{
	"twl-scrambler",
	"ctr-scrambler",
}};

static constexpr uint8_t EncryptionKeyVerifyData[static_cast<size_t>(EncryptionKeys::Key_Max)][16] = {
	// twl-scrambler
	{0x65,0xCF,0x82,0xC5,0xDB,0x79,0x93,0x8C,
	 0x01,0x33,0x65,0x87,0x72,0xDF,0x60,0x94},
	// ctr-scrambler
	{0xEF,0x4F,0x47,0x3C,0x04,0xAD,0xAA,0xAE,
	 0x66,0x98,0x29,0xCB,0xC2,0x4D,0x9D,0xB0},
};

}

/**
 * Get the total number of encryption key names.
 * @return Number of encryption key names.
 */
int encryptionKeyCount_static(void)
{
	return (int)EncryptionKeys::Key_Max;
}

/**
 * Get an encryption key name.
 * @param keyIdx Encryption key index.
 * @return Encryption key name (in ASCII), or nullptr on error.
 */
const char *encryptionKeyName_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < static_cast<int>(EncryptionKeys::Key_Max));
	if (keyIdx < 0 || keyIdx >= static_cast<int>(EncryptionKeys::Key_Max))
		return nullptr;
	return Private::EncryptionKeyNames[keyIdx];
}

/**
 * Get the verification data for a given encryption key index.
 * @param keyIdx Encryption key index.
 * @return Verification data. (16 bytes)
 */
const uint8_t *encryptionVerifyData_static(int keyIdx)
{
	assert(keyIdx >= 0);
	assert(keyIdx < static_cast<int>(EncryptionKeys::Key_Max));
	if (keyIdx < 0 || keyIdx >= static_cast<int>(EncryptionKeys::Key_Max))
		return nullptr;
	return Private::EncryptionKeyVerifyData[keyIdx];
}

/**
 * Byteswap a 128-bit key for use with 32/64-bit addition.
 * NOTE: Byteswaps in-place.
 * @param key Key
 */
static inline void bswap_u128_t(u128_t &key)
{
	key.u64[0] = __swab64(key.u64[0]);
	key.u64[1] = __swab64(key.u64[1]);
}

/**
 * CTR key scrambler (for keyslots 0x04-0x3F)
 * @param keyNormal	[out] Normal key
 * @param keyX		[in] KeyX
 * @param keyY		[in] KeyY
 * @param ctr_scrambler	[in] Scrambler constant
 * @return 0 on success; negative POSIX error code on error.
 */
int CtrScramble(u128_t &keyNormal,
	u128_t keyX, u128_t keyY, u128_t ctr_scrambler)
{
	// CTR key scrambler: KeyNormal = (((KeyX <<< 2) ^ KeyY) + constant) <<< 87
	// NOTE: Since C doesn't have 128-bit types, we'll operate on
	// 64-bit types. This requires some byteswapping, since the
	// key is handled as if it's big-endian.

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	bswap_u128_t(keyX);
	bswap_u128_t(ctr_scrambler);
#endif

	// Rotate KeyX left by two.
	u128_t keyTmp;
	keyTmp.u64[0] = (keyX.u64[0] << 2) | (keyX.u64[1] >> 62);
	keyTmp.u64[1] = (keyX.u64[1] << 2) | (keyX.u64[0] >> 62);

	// XOR by KeyY.
	keyTmp.u64[0] ^= be64_to_cpu(keyY.u64[0]);
	keyTmp.u64[1] ^= be64_to_cpu(keyY.u64[1]);

	// Add the constant.
	// Reference for carry functionality: https://accu.org/index.php/articles/1849
	keyTmp.u64[1] += ctr_scrambler.u64[1];
	keyTmp.u64[0] += ctr_scrambler.u64[0] + (keyTmp.u64[1] < ctr_scrambler.u64[1]);

	// Rotate left by 87.
	// This is effectively "rotate left by 23" with adjusted DWORD indexes.
	keyNormal.u64[1] = cpu_to_be64((keyTmp.u64[0] << 23) | (keyTmp.u64[1] >> 41));
	keyNormal.u64[0] = cpu_to_be64((keyTmp.u64[1] << 23) | (keyTmp.u64[0] >> 41));

	// We're done here.
	return 0;
}

/**
 * CTR key scrambler (for keyslots 0x04-0x3F)
 *
 * "ctr-scrambler" is retrieved from KeyManager and is
 * used as the scrambler constant.
 *
 * @param keyNormal	[out] Normal key
 * @param keyX		[in] KeyX
 * @param keyY		[in] KeyY
 * @return 0 on success; negative POSIX error code on error.
 */
int CtrScramble(u128_t &keyNormal, u128_t keyX, u128_t keyY)
{
	// Load the key scrambler constant.
	KeyManager *const keyManager = KeyManager::instance();
	if (!keyManager) {
		// Unable to initialize the KeyManager.
		return -EIO;
	}

	KeyManager::KeyData_t keyData;
	KeyManager::VerifyResult res = keyManager->getAndVerify(
		Private::EncryptionKeyNames[static_cast<size_t>(EncryptionKeys::Key_Ctr_Scrambler)], &keyData,
		Private::EncryptionKeyVerifyData[static_cast<size_t>(EncryptionKeys::Key_Ctr_Scrambler)], 16);
	if (res != KeyManager::VerifyResult::OK) {
		// Key error.
		// TODO: Return the key error?
		return -ENOENT;
	}

	if (!keyData.key || keyData.length != 16) {
		// Key is not valid.
		return -EIO;	// TODO: Better error code?
	}

	return CtrScramble(keyNormal, keyX, keyY,
		*(reinterpret_cast<const u128_t*>(keyData.key)));
}

} }

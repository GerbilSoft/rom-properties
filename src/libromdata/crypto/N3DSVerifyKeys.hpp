/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * N3DSVerifyKeys.hpp: Nintendo 3DS key verification data.                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpbase.h"
#ifndef ENABLE_DECRYPTION
#  error This file should only be compiled if decryption is enabled.
#endif /* !ENABLE_DECRYPTION */

#include "common.h"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

#include "librpbase/crypto/KeyManager.hpp"
#include "../Handheld/n3ds_structs.h"

namespace LibRomData {

// 128-bit type for AES counter and keys.
union u128_t {
	uint8_t u8[16];
	uint32_t u32[4];
	uint64_t u64[2];

	/**
	 * Initialize the AES-CTR counter using a Title ID.
	 * @param tid_be Title ID, in big-endian.
	 * @param section NCCH section.
	 * @param offset Partition offset, in bytes.
	 */
	inline void init_ctr(uint64_t tid_be, uint8_t section, uint32_t offset)
	{
		u64[0] = tid_be;
		u8[8] = section;
		u8[9] = 0;
		u8[10] = 0;
		u8[11] = 0;
		offset /= 16;
		u32[3] = cpu_to_be32(offset);
	}
};

namespace N3DSVerifyKeys {

/**
 * Attempt to load an AES normal key.
 * @param pKeyOut		[out] Output key data.
 * @param keyNormal_name	[in,opt] KeyNormal slot name.
 * @param keyX_name		[in,opt] KeyX slot name.
 * @param keyY_name		[in,opt] KeyY slot name.
 * @param keyNormal_verify	[in,opt] KeyNormal verification data. (NULL or 16 bytes)
 * @param keyX_verify		[in,opt] KeyX verification data. (NULL or 16 bytes)
 * @param keyY_verify		[in,opt] KeyY verification data. (NULL or 16 bytes)
 * @return VerifyResult.
 */
LibRpBase::KeyManager::VerifyResult loadKeyNormal(u128_t *pKeyOut,
	const char *keyNormal_name,
	const char *keyX_name,
	const char *keyY_name,
	const uint8_t *keyNormal_verify,
	const uint8_t *keyX_verify,
	const uint8_t *keyY_verify);

/**
 * Generate an AES normal key from a KeyX and an NCCH signature.
 * KeyX will be selected based on ncchflags[3].
 * The first 16 bytes of the NCCH signature is used as KeyY.
 *
 * NOTE: If the NCCH uses NoCrypto, this function will return
 * an error, since there's no keys that would work for it.
 * Check for NoCrypto before calling this function.
 *
 * TODO: SEED encryption is not supported, though it isn't needed
 * for "exefs:/icon" and "exefs:/banner".
 *
 * @param pKeyOut		[out] Output key data. (array of 2 keys)
 * @param pNcchHeader		[in] NCCH header, with signature.
 * @param issuer		[in] Issuer type. (N3DS_Ticket_TitleKey_KeyY)
 *                                   If unknown, will try Debug, then Retail.
 * @return VerifyResult.
 */
LibRpBase::KeyManager::VerifyResult loadNCCHKeys(u128_t pKeyOut[2],
	const N3DS_NCCH_Header_t *pNcchHeader, uint8_t issuer);

// Encryption key indexes
enum class EncryptionKeys {
	Key_Unknown = -1,

	// Retail
	Key_Retail_SpiBoot = 0,
	Key_Retail_Slot0x18KeyX,
	Key_Retail_Slot0x1BKeyX,
	Key_Retail_Slot0x25KeyX,
	Key_Retail_Slot0x2CKeyX,
	Key_Retail_Slot0x3DKeyX,
	Key_Retail_Slot0x3DKeyY_0,
	Key_Retail_Slot0x3DKeyY_1,
	Key_Retail_Slot0x3DKeyY_2,
	Key_Retail_Slot0x3DKeyY_3,
	Key_Retail_Slot0x3DKeyY_4,
	Key_Retail_Slot0x3DKeyY_5,
	Key_Retail_Slot0x3DKeyNormal_0,
	Key_Retail_Slot0x3DKeyNormal_1,
	Key_Retail_Slot0x3DKeyNormal_2,
	Key_Retail_Slot0x3DKeyNormal_3,
	Key_Retail_Slot0x3DKeyNormal_4,
	Key_Retail_Slot0x3DKeyNormal_5,

	// Debug
	Key_Debug_SpiBoot,
	Key_Debug_FixedCryptoKey,
	Key_Debug_Slot0x18KeyX,
	Key_Debug_Slot0x1BKeyX,
	Key_Debug_Slot0x25KeyX,
	Key_Debug_Slot0x2CKeyX,
	Key_Debug_Slot0x3DKeyX,
	Key_Debug_Slot0x3DKeyY_0,
	Key_Debug_Slot0x3DKeyY_1,
	Key_Debug_Slot0x3DKeyY_2,
	Key_Debug_Slot0x3DKeyY_3,
	Key_Debug_Slot0x3DKeyY_4,
	Key_Debug_Slot0x3DKeyY_5,
	Key_Debug_Slot0x3DKeyNormal_0,
	Key_Debug_Slot0x3DKeyNormal_1,
	Key_Debug_Slot0x3DKeyNormal_2,
	Key_Debug_Slot0x3DKeyNormal_3,
	Key_Debug_Slot0x3DKeyNormal_4,
	Key_Debug_Slot0x3DKeyNormal_5,

	Key_Max
};

/** Public access functions used by rpcli and others. **/

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

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesNettle.cpp: AES decryption class using GNU Nettle.                   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AesNettle.hpp"

// Nettle AES functions.
#include <nettle/nettle-types.h>
#include <nettle/aes.h>
#include <nettle/cbc.h>
#include <nettle/ctr.h>

#ifdef HAVE_NETTLE_VERSION_H
#  include <nettle/version.h>
#endif /* HAVE_NETTLE_VERSION_H */

namespace LibRpBase {

class AesNettlePrivate
{
	public:
		AesNettlePrivate();
		~AesNettlePrivate() = default;

	private:
		RP_DISABLE_COPY(AesNettlePrivate)

	public:
		// AES context.
#ifdef HAVE_NETTLE_3
		union {
			struct aes128_ctx aes128;
			struct aes192_ctx aes192;
			struct aes256_ctx aes256;
		} ctx;
#else /* !HAVE_NETTLE_3 */
		struct aes_ctx ctx;
#endif /* HAVE_NETTLE_3 */

		// Encryption key.
		// Stored here because we need to
		// use decryption for ECB and CBC,
		// but encryption when using CTR.
		uint8_t key[32];
		int key_len;

		// CBC: Initialization vector.
		// CTR: Counter.
		uint8_t iv[AES_BLOCK_SIZE];

		IAesCipher::ChainingMode chainingMode;

		// Has the key been changed since the last operation?
		bool key_changed;

#ifdef HAVE_NETTLE_3
		// Cipher functions.
		nettle_cipher_func *decrypt_fn;
		nettle_cipher_func *encrypt_fn;
		// Set Key functions.
		nettle_set_key_func *setkey_dec_fn;
		nettle_set_key_func *setkey_enc_fn;
#endif /* HAVE_NETTLE_3 */
};

/** AesNettlePrivate **/

AesNettlePrivate::AesNettlePrivate()
	: chainingMode(IAesCipher::ChainingMode::ECB)
	, key_changed(true)
#ifdef HAVE_NETTLE_3
	, decrypt_fn(nullptr)
	, encrypt_fn(nullptr)
	, setkey_dec_fn(nullptr)
	, setkey_enc_fn(nullptr)
#endif /* HAVE_NETTLE_3 */
{
	// Clear the context and keys.
	memset(&ctx, 0, sizeof(ctx));
	memset(key, 0, sizeof(key));
	key_len = 0;
	memset(iv, 0, sizeof(iv));
}

/** AesNettle **/

AesNettle::AesNettle()
	: d_ptr(new AesNettlePrivate())
{ }

AesNettle::~AesNettle()
{
	delete d_ptr;
}

/**
 * Get the name of the AesCipher implementation.
 * @return Name.
 */
const char *AesNettle::name(void) const
{
	// TODO: Use NETTLE_VERSION_MAJOR and NETTLE_VERSION_MINOR if available.
#ifdef HAVE_NETTLE_3
	return "GNU Nettle 3.x";
#else
	return "GNU Nettle 2.x";
#endif
}

/**
 * Has the cipher been initialized properly?
 * @return True if initialized; false if not.
 */
bool AesNettle::isInit(void) const
{
	// Nettle always works.
	return true;
}

/**
 * Set the encryption key.
 * @param pKey	[in] Key data.
 * @param size	[in] Size of pKey, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesNettle::setKey(const uint8_t *RESTRICT pKey, size_t size)
{
	// Acceptable key lengths:
	// - 16 (AES-128)
	// - 24 (AES-192)
	// - 32 (AES-256)
	if (!pKey || !(size == 16 || size == 24 || size == 32)) {
		return -EINVAL;
	}

	RP_D(AesNettle);
#ifdef HAVE_NETTLE_3
	switch (size) {
		case 16:
			d->decrypt_fn = (nettle_cipher_func*)aes128_decrypt;
			d->encrypt_fn = (nettle_cipher_func*)aes128_encrypt;
			d->setkey_dec_fn = (nettle_set_key_func*)aes128_set_decrypt_key;
			d->setkey_enc_fn = (nettle_set_key_func*)aes128_set_encrypt_key;
			break;
		case 24:
			d->decrypt_fn = (nettle_cipher_func*)aes192_decrypt;
			d->encrypt_fn = (nettle_cipher_func*)aes192_encrypt;
			d->setkey_dec_fn = (nettle_set_key_func*)aes192_set_decrypt_key;
			d->setkey_enc_fn = (nettle_set_key_func*)aes192_set_encrypt_key;
			break;
		case 32:
			d->decrypt_fn = (nettle_cipher_func*)aes256_decrypt;
			d->encrypt_fn = (nettle_cipher_func*)aes256_encrypt;
			d->setkey_dec_fn = (nettle_set_key_func*)aes256_set_decrypt_key;
			d->setkey_enc_fn = (nettle_set_key_func*)aes256_set_encrypt_key;
			break;
		default:
			return -EINVAL;
	}
#endif

	// Save the key data.
	memcpy(d->key, pKey, size);
	d->key_len = size;
	d->key_changed = true;
	return 0;
}

/**
 * Set the cipher chaining mode.
 *
 * Note that the IV/counter must be set *after* setting
 * the chaining mode; otherwise, setIV() will fail.
 *
 * @param mode Cipher chaining mode.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesNettle::setChainingMode(ChainingMode mode)
{
	if (mode < ChainingMode::ECB || mode >= ChainingMode::Max) {
		return -EINVAL;
	}

	RP_D(AesNettle);
	if (d->chainingMode != mode) {
		d->chainingMode = mode;
		d->key_changed = true;
	}
	return 0;
}

/**
 * Set the IV (CBC mode) or counter (CTR mode).
 * @param pIV	[in] IV/counter data.
 * @param size	[in] Size of pIV, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesNettle::setIV(const uint8_t *RESTRICT pIV, size_t size)
{
	RP_D(AesNettle);
	if (!pIV || size != AES_BLOCK_SIZE ||
	    d->chainingMode < ChainingMode::CBC || d->chainingMode >= ChainingMode::Max)
	{
		// Invalid parameters and/or chaining mode.
		return -EINVAL;
	}

	// Set the IV/counter.
	// NOTE: This does NOT require a key update.
	memcpy(d->iv, pIV, AES_BLOCK_SIZE);
	return 0;
}

/**
 * Decrypt a block of data.
 * @param pData	[in/out] Data block.
 * @param size	[in] Length of data block. (Must be a multiple of 16.)
 * @return Number of bytes decrypted on success; 0 on error.
 */
size_t AesNettle::decrypt(uint8_t *RESTRICT pData, size_t size)
{
	if (!pData || size == 0 || (size % AES_BLOCK_SIZE != 0)) {
		// Invalid parameters.
		return 0;
	}

	// Decrypt the data.
	RP_D(AesNettle);

#ifdef HAVE_NETTLE_3
	if (!d->decrypt_fn) {
		// No decryption function set...
		return 0;
	}

	switch (d->chainingMode) {
		case ChainingMode::ECB:
			if (d->key_changed) {
				d->setkey_dec_fn(&d->ctx, d->key);
				d->key_changed = false;
			}
			d->decrypt_fn(&d->ctx, size, pData, pData);
			break;

		case ChainingMode::CBC:
			// IV is automatically updated for the next block.
			if (d->key_changed) {
				d->setkey_dec_fn(&d->ctx, d->key);
				d->key_changed = false;
			}
			cbc_decrypt(&d->ctx, d->decrypt_fn, AES_BLOCK_SIZE,
				    d->iv, size, pData, pData);
			break;

		case ChainingMode::CTR:
			// ctr is automatically updated for the next block.
			// NOTE: ctr uses the *encrypt* function, even for decryption.
			if (d->key_changed) {
				d->setkey_enc_fn(&d->ctx, d->key);
				d->key_changed = false;
			}
			ctr_crypt(&d->ctx, d->encrypt_fn, AES_BLOCK_SIZE,
				  d->iv, size, pData, pData);
			break;
		default:
			return 0;
	}
#else /* !HAVE_NETTLE_3 */
	switch (d->chainingMode) {
		case ChainingMode::ECB:
			if (d->key_changed) {
				aes_set_decrypt_key(&d->ctx, d->key_len, d->key);
				d->key_changed = false;
			}
			aes_decrypt(&d->ctx, size, pData, pData);
			break;

		case ChainingMode::CBC:
			// IV is automatically updated for the next block.
			if (d->key_changed) {
				aes_set_decrypt_key(&d->ctx, d->key_len, d->key);
				d->key_changed = false;
			}
			cbc_decrypt(&d->ctx, (nettle_crypt_func*)aes_decrypt, AES_BLOCK_SIZE,
				    d->iv, size, pData, pData);
			break;

		case ChainingMode::CTR:
			// ctr is automatically updated for the next block.
			// NOTE: ctr uses the *encrypt* function, even for decryption.
			if (d->key_changed) {
				aes_set_encrypt_key(&d->ctx, d->key_len, d->key);
				d->key_changed = false;
			}
			ctr_crypt(&d->ctx, (nettle_crypt_func*)aes_encrypt, AES_BLOCK_SIZE,
				  d->iv, size, pData, pData);
			break;

		default:
			return 0;
	}
#endif /* HAVE_NETTLE_3 */

	return size;
}

/**
 * Get the nettle compile-time version.
 * @param pMajor	[out] Pointer to store major version.
 * @param pMinor	[out] Pointer to store minor version.
 * @return 0 on success; non-zero on error.
 */
int AesNettle::get_nettle_compile_time_version(int *pMajor, int *pMinor)
{
#if defined(HAVE_NETTLE_VERSION_H)
	*pMajor = NETTLE_VERSION_MAJOR;
	*pMinor = NETTLE_VERSION_MINOR;
#elif defined(HAVE_NETTLE_3)
	*pMajor = 3;
	*pMinor = 0;
#else
	*pMajor = 2;
	*pMinor = 0;	// NOTE: handle as "2.x"
#endif

	return 0;
}

/**
 * Get the nettle runtime version.
 * @param pMajor	[out] Pointer to store major version.
 * @param pMinor	[out] Pointer to store minor version.
 * @return 0 on success; non-zero on error.
 */
int AesNettle::get_nettle_runtime_version(int *pMajor, int *pMinor)
{
#if defined(HAVE_NETTLE_VERSION_H) && defined(HAVE_NETTLE_VERSION_FUNCTIONS)
	*pMajor = nettle_version_major();
	*pMinor = nettle_version_minor();
	return 0;
#else /* !(HAVE_NETTLE_VERSION_H && HAVE_NETTLE_VERSION_FUNCTIONS) */
	RP_UNUSED(pMajor);
	RP_UNUSED(pMinor);
	return -ENOTSUP;
#endif /* HAVE_NETTLE_VERSION_H && HAVE_NETTLE_VERSION_FUNCTIONS */
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesNettle.cpp: AES decryption class using GNU Nettle.                   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "librpbase/config.librpbase.h"

#include "AesNettle.hpp"
#include "../common.h"

// C includes. (C++ namespace)
#include <cerrno>
#include <cstring>

// Nettle AES functions.
#include <nettle/nettle-types.h>
#include <nettle/aes.h>
#include <nettle/cbc.h>
#include <nettle/ctr.h>

namespace LibRpBase {

class AesNettlePrivate
{
	public:
		AesNettlePrivate();
		~AesNettlePrivate() { }

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
	: chainingMode(IAesCipher::CM_ECB)
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
	if (mode < CM_ECB || mode > CM_CTR) {
		return -EINVAL;
	}

	RP_D(AesNettle);
	d->chainingMode = mode;
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
	    d->chainingMode < CM_CBC || d->chainingMode > CM_CTR)
	{
		// Invalid parameters and/or chaining mode.
		return -EINVAL;
	}

	// Set the IV/counter.
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

	// TODO: Optimize key setting so it isn't
	// done unless it's needed.
	switch (d->chainingMode) {
		case CM_ECB:
			d->setkey_dec_fn(&d->ctx, d->key);
			d->decrypt_fn(&d->ctx, size, pData, pData);
			break;

		case CM_CBC:
			// IV is automatically updated for the next block.
			d->setkey_dec_fn(&d->ctx, d->key);
			cbc_decrypt(&d->ctx, d->decrypt_fn, AES_BLOCK_SIZE,
				    d->iv, size, pData, pData);
			break;

		case CM_CTR:
			// ctr is automatically updated for the next block.
			// NOTE: ctr uses the *encrypt* function, even for decryption.
			d->setkey_enc_fn(&d->ctx, d->key);
			ctr_crypt(&d->ctx, d->encrypt_fn, AES_BLOCK_SIZE,
				  d->iv, size, pData, pData);
			break;
		default:
			return 0;
	}
#else /* !HAVE_NETTLE_3 */
	switch (d->chainingMode) {
		case CM_ECB:
			aes_set_decrypt_key(&d->ctx, d->key_len, d->key);
			aes_decrypt(&d->ctx, size, pData, pData);
			break;

		case CM_CBC:
			// IV is automatically updated for the next block.
			aes_set_decrypt_key(&d->ctx, d->key_len, d->key);
			cbc_decrypt(&d->ctx, (nettle_crypt_func*)aes_decrypt, AES_BLOCK_SIZE,
				    d->iv, size, pData, pData);
			break;

		case CM_CTR:
			// ctr is automatically updated for the next block.
			// NOTE: ctr uses the *encrypt* function, even for decryption.
			aes_set_encrypt_key(&d->ctx, d->key_len, d->key);
			ctr_crypt(&d->ctx, (nettle_crypt_func*)aes_encrypt, AES_BLOCK_SIZE,
				  d->iv, size, pData, pData);
			break;

		default:
			return 0;
	}
#endif /* HAVE_NETTLE_3 */

	return size;
}

}

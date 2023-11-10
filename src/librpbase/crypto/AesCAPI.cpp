/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCAPI.cpp: AES decryption class using Win32 CryptoAPI.                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AesCAPI.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.hpp"

// References:
// - http://www.codeproject.com/Tips/787096/Operation-Password-CryptoAPI-with-AES
//   [Google: "CryptoAPI decrypting AES example" (no quotes)]
// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/cryptography-portal
// - http://stackoverflow.com/questions/29636767/how-to-aes-cbc-encryption-using-cryptoapi
//   [Google: "CryptoAPI AES-CBC" (no quotes)]
// - http://www.codeproject.com/Articles/11578/Encryption-using-the-Win-Crypto-API
//   [Google: "CryptoAPI AES-CBC" (no quotes)]
// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--importing-a-plaintext-key
//   [Google: "CryptImportKey" (no quotes)]
// - http://etutorials.org/Programming/secure+programming/Chapter+5.+Symmetric+Encryption/5.25+Using+Symmetric+Encryption+with+Microsoft+s+CryptoAPI/
//   [Google: "CryptoAPI set IV" (no quotes)]
// - https://modexp.wordpress.com/2016/03/10/windows-ctr-mode-with-crypto-api/
//   [Google: "cryptoapi-ng aes-ctr" (no quotes)]
#include <wincrypt.h>

namespace LibRpBase {

class AesCAPIPrivate
{
	public:
		AesCAPIPrivate();
		~AesCAPIPrivate();

	private:
		RP_DISABLE_COPY(AesCAPIPrivate)

	public:
		// CryptoAPI provider.
		HCRYPTPROV hProvider;
		// CryptoAPI key.
		HCRYPTKEY hKey;

		// Chaining mode.
		IAesCipher::ChainingMode chainingMode;

		// Counter for CTR mode.
		uint8_t ctr[16];

		/**
		 * Set the chaining mode on a key.
		 * @param hKey HCRYPTKEY.
		 * @param mode Chaining mode.
		 * @return 0 on success; non-zero on error.
		 */
		static int setChainingMode(HCRYPTKEY hKey, IAesCipher::ChainingMode mode);
};

/** AesCAPIPrivate **/

AesCAPIPrivate::AesCAPIPrivate()
	: hProvider(0)
	, hKey(0)
	, chainingMode(IAesCipher::ChainingMode::ECB)
{
	// Clear the counter.
	memset(ctr, 0, sizeof(ctr));

	// Initialize the CryptoAPI provider.
	// TODO: Try multiple times, e.g.:
	// - https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--importing-a-plaintext-key
	// http://stackoverflow.com/questions/4495247/ms-crypto-api-behavior-on-windows-xp-vs-vista-7
	// MS_ENH_RSA_AES_PROV is the value for Windows 7, but it fails for XP.
	// XP expects MS_ENH_RSA_AES_PROV_XP, which has "(Prototype)".
	// Specifiyng nullptr should work in both cases.
	if (!CryptAcquireContext(&hProvider, nullptr, nullptr,
	    PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		// Unable to find an AES encryption provider.
		hProvider = 0;
	}
}

AesCAPIPrivate::~AesCAPIPrivate()
{
	if (hKey != 0) {
		CryptDestroyKey(hKey);
	}
	if (hProvider != 0) {
		CryptReleaseContext(hProvider, 0);
	}
}

/**
 * Set the chaining mode on a key.
 * @param hKey HCRYPTKEY.
 * @param mode Chaining mode.
 * @return 0 on success; non-zero on error.
 */
int AesCAPIPrivate::setChainingMode(HCRYPTKEY hKey, IAesCipher::ChainingMode mode)
{
	DWORD dwMode;
	switch (mode) {
		case IAesCipher::ChainingMode::ECB:
		case IAesCipher::ChainingMode::CTR:
			dwMode = CRYPT_MODE_ECB;
			break;
		case IAesCipher::ChainingMode::CBC:
			dwMode = CRYPT_MODE_CBC;
			break;
		default:
			return -EINVAL;
	}

	if (!CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0)) {
		// Error setting the chaining mode.
		return -w32err_to_posix(GetLastError());
	}

	// Chaining mode set.
	return 0;
}

/** AesCAPI **/

AesCAPI::AesCAPI()
	: d_ptr(new AesCAPIPrivate())
{ }

AesCAPI::~AesCAPI()
{
	delete d_ptr;
}

/**
 * Get the name of the AesCipher implementation.
 * @return Name.
 */
const char *AesCAPI::name(void) const
{
	return "CryptoAPI";
}

/**
 * Has the cipher been initialized properly?
 * @return True if initialized; false if not.
 */
bool AesCAPI::isInit(void) const
{
	RP_D(const AesCAPI);
	return (d->hProvider != 0);
}

/**
 * Set the encryption key.
 * @param pKey	[in] Key data.
 * @param size	[in] Size of pKey, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI::setKey(const uint8_t *RESTRICT pKey, size_t size)
{
	// Acceptable key lengths:
	// - 16 (AES-128)
	// - 24 (AES-192)
	// - 32 (AES-256)
	RP_D(AesCAPI);
	if (!pKey) {
		// No key specified.
		return -EINVAL;
	} else if (d->hProvider == 0) {
		// Provider is not available.
		return -EBADF;
	}

	ALG_ID alg_id;
	switch (size) {
		case 16:
			alg_id = CALG_AES_128;
			break;
		case 24:
			alg_id = CALG_AES_192;
			break;
		case 32:
			alg_id = CALG_AES_256;
			break;
		default:
			// Invalid key length.
			return -EINVAL;
	}

	// Create an AES key blob.
	// Reference: http://stackoverflow.com/questions/842357/hard-coded-aes-256-key-with-wincrypt-cryptimportkey
	struct aesblob {
		BLOBHEADER hdr;
		DWORD keySize;
		BYTE bytes[32];	// maximum size
	} blob;
	blob.hdr.bType = PLAINTEXTKEYBLOB;
	blob.hdr.bVersion = CUR_BLOB_VERSION;
	blob.hdr.reserved = 0;
	blob.hdr.aiKeyAlg = alg_id;
	blob.keySize = static_cast<DWORD>(size);
	memcpy(blob.bytes, pKey, size);

	// Calculate the blob size based on the
	// specified key size.
	const DWORD blobSize = static_cast<DWORD>(sizeof(blob.hdr) + sizeof(blob.keySize) + size);

	// Load the key.
	HCRYPTKEY hNewKey;
	if (!CryptImportKey(d->hProvider, (BYTE*)&blob, blobSize, 0, 0, &hNewKey)) {
		// Error loading the key.
		return -w32err_to_posix(GetLastError());
	}

	// Set the cipher chaining mode.
	int ret = d->setChainingMode(hNewKey, d->chainingMode);
	if (ret != 0) {
		// Error setting the chaining mode.
		CryptDestroyKey(hNewKey);
		return ret;
	}

	// Key loaded successfully.
	HCRYPTKEY hOldKey = d->hKey;
	d->hKey = hNewKey;
	if (hOldKey != 0) {
		// Destroy the old key.
		CryptDestroyKey(hOldKey);
	}
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
int AesCAPI::setChainingMode(ChainingMode mode)
{
	RP_D(AesCAPI);
	if (d->chainingMode == mode) {
		// No change...
		return 0;
	}

	// Save the chaining mode.
	d->chainingMode = mode;

	if (d->hKey) {
		// Set the chaining mode.
		return d->setChainingMode(d->hKey, mode);
	}

	// We can't apply the chaining mode yet,
	// since we don't have a key.
	return 0;
}

/**
 * Set the IV (CBC mode) or counter (CTR mode).
 * @param pIV	[in] IV/counter data.
 * @param size	[in] Size of pIV, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI::setIV(const uint8_t *RESTRICT pIV, size_t size)
{
	RP_D(AesCAPI);
	if (!pIV || size != 16) {
		return -EINVAL;
	} else if (d->hKey == 0) {
		// Key hasn't been set.
		return -EBADF;
	}

	switch (d->chainingMode) {
		case ChainingMode::ECB:
		default:
			// No IV.
			return -EINVAL;
		case ChainingMode::CBC:
			// Set the IV.
			if (!CryptSetKeyParam(d->hKey, KP_IV, pIV, 0)) {
				// Error setting the IV.
				return -w32err_to_posix(GetLastError());
			}
			break;
		case ChainingMode::CTR:
			// Set the counter.
			memcpy(d->ctr, pIV, size);
			break;
	}

	return 0;
}

/**
 * Decrypt a block of data.
 * Key and IV/counter must be set before calling this function.
 *
 * @param pData	[in/out] Data block.
 * @param size	[in] Length of data block. (Must be a multiple of 16.)
 * @return Number of bytes decrypted on success; 0 on error.
 */
size_t AesCAPI::decrypt(uint8_t *RESTRICT pData, size_t size)
{
	RP_D(AesCAPI);
	if (d->hKey == 0) {
		// Key hasn't been set.
		return 0;
	}

	if (size == 0) {
		// Nothing to decrypt...
		return 0;
	}

	assert(size % 16 == 0);
	if (size % 16 != 0) {
		// Not a multiple of the block size.
		return 0;
	}

	// Temporarily duplicate the key so we don't overwrite
	// the feedback register in the original key.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-cryptdecrypt
	HCRYPTKEY hMyKey;
	if (!CryptDuplicateKey(d->hKey, nullptr, 0, &hMyKey)) {
		// Error duplicating the key.
		return 0;
	}

	if (d->chainingMode == ChainingMode::CBC) {
		// Ensure we have the correct IV afterwards.
		// IV should be the last encrypted block.
		// TODO: Optimize things so we don't have to do this...
		setIV(&pData[size-16], 16);
	}

	// Decrypt the data.
	// NOTE: Specifying TRUE as the Final parameter results in
	// CryptDecrypt failing with NTE_BAD_DATA, even though the
	// data has the correct block length.
	DWORD dwLen;
	BOOL bRet = FALSE;
	if (d->chainingMode == ChainingMode::CTR) {
		// CTR isn't supported by CryptoAPI directly.
		// Need to decrypt each block manually.

		// Using a union to allow 64-bit XORs
		// for improved performance.
		// NOTE: Can't use u128_t, since that's in libromdata. (N3DSVerifyKeys.hpp)
		union ctr_block {
			uint8_t u8[16];
			uint64_t u64[2];
		};

		// TODO: Verify data alignment.
		ctr_block ctr_crypt;
		ctr_block *ctr_data = reinterpret_cast<ctr_block*>(pData);
		dwLen = 0;
		for (; size > 0; size -= 16, ctr_data++) {
			// Encrypt the current counter.
			memcpy(ctr_crypt.u8, d->ctr, sizeof(ctr_crypt.u8));
			DWORD dwTempLen = 16;
			bRet = CryptEncrypt(hMyKey, 0, FALSE, 0, ctr_crypt.u8, &dwTempLen, sizeof(ctr_crypt.u8));
			if (!bRet) {
				// Encryption failed.
				break;
			}
			dwLen += dwTempLen;

			// XOR with the ciphertext.
			ctr_data->u64[0] ^= ctr_crypt.u64[0];
			ctr_data->u64[1] ^= ctr_crypt.u64[1];

			// Increment the counter.
			for (int i = 15; i >= 0; i--) {
				if (++d->ctr[i] != 0) {
					// No carry needed.
					break;
				}
			}
		}
	} else {
		// ECB and/or CBC.
		dwLen = static_cast<DWORD>(size);
		bRet = CryptDecrypt(hMyKey, 0, FALSE, 0, pData, &dwLen);
	}

	CryptDestroyKey(hMyKey);
	return (bRet ? dwLen : 0);
}

}

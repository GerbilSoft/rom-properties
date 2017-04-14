/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCAPI.cpp: AES decryption class using Win32 CryptoAPI.                *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "AesCAPI.hpp"
#include "../common.h"

// C includes. (C++ namespace)
#include <cerrno>

// References:
// - http://www.codeproject.com/Tips/787096/Operation-Password-CryptoAPI-with-AES
//   [Google: "CryptoAPI decrypting AES example" (no quotes)]
// - https://msdn.microsoft.com/en-us/library/windows/desktop/aa380255(v=vs.85).aspx
// - http://stackoverflow.com/questions/29636767/how-to-aes-cbc-encryption-using-cryptoapi
//   [Google: "CryptoAPI AES-CBC" (no quotes)]
// - http://www.codeproject.com/Articles/11578/Encryption-using-the-Win-Crypto-API
//   [Google: "CryptoAPI AES-CBC" (no quotes)]
// - https://msdn.microsoft.com/en-us/library/windows/desktop/aa382383(v=vs.85).aspx
//   [Google: "CryptImportKey" (no quotes)]
// - http://etutorials.org/Programming/secure+programming/Chapter+5.+Symmetric+Encryption/5.25+Using+Symmetric+Encryption+with+Microsoft+s+CryptoAPI/
//   [Google: "CryptoAPI set IV" (no quotes)]
// - https://modexp.wordpress.com/2016/03/10/windows-ctr-mode-with-crypto-api/
//   [Google: "cryptoapi-ng aes-ctr" (no quotes)]
#include "../RpWin32.hpp"
#include <wincrypt.h>

namespace LibRomData {

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
	, chainingMode(IAesCipher::CM_ECB)
{
	// Clear the counter.
	memset(ctr, 0, sizeof(ctr));

	// Initialize the CryptoAPI provider.
	// TODO: Try multiple times, e.g.:
	// - https://msdn.microsoft.com/en-us/library/windows/desktop/aa382383(v=vs.85).aspx
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
		case IAesCipher::CM_ECB:
		case IAesCipher::CM_CTR:
			dwMode = CRYPT_MODE_ECB;
			break;
		case IAesCipher::CM_CBC:
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
const rp_char *AesCAPI::name(void) const
{
	return _RP("CryptoAPI");
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
 * @param key Key data.
 * @param len Key length, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI::setKey(const uint8_t *key, unsigned int len)
{
	// Acceptable key lengths:
	// - 16 (AES-128)
	// - 24 (AES-192)
	// - 32 (AES-256)
	RP_D(AesCAPI);
	if (!key) {
		// No key specified.
		return -EINVAL;
	} else if (d->hProvider == 0) {
		// Provider is not available.
		return -EBADF;
	}

	ALG_ID alg_id;
	switch (len) {
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
	blob.keySize = len;
	memcpy(blob.bytes, key, len);

	// Calculate the blob size based on the
	// specified key size.
	const DWORD blobSize = (DWORD)(sizeof(BLOBHEADER)+sizeof(DWORD)+len);

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
 * @param iv IV/counter data.
 * @param len IV/counter length, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI::setIV(const uint8_t *iv, unsigned int len)
{
	RP_D(AesCAPI);
	if (!iv || len != 16) {
		return -EINVAL;
	} else if (d->hKey == 0) {
		// Key hasn't been set.
		return -EBADF;
	}

	switch (d->chainingMode) {
		case CM_ECB:
		default:
			// No IV.
			return -EINVAL;
		case CM_CBC:
			// Set the IV.
			if (!CryptSetKeyParam(d->hKey, KP_IV, iv, 0)) {
				// Error setting the IV.
				return -w32err_to_posix(GetLastError());
			}
			break;
		case CM_CTR:
			// Set the counter.
			memcpy(d->ctr, iv, len);
			break;
	}

	return 0;
}

/**
 * Decrypt a block of data.
 * @param data Data block.
 * @param data_len Length of data block.
 * @return Number of bytes decrypted on success; 0 on error.
 */
unsigned int AesCAPI::decrypt(uint8_t *data, unsigned int data_len)
{
	RP_D(AesCAPI);
	if (d->hKey == 0) {
		// Key hasn't been set.
		return 0;
	}

	// FIXME: Nettle version doesn't do this, which allows
	// calling decrypt() multiple times for CBC with large
	// amounts of data.

	// Temporarily duplicate the key so we don't overwrite
	// the feedback register in the original key.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa379913(v=vs.85).aspx
	HCRYPTKEY hMyKey;
	if (!CryptDuplicateKey(d->hKey, nullptr, 0, &hMyKey)) {
		// Error duplicating the key.
		return 0;
	}

	// Decrypt the data.
	// NOTE: Specifying TRUE as the Final parameter results in
	// CryptDecrypt failing with NTE_BAD_DATA, even though the
	// data has the correct block length.
	DWORD dwLen;
	BOOL bRet = FALSE;
	if (d->chainingMode == CM_CTR) {
		// CTR isn't supported by CryptoAPI directly.
		// Need to decrypt each block manually.
		uint8_t ctr_crypt[16];
		dwLen = 0;
		for (; data_len > 0; data_len -= 16, data += 16) {
			// Encrypt the current counter.
			memcpy(ctr_crypt, d->ctr, sizeof(ctr_crypt));
			DWORD dwTempLen = 16;
			bRet = CryptEncrypt(hMyKey, 0, FALSE, 0, ctr_crypt, &dwTempLen, sizeof(ctr_crypt));
			if (!bRet) {
				// Encryption failed.
				return 0;
			}
			dwLen += dwTempLen;

			// XOR with the ciphertext.
			// TODO: Optimized XOR.
			for (int i = 15; i >= 0; i--) {
				data[i] ^= ctr_crypt[i];
			}

			// Increment the counter.
			for (int i = 15; i >= 0; i--) {
				if (++d->ctr[i] != 0) {
					// No carry needed.
					break;
				}
			}
		}
	} else {
		// EBC and/or CBC.
		dwLen = data_len;
		bRet = CryptDecrypt(hMyKey, 0, FALSE, 0, data, &dwLen);
		CryptDestroyKey(hMyKey);
	}

	return (bRet ? dwLen : 0);
}

/**
 * Decrypt a block of data using the specified IV (CBC mode) or counter (CTR mode).
 * @param data Data block.
 * @param data_len Length of data block.
 * @param iv IV/counter for the data block.
 * @param iv_len Length of the IV/counter.
 * @return Number of bytes decrypted on success; 0 on error.
 */
unsigned int AesCAPI::decrypt(uint8_t *data, unsigned int data_len,
	const uint8_t *iv, unsigned int iv_len)
{
	RP_D(AesCAPI);
	if (!iv || iv_len != 16) {
		// Invalid IV.
		return 0;
	} else if (d->hKey == 0) {
		// Key hasn't been set.
		return 0;
	}

	// Set the IV.
	if (!CryptSetKeyParam(d->hKey, KP_IV, iv, 0)) {
		// Error setting the IV.
		return 0;
	}

	// Use the regular decrypt() function.
	return decrypt(data, data_len);
}

}

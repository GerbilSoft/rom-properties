/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCAPI_NG.cpp: AES decryption class using Win32 CryptoAPI NG.          *
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

#include "AesCAPI_NG.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/aa376234%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
#include "../RpWin32.hpp"
#include <bcrypt.h>
#include <winternl.h>

namespace LibRomData {

class AesCAPI_NG_Private
{
	public:
		AesCAPI_NG_Private();
		~AesCAPI_NG_Private();

	private:
		AesCAPI_NG_Private(const AesCAPI_NG_Private &other);
		AesCAPI_NG_Private &operator=(const AesCAPI_NG_Private &other);

	public:
		// NOTE: While the provider is shared in AesCAPI,
		// it can't be shared here because properties like
		// chaining mode and IV are set on the algorithm
		// handle, not the key.
		BCRYPT_ALG_HANDLE hAesAlg;
		BCRYPT_KEY_HANDLE hKey;

		// Key object.
		uint8_t *pbKeyObject;
		ULONG cbKeyObject;

		// Initialization vector.
		// CryptoAPI NG doesn't store it in the key object,
		// unlike the older CryptoAPI.
		uint8_t iv[16];

		// Chaining mode.
		IAesCipher::ChainingMode chainingMode;
};

/** AesCAPI_NG_Private **/

AesCAPI_NG_Private::AesCAPI_NG_Private()
	: hAesAlg(nullptr)
	, hKey(nullptr)
	, pbKeyObject(nullptr)
	, cbKeyObject(0)
	, chainingMode(IAesCipher::CM_ECB)
{
	// Clear the IV.
	memset(iv, 0, sizeof(iv));

	BCRYPT_ALG_HANDLE hAesAlg;
	NTSTATUS status = BCryptOpenAlgorithmProvider(
		&hAesAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (NT_SUCCESS(status)) {
		// TODO: Can we just set this->hAesAlg
		// in the function call?
		this->hAesAlg = hAesAlg;
	}
}

AesCAPI_NG_Private::~AesCAPI_NG_Private()
{
	if (hKey) {
		BCryptDestroyKey(hKey);
	}

	if (hAesAlg) {
		BCryptCloseAlgorithmProvider(hAesAlg, 0);
	}

	free(pbKeyObject);
}

/** AesCAPI_NG **/

AesCAPI_NG::AesCAPI_NG()
	: d(new AesCAPI_NG_Private())
{ }

AesCAPI_NG::~AesCAPI_NG()
{
	delete d;
}

/**
 * Is CryptoAPI NG usable on this system?
 *
 * If CryptoAPI NG is usable, this function will load
 * bcrypt.dll and all required function pointers.
 *
 * @return True if this system supports CryptoAPI NG.
 */
bool AesCAPI_NG::isUsable(void)
{
	// TODO: Use function pointers.
	HMODULE hBcryptDll = LoadLibrary(L"bcrypt.dll");
	bool ret = (hBcryptDll != nullptr);
	FreeLibrary(hBcryptDll);
	return ret;
}

/**
 * Has the cipher been initialized properly?
 * @return True if initialized; false if not.
 */
bool AesCAPI_NG::isInit(void) const
{
	return (d->hAesAlg != nullptr);
}

/**
 * Set the encryption key.
 * @param key Key data.
 * @param len Key length, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setKey(const uint8_t *key, unsigned int len)
{
	// Acceptable key lengths:
	// - 16 (AES-128)
	// - 24 (AES-192)
	// - 32 (AES-256)
	if (!key) {
		// No key specified.
		return -EINVAL;
	} else if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	}

	if (len != 16 && len != 24 && len != 32) {
		// Invalid key length.
		return -EINVAL;
	}

	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa376234%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396

	// Calculate the buffer size for the key object.
	ULONG cbKeyObject;
	ULONG cbData;
	NTSTATUS status = BCryptGetProperty(
		d->hAesAlg,
		BCRYPT_OBJECT_LENGTH,
		(PBYTE)&cbKeyObject, sizeof(cbKeyObject),
		&cbData, 0);
	if (!NT_SUCCESS(status) || cbData != sizeof(cbKeyObject)) {
		// Failed to get the key object length.
		// TODO: Better error?
		return -ENOMEM;
	}

	uint8_t *pbKeyObject = reinterpret_cast<uint8_t*>(malloc(cbKeyObject));
	if (!pbKeyObject) {
		return -ENOMEM;
	}

	// Generate the key.
	BCRYPT_KEY_HANDLE hKey;
	status = BCryptGenerateSymmetricKey(
		d->hAesAlg,
		&hKey,
		pbKeyObject, cbKeyObject,
		(PBYTE)key, len, 0);
	if (!NT_SUCCESS(status)) {
		// Error generating the key.
		free(pbKeyObject);
		// TODO: Better error?
		return -ENOMEM;
	}

	// Key loaded successfully.
	BCRYPT_KEY_HANDLE hOldKey = d->hKey;
	uint8_t *pbOldKeyObject = d->pbKeyObject;
	d->hKey = hKey;
	d->pbKeyObject = pbKeyObject;
	d->cbKeyObject = cbKeyObject;
	if (hOldKey != nullptr) {
		// Destroy the old key.
		BCryptDestroyKey(hKey);
	}
	if (pbOldKeyObject != nullptr) {
		// Delete the old key blob.
		free(pbOldKeyObject);
	}

	return 0;
}

/**
 * Set the cipher chaining mode.
 * @param mode Cipher chaining mode.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setChainingMode(ChainingMode mode)
{
	// TODO: What is the default chaining mode?
	// TODO: Store the chaining mode as a property?
	if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	}

	const wchar_t *szMode;
	ULONG cbMode;
	switch (mode) {
		case CM_ECB:
			szMode = BCRYPT_CHAIN_MODE_ECB;
			cbMode = sizeof(BCRYPT_CHAIN_MODE_ECB);
			break;
		case CM_CBC:
			szMode = BCRYPT_CHAIN_MODE_CBC;
			cbMode = sizeof(BCRYPT_CHAIN_MODE_CBC);
			break;
		default:
			return -EINVAL;
	}

	// Set the cipher chaining mode.
	NTSTATUS status = BCryptSetProperty(
				d->hAesAlg, 
                                BCRYPT_CHAINING_MODE, 
                                (PBYTE)szMode, cbMode, 0);
	if (!NT_SUCCESS(status)) {
		// Error setting the cipher chaining mode.
		// TODO: Specific error?
		return -EIO;
	}

	d->chainingMode = mode;
	return 0;
}

/**
 * Set the IV.
 * @param iv IV data.
 * @param len IV length, in bytes.
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setIV(const uint8_t *iv, unsigned int len)
{
	if (!iv || len != 16) {
		return -EINVAL;
	} else if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	}

	// Verify the block length.
	ULONG cbBlockLen;
	ULONG cbData;
	NTSTATUS status = BCryptGetProperty(
				d->hAesAlg, 
				BCRYPT_BLOCK_LENGTH, 
				(PBYTE)&cbBlockLen, sizeof(DWORD),
				&cbData, 0);
	if (!NT_SUCCESS(status) || cbData != sizeof(cbBlockLen)) {
		// Failed to get the block length.
		// TODO: Better error?
		return -EIO;
	}

	if (cbBlockLen != 16) {
		// Block length is incorrect...
		return -EIO;
	}

	// Set the IV.
	assert(sizeof(d->iv) == 16);
	memcpy(d->iv, iv, sizeof(d->iv));
	return 0;
}

/**
 * Decrypt a block of data.
 * @param data Data block.
 * @param data_len Length of data block.
 * @return Number of bytes decrypted on success; 0 on error.
 */
unsigned int AesCAPI_NG::decrypt(uint8_t *data, unsigned int data_len)
{
	if (!d->hAesAlg || !d->hKey) {
		// Algorithm is not available,
		// or the key hasn't been set.
		return 0;
	}

	// Get the block length.
	ULONG cbBlockLen;
	ULONG cbData;
	NTSTATUS status = BCryptGetProperty(
				d->hAesAlg, 
				BCRYPT_BLOCK_LENGTH, 
				(PBYTE)&cbBlockLen, sizeof(DWORD),
				&cbData, 0);
	if (!NT_SUCCESS(status) || cbData != sizeof(cbBlockLen)) {
		// Failed to get the block length.
		return 0;
	}

	if (cbBlockLen != 16) {
		// Block length is incorrect...
		return 0;
	}

	// data_len must be a multiple of the block length.
	assert(data_len % cbBlockLen == 0);
	if (data_len % cbBlockLen != 0) {
		// Invalid data length.
		return 0;
	}

	ULONG cbResult;
	switch (d->chainingMode) {
		case CM_ECB:
			status = BCryptDecrypt(d->hKey,
						data, data_len,
						nullptr,
						nullptr, 0,
						data, data_len,
						&cbResult, 0);
			break;

		case CM_CBC:
			status = BCryptDecrypt(d->hKey,
						data, data_len,
						nullptr,
						d->iv, sizeof(d->iv),
						data, data_len,
						&cbResult, 0);
			break;

		default:
			// Invalid chaining mode.
			return 0;
	}
	
	return (NT_SUCCESS(status) ? cbResult : 0);
}

/**
 * Decrypt a block of data using the specified IV.
 * @param data Data block.
 * @param data_len Length of data block.
 * @param iv IV for the data block.
 * @param iv_len Length of the IV.
 * @return Number of bytes decrypted on success; 0 on error.
 */
unsigned int AesCAPI_NG::decrypt(uint8_t *data, unsigned int data_len,
	const uint8_t *iv, unsigned int iv_len)
{
	if (!d->hAesAlg || !d->hKey) {
		// Algorithm is not available,
		// or the key hasn't been set.
		return 0;
	} else if (!iv || iv_len != 16) {
		// Invalid IV.
		return 0;
	}

	// Get the block length.
	ULONG cbBlockLen;
	ULONG cbData;
	NTSTATUS status = BCryptGetProperty(
				d->hAesAlg, 
				BCRYPT_BLOCK_LENGTH, 
				(PBYTE)&cbBlockLen, sizeof(DWORD),
				&cbData, 0);
	if (!NT_SUCCESS(status) || cbData != sizeof(cbBlockLen)) {
		// Failed to get the block length.
		return 0;
	}

	if (cbBlockLen != 16) {
		// Block length is incorrect...
		return 0;
	}

	// data_len must be a multiple of the block length.
	assert(data_len % cbBlockLen == 0);
	if (data_len % cbBlockLen != 0) {
		// Invalid data length.
		return 0;
	}

	// Set the IV.
	assert(sizeof(d->iv) == 16);
	memcpy(d->iv, iv, sizeof(d->iv));

	ULONG cbResult;
	switch (d->chainingMode) {
		case CM_ECB:
			status = BCryptDecrypt(d->hKey,
						data, data_len,
						nullptr,
						nullptr, 0,
						data, data_len,
						&cbResult, 0);
			break;

		case CM_CBC:
			status = BCryptDecrypt(d->hKey,
						data, data_len,
						nullptr,
						d->iv, sizeof(d->iv),
						data, data_len,
						&cbResult, 0);
			break;

		default:
			// Invalid chaining mode.
			return 0;
	}

	return (NT_SUCCESS(status) ? cbResult : 0);
}

}

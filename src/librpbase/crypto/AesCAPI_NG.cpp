/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCAPI_NG.cpp: AES decryption class using Win32 CryptoAPI NG.          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"
#include "AesCAPI_NG.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "tcharx.h"

// HMODULE deleter for std::unique_ptr<>
#include "HMODULE_deleter.hpp"

// References:
// - https://docs.microsoft.com/en-us/windows/win32/seccng/encrypting-data-with-cng
#include <bcrypt.h>
#include <winternl.h>

// Workaround for RP_D() expecting the no-underscore naming convention.
#define AesCAPI_NGPrivate AesCAPI_NG_Private

// C includes (C++ namespace)
#include <cstdint>
#include <cstring>

// C++ STL classes
#include <array>
#include <mutex>
using std::array;
using std::unique_ptr;

namespace LibRpBase {

// FIXME: The Windows Vista method requires linking to bcrypt.dll,
// which requires delay-load stuff, which defeats the point of the
// whole GetProcAddress() setup. We'll use the GetProcAddress()
// version on all systems for now.
#define RP_SUPPORTS_WINDOWS_XP 1

#ifdef RP_SUPPORTS_WINDOWS_XP
namespace Private {

// Function pointer macro.
#define DECL_FUNCPTR(f) __typeof__(f) * pfn##f
#define DEF_FUNCPTR(f) __typeof__(f) * pfn##f = nullptr
#define LOAD_FUNCPTR(f) pfn##f = (__typeof__(f)*)GetProcAddress(hBcrypt_dll.get(), #f)

/** bcrypt.dll handle **/
static unique_ptr<HMODULE, HMODULE_deleter> hBcrypt_dll;
static std::once_flag once_bcrypt;

/** bcrypt.dll function pointers **/
static DEF_FUNCPTR(BCryptOpenAlgorithmProvider);
static DEF_FUNCPTR(BCryptGetProperty);
static DEF_FUNCPTR(BCryptSetProperty);
static DEF_FUNCPTR(BCryptCloseAlgorithmProvider);
static DEF_FUNCPTR(BCryptGenerateSymmetricKey);
static DEF_FUNCPTR(BCryptDecrypt);
static DEF_FUNCPTR(BCryptDestroyKey);
static DEF_FUNCPTR(BCryptEncrypt);

/**
 * Load bcrypt.dll and associated function pointers.
 * Called by std::call_once().
 *
 * Check if hBcrypt_dll is nullptr afterwards.
 */
static void init_bcrypt(void)
{
	// Attempt to load bcrypt.dll.
	hBcrypt_dll.reset(LoadLibraryEx(_T("bcrypt.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
	if (!hBcrypt_dll) {
		// bcrypt.dll not found.
		return;
	}

	// Load the function pointers.
	LOAD_FUNCPTR(BCryptOpenAlgorithmProvider);
	LOAD_FUNCPTR(BCryptGetProperty);
	LOAD_FUNCPTR(BCryptSetProperty);
	LOAD_FUNCPTR(BCryptCloseAlgorithmProvider);
	LOAD_FUNCPTR(BCryptGenerateSymmetricKey);
	LOAD_FUNCPTR(BCryptDecrypt);
	LOAD_FUNCPTR(BCryptDestroyKey);
	LOAD_FUNCPTR(BCryptEncrypt);

	// Make sure all of the function pointers are valid.
	if (!pfnBCryptOpenAlgorithmProvider ||
	    !pfnBCryptGetProperty ||
	    !pfnBCryptSetProperty ||
	    !pfnBCryptCloseAlgorithmProvider ||
	    !pfnBCryptGenerateSymmetricKey ||
	    !pfnBCryptDecrypt ||
	    !pfnBCryptDestroyKey ||
	    !pfnBCryptEncrypt)
	{
		// At least one function pointer is missing.
		pfnBCryptOpenAlgorithmProvider = nullptr;
		pfnBCryptGetProperty = nullptr;
		pfnBCryptSetProperty = nullptr;
		pfnBCryptCloseAlgorithmProvider = nullptr;
		pfnBCryptGenerateSymmetricKey = nullptr;
		pfnBCryptDecrypt = nullptr;
		pfnBCryptDestroyKey = nullptr;
		pfnBCryptEncrypt = nullptr;

		hBcrypt_dll.reset();
		return;
	}

	// bcrypt.dll has been loaded.
}

}

#  define _BCryptOpenAlgorithmProvider(phAlgorithm, pszAlgId, pszImplementation, dwFlags) \
	Private::pfnBCryptOpenAlgorithmProvider((phAlgorithm), (pszAlgId), (pszImplementation), (dwFlags))
#  define _BCryptGetProperty(hObject, pszProperty, pbOutput, cbOutput, pcbResult, dwFlags) \
	Private::pfnBCryptGetProperty((hObject), (pszProperty), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#  define _BCryptSetProperty(hObject, pszProperty, pbInput, cbInput, dwFlags) \
	Private::pfnBCryptSetProperty((hObject), (pszProperty), (pbInput), (cbInput), (dwFlags))
#  define _BCryptCloseAlgorithmProvider(hAlgorithm, dwFlags) \
	Private::pfnBCryptCloseAlgorithmProvider((hAlgorithm), (dwFlags))
#  define _BCryptGenerateSymmetricKey(hAlgorithm, phKey, pbKeyObject, cbKeyObject, pbSecret, cbSecret, dwFlags) \
	Private::pfnBCryptGenerateSymmetricKey((hAlgorithm), (phKey), (pbKeyObject), (cbKeyObject), (pbSecret), (cbSecret), (dwFlags))
#  define _BCryptDecrypt(hKey, pInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags) \
	Private::pfnBCryptDecrypt((hKey), (pInput), (cbInput), (pPaddingInfo), (pbIV), (cbIV), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#  define _BCryptDestroyKey(hKey) \
	Private::pfnBCryptDestroyKey(hKey)
#  define _BCryptEncrypt(hKey, pbInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags) \
	Private::pfnBCryptEncrypt((hKey), (pbInput), (cbInput), (pPaddingInfo), (pbIV), (cbIV), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#else /* !RP_SUPPORTS_WINDOWS_XP */
#  define _BCryptOpenAlgorithmProvider(phAlgorithm, pszAlgId, pszImplementation, dwFlags) \
	BCryptOpenAlgorithmProvider((phAlgorithm), (pszAlgId), (pszImplementation), (dwFlags))
#  define _BCryptGetProperty(hObject, pszProperty, pbOutput, cbOutput, pcbResult, dwFlags) \
	BCryptGetProperty((hObject), (pszProperty), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#  define _BCryptSetProperty(hObject, pszProperty, pbInput, cbInput, dwFlags) \
	BCryptSetProperty((hObject), (pszProperty), (pbInput), (cbInput), (dwFlags))
#  define _BCryptCloseAlgorithmProvider(hAlgorithm, dwFlags) \
	BCryptCloseAlgorithmProvider((hAlgorithm), (dwFlags))
#  define _BCryptGenerateSymmetricKey(hAlgorithm, phKey, pbKeyObject, cbKeyObject, pbSecret, cbSecret, dwFlags) \
	BCryptGenerateSymmetricKey((hAlgorithm), (phKey), (pbKeyObject), (cbKeyObject), (pbSecret), (cbSecret), (dwFlags))
#  define _BCryptDecrypt(hKey, pInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags) \
	BCryptDecrypt((hKey), (pInput), (cbInput), (pPaddingInfo), (pbIV), (cbIV), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#  define _BCryptDestroyKey(hKey) \
	BCryptDestroyKey(hKey)
#  define _BCryptEncrypt(hKey, pbInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags) \
	BCryptEncrypt((hKey), (pbInput), (cbInput), (pPaddingInfo), (pbIV), (cbIV), (pbOutput), (cbOutput), (pcbResult), (dwFlags))
#endif /* RP_SUPPORTS_WINDOWS_XP */

class AesCAPI_NG_Private
{
public:
	AesCAPI_NG_Private();
	~AesCAPI_NG_Private();

private:
	RP_DISABLE_COPY(AesCAPI_NG_Private)

public:
	// NOTE: While the provider is shared in AesCAPI,
	// it can't be shared here because properties like
	// chaining mode and IV are set on the algorithm
	// handle, not the key.
	BCRYPT_ALG_HANDLE hAesAlg;
	BCRYPT_KEY_HANDLE hKey;

	// Key object
	uint8_t *pbKeyObject;
	ULONG cbKeyObject;

	// Key data
	// If the cipher mode is changed, the key
	// has to be reinitialized.
	array<uint8_t, 32> key;
	unsigned int key_len;

	// Chaining mode
	IAesCipher::ChainingMode chainingMode;

	// CBC: Initialization vector
	// CTR: Counter
	array<uint8_t, 16> iv;
};

/** AesCAPI_NG_Private **/

AesCAPI_NG_Private::AesCAPI_NG_Private()
	: hAesAlg(nullptr)
	, hKey(nullptr)
	, pbKeyObject(nullptr)
	, cbKeyObject(0)
	, key_len(0)
	, chainingMode(IAesCipher::ChainingMode::ECB)
{
	// Clear the key and IV.
	key.fill(0);
	iv.fill(0);

#ifdef RP_SUPPORTS_WINDOWS_XP
	// Attempt to load bcrypt.dll.
	std::call_once(Private::once_bcrypt, Private::init_bcrypt);
	if (!Private::hBcrypt_dll) {
		// Error loading bcrypt.dll.
		return;
	}
#endif /* RP_SUPPORTS_WINDOWS_XP */

	BCRYPT_ALG_HANDLE hAesAlg;
	NTSTATUS status = _BCryptOpenAlgorithmProvider(
		&hAesAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
	if (NT_SUCCESS(status)) {
		// Default to ECB chaining.
		status = _BCryptSetProperty(
				hAesAlg, 
				BCRYPT_CHAINING_MODE, 
				(PBYTE)BCRYPT_CHAIN_MODE_ECB,
				sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
		if (NT_SUCCESS(status)) {
			// Save the algorithm.
			this->hAesAlg = hAesAlg;
		} else {
			// Error setting the chaining mode.
			_BCryptCloseAlgorithmProvider(hAesAlg, 0);
		}
	}
}

AesCAPI_NG_Private::~AesCAPI_NG_Private()
{
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (Private::hBcrypt_dll)
#endif /* RP_SUPPORTS_WINDOWS_XP */
	{
		if (hKey) {
			_BCryptDestroyKey(hKey);
		}
		if (hAesAlg) {
			_BCryptCloseAlgorithmProvider(hAesAlg, 0);
		}
	}

	free(pbKeyObject);
}

/** AesCAPI_NG **/

AesCAPI_NG::AesCAPI_NG()
	: d_ptr(new AesCAPI_NG_Private())
{ }

AesCAPI_NG::~AesCAPI_NG()
{
	delete d_ptr;
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
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (Private::hBcrypt_dll) {
		// bcrypt.dll is loaded.
		return true;
	}

	// NOTE: We can't call init_bcrypt() due to reference counting,
	// so assume it works as long as bcrypt.dll is present and
	// BCryptOpenAlgorithmProvider exists.
	// NOTE 2: LOAD_LIBRARY_SEARCH_SYSTEM32 requires Windows Vista or later,
	// which works for us because bcrypt.dll was first added in Windows Vista.
	bool bRet = false;
	HMODULE tmp_hBcrypt_dll = LoadLibraryEx(_T("bcrypt.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (tmp_hBcrypt_dll) {
		bRet = (GetProcAddress(tmp_hBcrypt_dll, "BCryptOpenAlgorithmProvider") != nullptr);
		FreeLibrary(tmp_hBcrypt_dll);
	}
	return bRet;
#else /* !RP_SUPPORTS_WINDOWS_XP */
	// Compiled for Vista or later. bcrypt.dll is *always* loaded.
	return true;
#endif /* RP_SUPPORTS_WINDOWS_XP */
}

/**
 * Get the name of the AesCipher implementation.
 * @return Name
 */
const char *AesCAPI_NG::name(void) const
{
	return "CryptoAPI NG";
}

/**
 * Has the cipher been initialized properly?
 * @return True if initialized; false if not.
 */
bool AesCAPI_NG::isInit(void) const
{
	RP_D(const AesCAPI_NG);
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (!Private::hBcrypt_dll) {
		return false;
	}
#endif /* RP_SUPPORTS_WINDOWS_XP */
	return (d->hAesAlg != nullptr);
}

/**
 * Set the encryption key.
 * @param pKey	[in] Key data
 * @param size	[in] Size of pKey, in bytes
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setKey(const uint8_t *RESTRICT pKey, size_t size)
{
	// Acceptable key lengths:
	// - 16 (AES-128)
	// - 24 (AES-192)
	// - 32 (AES-256)
	RP_D(AesCAPI_NG);
	if (!pKey) {
		// No key specified.
		return -EINVAL;
	}
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (!Private::hBcrypt_dll) {
		// bcrypt.dll is not available.
		return -EBADF;
	}
#endif /* RP_SUPPORTS_WINDOWS_XP */
	if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	}

	if (size != 16 && size != 24 && size != 32) {
		// Invalid key length.
		return -EINVAL;
	}

	// Reference: https://docs.microsoft.com/en-us/windows/win32/seccng/encrypting-data-with-cng

	// Calculate the buffer size for the key object.
	ULONG cbKeyObject;
	ULONG cbData;
	NTSTATUS status = _BCryptGetProperty(
		d->hAesAlg,
		BCRYPT_OBJECT_LENGTH,
		(PBYTE)&cbKeyObject, sizeof(cbKeyObject),
		&cbData, 0);
	if (!NT_SUCCESS(status) || cbData != sizeof(cbKeyObject)) {
		// Failed to get the key object length.
		// TODO: Better error?
		return -ENOMEM;
	}

	uint8_t *const pbKeyObject = static_cast<uint8_t*>(malloc(cbKeyObject));
	if (!pbKeyObject) {
		return -ENOMEM;
	}

	// Generate the key.
	BCRYPT_KEY_HANDLE hKey;
	status = _BCryptGenerateSymmetricKey(
		d->hAesAlg,
		&hKey,
		pbKeyObject, cbKeyObject,
		(PBYTE)pKey, (ULONG)size, 0);
	if (!NT_SUCCESS(status)) {
		// Error generating the key.
		free(pbKeyObject);
		// TODO: Better error?
		return -ENOMEM;
	}

	// Key loaded successfully.
	BCRYPT_KEY_HANDLE hOldKey = d->hKey;
	uint8_t *const pbOldKeyObject = d->pbKeyObject;
	d->hKey = hKey;
	d->pbKeyObject = pbKeyObject;
	d->cbKeyObject = cbKeyObject;
	if (hOldKey != nullptr) {
		// Destroy the old key.
		_BCryptDestroyKey(hOldKey);
	}
	// Delete the old key blob.
	free(pbOldKeyObject);

	// Save the key data.
	memcpy(d->key.data(), pKey, size);
	d->key_len = static_cast<unsigned int>(size);
	return 0;
}

/**
 * Set the cipher chaining mode.
 *
 * Note that the IV/counter must be set *after* setting
 * the chaining mode; otherwise, setIV() will fail.
 *
 * @param mode Cipher chaining mode
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setChainingMode(ChainingMode mode)
{
	RP_D(AesCAPI_NG);
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (!Private::hBcrypt_dll) {
		// bcrypt.dll is not available.
		return -EBADF;
	}
#endif /* RP_SUPPORTS_WINDOWS_XP */
	if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	} else if (d->chainingMode == mode) {
		// No change necessary.
		return 0;
	}

	const wchar_t *szMode;
	ULONG cbMode;
	switch (mode) {
		case ChainingMode::ECB:
		case ChainingMode::CTR:	// implemented using ECB
			szMode = BCRYPT_CHAIN_MODE_ECB;
			cbMode = sizeof(BCRYPT_CHAIN_MODE_ECB);
			break;
		case ChainingMode::CBC:
			szMode = BCRYPT_CHAIN_MODE_CBC;
			cbMode = sizeof(BCRYPT_CHAIN_MODE_CBC);
			break;
		default:
			return -EINVAL;
	}

	// Set the cipher chaining mode on the algorithm.
	NTSTATUS status = _BCryptSetProperty(
				d->hAesAlg, 
                                BCRYPT_CHAINING_MODE, 
                                (PBYTE)szMode, cbMode, 0);
	if (!NT_SUCCESS(status)) {
		// Error setting the cipher chaining mode.
		// TODO: Specific error?
		return -EIO;
	}

	d->chainingMode = mode;

	// Re-apply the key.
	// Otherwise, the chaining mode won't take effect.
	setKey(d->key.data(), d->key_len);
	return 0;
}

/**
 * Set the IV (CBC mode) or counter (CTR mode).
 * @param pIV	[in] IV/counter data
 * @param size	[in] Size of pIV, in bytes
 * @return 0 on success; negative POSIX error code on error.
 */
int AesCAPI_NG::setIV(const uint8_t *RESTRICT pIV, size_t size)
{
	RP_D(AesCAPI_NG);
	if (!pIV || size != 16) {
		return -EINVAL;
	}
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (!Private::hBcrypt_dll) {
		// bcrypt.dll is not available.
		return -EBADF;
	}
#endif /* #ifdef RP_SUPPORTS_WINDOWS_XP */
	if (!d->hAesAlg) {
		// Algorithm is not available.
		return -EBADF;
	} else if (d->chainingMode < ChainingMode::CBC || d->chainingMode >= ChainingMode::Max) {
		// This chaining mode doesn't have an IV or counter.
		return -EINVAL;
	}

	// Verify the block length.
	ULONG cbBlockLen;
	ULONG cbData;
	NTSTATUS status = _BCryptGetProperty(
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
	assert(d->iv.size() == 16);
	memcpy(d->iv.data(), pIV, d->iv.size());
	return 0;
}

/**
 * Decrypt a block of data.
 * Key and IV/counter must be set before calling this function.
 *
 * @param pData	[in/out] Data block
 * @param size	[in] Length of data block (Must be a multiple of 16)
 * @return Number of bytes decrypted on success; 0 on error.
 */
size_t AesCAPI_NG::decrypt(uint8_t *RESTRICT pData, size_t size)
{
	RP_D(AesCAPI_NG);
#ifdef RP_SUPPORTS_WINDOWS_XP
	if (!Private::hBcrypt_dll) {
		// bcrypt.dll is not available.
		return 0;
	}
#endif /* RP_SUPPORTS_WINDOWS_XP */
	if (!d->hAesAlg || !d->hKey) {
		// Algorithm is not available,
		// or the key hasn't been set.
		return 0;
	}

	// Get the block length.
	ULONG cbBlockLen;
	ULONG cbData;
	NTSTATUS status = _BCryptGetProperty(
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

	// size must be a multiple of the block length.
	assert(size % cbBlockLen == 0);
	if (size % cbBlockLen != 0) {
		// Invalid data length.
		return 0;
	}

	ULONG cbResult;
	switch (d->chainingMode) {
		case ChainingMode::ECB:
			status = _BCryptDecrypt(d->hKey,
						pData, (ULONG)size,
						nullptr,
						nullptr, 0,
						pData, (ULONG)size,
						&cbResult, 0);
			break;

		case ChainingMode::CBC:
			status = _BCryptDecrypt(d->hKey,
						pData, (ULONG)size,
						nullptr,
						d->iv.data(), (ULONG)d->iv.size(),
						pData, (ULONG)size,
						&cbResult, 0);
			break;

		case ChainingMode::CTR: {
			// CTR isn't supported by CryptoAPI-NG directly.
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
			ULONG cbTmpResult;
			cbResult = 0;
			for (; size > 0; size -= 16, ctr_data++) {
				// Encrypt the current counter.
				memcpy(ctr_crypt.u8, d->iv.data(), sizeof(ctr_crypt.u8));
				status = _BCryptEncrypt(d->hKey,
						ctr_crypt.u8, sizeof(ctr_crypt.u8),
						nullptr,
						nullptr, 0,
						ctr_crypt.u8, sizeof(ctr_crypt.u8),
						&cbTmpResult, 0);
				if (!NT_SUCCESS(status)) {
					// Encryption failed.
					return 0;
				}
				cbResult += cbTmpResult;

				// XOR with the ciphertext.
				ctr_data->u64[0] ^= ctr_crypt.u64[0];
				ctr_data->u64[1] ^= ctr_crypt.u64[1];

				// Increment the counter.
				for (int i = 15; i >= 0; i--) {
					if (++d->iv[i] != 0) {
						// No carry needed.
						break;
					}
				}
			}
			break;
		}

		default:
			// Invalid chaining mode.
			return 0;
	}
	
	return (NT_SUCCESS(status) ? cbResult : 0);
}

}

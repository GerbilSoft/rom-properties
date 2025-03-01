/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCipherFactory.cpp: IAesCipher factory class.                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AesCipherFactory.hpp"

// IAesCipher implementations.
#ifdef _WIN32
#  include "AesCAPI.hpp"
#  include "AesCAPI_NG.hpp"
#endif
#ifdef HAVE_NETTLE
#  include "AesNettle.hpp"
#endif

namespace LibRpBase { namespace AesCipherFactory {

/**
 * Create an IAesCipher class.
 *
 * The implementation is chosen depending on the system
 * environment. The caller doesn't need to know what
 * the underlying implementation is.
 *
 * @return IAesCipher class, or nullptr if decryption isn't supported
 */
IAesCipher *create(void)
{
#ifdef ENABLE_DECRYPTION

#if defined(_WIN32)
	// Windows: Use CryptoAPI NG if available.
	// If not, fall back to CryptoAPI.
	if (AesCAPI_NG::isUsable()) {
		// CryptoAPI NG is available.
		// NOTE: Wine (as of 2.5) has CryptoAPI NG, but it
		// doesn't actually implement any encryption algorithms,
		// so we can't use it. We'll need to verify that AES
		// is initialized before returning the AesCAPI_NG object.
		// Wine's CryptoAPI implementation *does* support AES.
		IAesCipher *cipher = new AesCAPI_NG();
		if (cipher->isInit()) {
			return cipher;
		}
		// AES isn't working in bcrypt.
		delete cipher;
	}

	// CryptoAPI NG is not available.
	// Fall back to CryptoAPI.
	return new AesCAPI();
#elif defined(HAVE_NETTLE)
	// Other: Use nettle.
	return new AesNettle();
#endif

#endif /* ENABLE_DECRYPTION */

	// Decryption is not supported.
	return nullptr;
}

/**
 * Create an IAesCipher object.
 *
 * The implementation can be selected by the caller.
 * This is usually only used for test suites.
 *
 * @return IAesCipher class, or nullptr if decryption or the selected implementation isn't supported
 */
IAesCipher *create(Implementation implementation)
{
	IAesCipher *cipher = nullptr;

#ifdef ENABLE_DECRYPTION
	switch (implementation) {
		default:
			break;

#ifdef _WIN32
		case Implementation::CAPI:
			cipher = new AesCAPI();
			break;
		case Implementation::CAPI_NG:
			if (AesCAPI_NG::isUsable()) {
				cipher = new AesCAPI_NG();
			}
			break;
#endif /* _WIN32 */
#ifdef HAVE_NETTLE
		case Implementation::Nettle:
			cipher = new AesNettle();
			break;
#endif /* HAVE_NETTLE */
	}
#endif /* ENABLE_DECRYPTION */

	return cipher;
}

} }

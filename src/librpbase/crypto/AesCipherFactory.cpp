/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * AesCipherFactory.cpp: IAesCipher factory class.                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "AesCipherFactory.hpp"

// IAesCipher implementations.
#if defined(_WIN32)
# include "AesCAPI.hpp"
# include "AesCAPI_NG.hpp"
#elif defined(HAVE_NETTLE)
# include "AesNettle.hpp"
#endif

namespace LibRpBase {

/**
 * Create an IAesCipher class.
 *
 * The implementation is chosen depending on the system
 * environment. The caller doesn't need to know what
 * the underlying implementation is.
 *
 * @return IAesCipher class, or nullptr if decryption isn't supported
 */
IAesCipher *AesCipherFactory::create(void)
{
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

	// Decryption is not supported.
	return nullptr;
}

}

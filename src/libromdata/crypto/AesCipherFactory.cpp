/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AesCipherFactory.cpp: IAesCipher factory class.                         *
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

#include "config.libromdata.h"

#include "AesCipherFactory.hpp"
#include "IAesCipher.hpp"

// IAesCipher implementations.
#if defined(_WIN32)
# include "AesCAPI.hpp"
# include "AesCAPI_NG.hpp"
#elif defined(HAVE_NETTLE)
# include "AesNettle.hpp"
#endif

namespace LibRomData {

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

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KeyManager.hpp: Encryption key manager.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CRYPTO_KEYMANAGER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CRYPTO_KEYMANAGER_HPP__

#include "common.h"

// C includes.
#include <stdint.h>

namespace LibRomData {

class KeyManagerPrivate;
class KeyManager
{
	public:
		KeyManager();
		~KeyManager();

	private:
		RP_DISABLE_COPY(KeyManager)

	private:
		friend class KeyManagerPrivate;
		KeyManagerPrivate *const d;

	public:
		// Encryption key data.
		struct KeyData_t {
			const uint8_t *key;	// Key data.
			uint32_t length;	// Key length.
		};

		/**
		 * Have the encryption keys been loaded yet?
		 *
		 * This function will *not* load the keys.
		 * To load the keys, call get() with the requested key name.
		 *
		 * If this function returns false after calling get(),
		 * keys.conf is probably missing.
		 *
		 * @return True if keys have been loaded; false if not.
		 */
		bool areKeysLoaded(void) const;

		/**
		 * Reload keys if the key configuration file has changed.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int reloadIfChanged(void);

		/**
		 * Get an encryption key.
		 * @param keyName	[in]  Encryption key name.
		 * @param pKeyData	[out] Key data struct.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int get(const char *keyName, KeyData_t *pKeyData) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_KEYMANAGER_HPP__ */

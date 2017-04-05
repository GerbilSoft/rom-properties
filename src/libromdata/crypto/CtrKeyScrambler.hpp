/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CtrKeyScrambler.hpp: Nintendo 3DS key scrambler.                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_CRYPTO_CTRKEYSCRAMBLER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CRYPTO_CTRKEYSCRAMBLER_HPP__

#include "config.libromdata.h"
#ifndef ENABLE_DECRYPTION
#error This file should only be included if decryption is enabled.
#endif /* ENABLE_DECRYPTION */

#include "common.h"
#include <stdint.h>

namespace LibRomData {

// 128-bit type for AES counter and keys.
// TODO: Move to another file.
typedef union {
	uint8_t u8[16];
	uint32_t u32[4];
	uint64_t u64[2];
} u128_t;

class CtrKeyScrambler
{
	private:
		// Static class.
		CtrKeyScrambler();
		~CtrKeyScrambler();
		RP_DISABLE_COPY(CtrKeyScrambler)

	public:
		/**
		 * CTR key scrambler. (for keyslots 0x04-0x3F)
		 * @param keyNormal	[out] Normal key.
		 * @param keyX		[in] KeyX.
		 * @param keyY		[in] KeyY.
		 * @param ctr_scrambler	[in] Scrambler constant.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int CtrScramble(u128_t *keyNormal,
			const u128_t *keyX, const u128_t *keyY,
			const u128_t *ctr_scrambler);

		/**
		 * CTR key scrambler. (for keyslots 0x04-0x3F)
		 *
		 * "ctr-scrambler" is retrieved from KeyManager and is
		 * used as the scrambler constant.
		 *
		 * @param keyNormal	[out] Normal key.
		 * @param keyX		[in] KeyX.
		 * @param keyY		[in] KeyY.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int CtrScramble(u128_t *keyNormal,
			const u128_t *keyX, const u128_t *keyY);

	public:
		/**
		 * Get the total number of encryption key names.
		 * @return Number of encryption key names.
		 */
		static int encryptionKeyCount_static(void);

		/**
		 * Get an encryption key name.
		 * @param keyIdx Encryption key index.
		 * @return Encryption key name (in ASCII), or nullptr on error.
		 */
		static const char *encryptionKeyName_static(int keyIdx);

		/**
		 * Get the verification data for a given encryption key index.
		 * @param keyIdx Encryption key index.
		 * @return Verification data. (16 bytes)
		 */
		static const uint8_t *encryptionVerifyData_static(int keyIdx);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_CTRKEYSCRAMBLER_HPP__ */

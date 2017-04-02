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

class CtrKeyScramblerPrivate;
class CtrKeyScrambler
{
	protected:
		/**
		 * CtrKeyScrambler class.
		 *
		 * This class is a Singleton, so the caller must obtain a
		 * pointer to the class using instance().
		 */
		CtrKeyScrambler();
		~CtrKeyScrambler();

	private:
		RP_DISABLE_COPY(CtrKeyScrambler)

	private:
		friend class CtrKeyScramblerPrivate;
		CtrKeyScramblerPrivate *const d_ptr;

	public:
		/**
		 * Get the CtrKeyScrambler instance.
		 * @return CtrKeyScrambler instance.
		 */
		static CtrKeyScrambler *instance(void);

	public:
		/**
		 * CTR key scrambler. (for keyslots 0x04-0x3F)
		 * @param keyNormal	[out] Normal key.
		 * @param keyX		[in] KeyX.
		 * @param keyY		[in] KeyY.
		 * @return 0 on success; non-zero on error.
		 */
		int CtrScramble(u128_t *keyNormal, const u128_t *keyX, const u128_t *keyY);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CRYPTO_CTRKEYSCRAMBLER_HPP__ */

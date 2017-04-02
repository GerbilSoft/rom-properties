/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CtrKeyScrambler.cpp: Nintendo 3DS key scrambler.                        *
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

#include "CtrKeyScrambler.hpp"
#include "byteswap.h"

#include "KeyManager.hpp"

// C includes. (C++ namespace)
#include <cstring>
#include <cerrno>

namespace LibRomData {

/**
 * CTR key scrambler. (for keyslots 0x04-0x3F)
 * @param keyNormal	[out] Normal key.
 * @param keyX		[in] KeyX.
 * @param keyY		[in] KeyY.
 * @return 0 on success; non-zero on error.
 */
int CtrKeyScrambler::CtrScramble(u128_t *keyNormal, const u128_t *keyX, const u128_t *keyY)
{
	// CTR key scrambler: KeyNormal = (((KeyX <<< 2) ^ KeyY) + constant) <<< 87
	// NOTE: Since C doesn't have 128-bit types, we'll operate on
	// 32-bit types. This requires some byteswapping, since the
	// key is handled as if it's big-endian.
	// TODO: Operate on 64-bit types if building for 64-bit?

	// Load the key scrambler constant.
	KeyManager *const keyManager = KeyManager::instance();
	if (!keyManager) {
		// Unable to initialize the KeyManager.
		return -EIO;
	}

	KeyManager::KeyData_t keyData;
	if (keyManager->get("ctr-scrambler", &keyData) != 0) {
		// Key not found.
		return -ENOENT;
	}
	if (!keyData.key || keyData.length != 16) {
		// Key is not valid.
		return -EIO;	// TODO: Better error code?
	}

	// Key scrambler constant loaded.
	u128_t ctr_scrambler;
	memcpy(ctr_scrambler.u8, keyData.key, 16);
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	// Byteswap the key scrambler constant.
	ctr_scrambler.u32[0] = be32_to_cpu(ctr_scrambler.u32[0]);
	ctr_scrambler.u32[1] = be32_to_cpu(ctr_scrambler.u32[1]);
	ctr_scrambler.u32[2] = be32_to_cpu(ctr_scrambler.u32[2]);
	ctr_scrambler.u32[3] = be32_to_cpu(ctr_scrambler.u32[3]);
#endif /* SYS_BYTEORDER == SYS_LIL_ENDIAN */

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	u128_t keyXtmp;
	keyXtmp.u32[0] = be32_to_cpu(keyX->u32[0]);
	keyXtmp.u32[1] = be32_to_cpu(keyX->u32[1]);
	keyXtmp.u32[2] = be32_to_cpu(keyX->u32[2]);
	keyXtmp.u32[3] = be32_to_cpu(keyX->u32[3]);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	const u128_t &keyXtmp = *keyX;
#endif

	// Rotate KeyX left by two.
	u128_t keyTmp;
	keyTmp.u32[0] = (keyXtmp.u32[0] << 2) | (keyXtmp.u32[1] >> 30);
	keyTmp.u32[1] = (keyXtmp.u32[1] << 2) | (keyXtmp.u32[2] >> 30);
	keyTmp.u32[2] = (keyXtmp.u32[2] << 2) | (keyXtmp.u32[3] >> 30);
	keyTmp.u32[3] = (keyXtmp.u32[3] << 2) | (keyXtmp.u32[0] >> 30);

	// XOR by KeyY.
	keyTmp.u32[0] ^= be32_to_cpu(keyY->u32[0]);
	keyTmp.u32[1] ^= be32_to_cpu(keyY->u32[1]);
	keyTmp.u32[2] ^= be32_to_cpu(keyY->u32[2]);
	keyTmp.u32[3] ^= be32_to_cpu(keyY->u32[3]);

	// Add the constant.
	// Reference for carry functionality: https://accu.org/index.php/articles/1849
	keyTmp.u32[3] += ctr_scrambler.u32[3];
	keyTmp.u32[2] += ctr_scrambler.u32[2] + (keyTmp.u32[3] < ctr_scrambler.u32[3]);
	keyTmp.u32[1] += ctr_scrambler.u32[1] + (keyTmp.u32[2] < ctr_scrambler.u32[2]);
	keyTmp.u32[0] += ctr_scrambler.u32[0] + (keyTmp.u32[1] < ctr_scrambler.u32[1]);

	// Rotate left by 87.
	// This is effectively "rotate left by 23" with adjusted DWORD indexes.
	keyNormal->u32[2] = cpu_to_be32((keyTmp.u32[0] << 23) | (keyTmp.u32[1] >> 9));
	keyNormal->u32[3] = cpu_to_be32((keyTmp.u32[1] << 23) | (keyTmp.u32[2] >> 9));
	keyNormal->u32[0] = cpu_to_be32((keyTmp.u32[2] << 23) | (keyTmp.u32[3] >> 9));
	keyNormal->u32[1] = cpu_to_be32((keyTmp.u32[3] << 23) | (keyTmp.u32[0] >> 9));

	// We're done here.
	return 0;
}

}

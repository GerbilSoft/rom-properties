/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XEX.hpp: Microsoft Xbox 360 executable reader.                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "librpbase/RomData.hpp"

namespace LibRomData {

class Xbox360_XEX_Private;
ROMDATA_DECL_BEGIN(Xbox360_XEX)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()

public:
	// Encryption key indexes.
	// NOTE: XEX2 debug key is all zeroes,
	// so it's not included here.
	enum class EncryptionKeys {
		Key_XEX1,	// aka Cardea
		Key_XEX2,

		Key_Max
	};

#ifdef ENABLE_DECRYPTION
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
#endif /* ENABLE_DECRYPTION */

ROMDATA_DECL_END()

}

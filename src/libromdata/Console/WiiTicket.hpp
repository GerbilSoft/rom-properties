/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiTicket.hpp: Nintendo Wii (and Wii U) ticket reader.                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once
#include "config.librpbase.h"

#include "librpbase/RomData.hpp"
#include "librpbase/crypto/KeyManager.hpp"

#include "wii_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiTicket)
ROMDATA_DECL_METADATA()

public:
	/**
	 * Get the ticket format version.
	 * @return Ticket format version
	 */
	unsigned int ticketFormatVersion(void) const;

	/**
	 * Get the ticket. (v0)
	 *
	 * NOTE: The v1 ticket doesn't have any useful extra data,
	 * so we're only offering the v0 ticket.
	 *
	 * @return Ticket
	 */
	const RVL_Ticket *ticket_v0(void) const;

public:
#ifdef ENABLE_DECRYPTION
	/**
	 * Get the decrypted title key.
	 * The title ID is used as the IV.
	 *
	 * @param pKeyBuf	[out] Pointer to key buffer
	 * @param size_t	[in] Size of pKeyBuf (must be 16)
	 * @return 0 on success; negative POSIX error code on error. (Check verifyResult() for key verification errors.)
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	int decryptTitleKey(uint8_t *pKeyBuf, size_t size);
#endif /* ENABLE_DECRYPTION */

	/**
	 * Encryption key verification result.
	 * Call this function after calling decryptTitleKey().
	 * @return Encryption key verification result.
	 */
	LibRpBase::KeyManager::VerifyResult verifyResult(void) const;

public:
	// Encryption key indexes
	enum class EncryptionKeys : int8_t {
		Unknown = -2,
		None = -1,	// No encryption (RVT-H Reader)

		// Retail
		Key_RVL_Common = 0,
		Key_RVL_Korean,
		Key_WUP_Starbuck_vWii_Common,

		// Debug
		Key_RVT_Debug,
		Key_RVT_Korean,
		Key_CAT_Starbuck_vWii_Common,

		// SD card (TODO: Retail vs. Debug?)
		Key_RVL_SD_AES,
		Key_RVL_SD_IV,
		Key_RVL_SD_MD5,

		// Wii U mode keys
		Key_WUP_Starbuck_WiiU_Common,
		Key_CAT_Starbuck_WiiU_Common,

		// iQue NetCard
		Key_NC_Retail,
		Key_NC_Debug,

		Max
	};

	/**
	 * Encryption key in use.
	 * @return Encryption key in use.
	 */
	EncryptionKeys encKey(void) const;

	/**
	 * Get a user-friendly name for the specified encryption key.
	 * NOTE: EncryptionKeys::Unknown will return nullptr.
	 *
	 * @param encKey Encryption key index
	 * @return User-friendly name, or nullptr if not found.
	 */
	static const char *encKeyName_static(EncryptionKeys encKey);

	/**
	 * Get a user-friendly name for this ticket's encryption key.
	 * NOTE: EncryptionKeys::Unknown will return nullptr.
	 *
	 * @return User-friendly name, or nullptr if not found.
	 */
	const char *encKeyName(void) const;

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
	static const char* encryptionKeyName_static(int keyIdx);

	/**
	 * Get the verification data for a given encryption key index.
	 * @param keyIdx Encryption key index.
	 * @return Verification data. (16 bytes)
	 */
	static const uint8_t* encryptionVerifyData_static(int keyIdx);
#endif /* ENABLE_DECRYPTION */

ROMDATA_DECL_END()

}

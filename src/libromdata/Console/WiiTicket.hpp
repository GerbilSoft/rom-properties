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

#ifdef ENABLE_DECRYPTION
	/**
	 * Get the decrypted title key.
	 * @param pKeyBuf	[out] Pointer to key buffer
	 * @param size		[in] Size of pKeyBuf (must be >= 16)
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int decryptTitleKey(uint8_t *pKeyBuf, size_t size) const;
#endif /* ENABLE_DECRYPTION */

ROMDATA_DECL_END()

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD_p.hpp: Nintendo Wii WAD file reader. (Private class)             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "stdafx.h"
#include "librpbase/config.librpbase.h"

#include "common.h"

// Other rom-properties libraries
#include "librpbase/RomData_p.hpp"
#include "librpbase/crypto/KeyManager.hpp"
#include "librpfile/IRpFile.hpp"

// Wii structs
#include "gcn_structs.h"
#include "wii_structs.h"
#include "wii_wad.h"
#include "wii_banner.h"

#ifdef ENABLE_DECRYPTION
#  include "librpbase/disc/CBCReader.hpp"
#endif /* ENABLE_DECRYPTION */

// WiiTicket for EncryptionKeys
// TODO: Use for title key decryption.
#include "../Console/WiiTicket.hpp"

// Uninitialized vector class
#include "uvector.h"

namespace LibRpBase {
#ifdef ENABLE_DECRYPTION
	class RomData;
#endif /* ENABLE_DECRYPTION */
}

namespace LibRomData {

class WiiWADPrivate final : public LibRpBase::RomDataPrivate
{
public:
	WiiWADPrivate(const LibRpFile::IRpFilePtr &file);
	~WiiWADPrivate() final = default;

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WiiWADPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const LibRpBase::RomDataInfo romDataInfo;

public:
	// WAD type
	enum class WadType {
		Unknown	= -1,	// Unknown WAD type.

		WAD	= 0,	// Standard WAD
		BWF	= 1,	// BroadOn WAD

		Max
	};
	WadType wadType;

	// WAD structs
	union {
		Wii_WAD_Header wad;
		Wii_BWF_Header bwf;
	} wadHeader;
	RVL_Ticket ticket;
	RVL_TMD_Header tmdHeader;

	// Data offset and size
	uint32_t data_offset;
	uint32_t data_size;

	// Name (BroadOn WADs only)
	// FIXME: This is the same "meta" section as Nintendo WADs...
	std::string wadName;

	// TMD contents table
	rp::uvector<RVL_Content_Entry> tmdContentsTbl;
	const RVL_Content_Entry *pIMETContent;
	uint32_t imetContentOffset;	// relative to start of data area

	/**
	 * Round a value to the next highest multiple of 64.
	 * @param value Value.
	 * @return Next highest multiple of 64.
	 */
	template<typename T>
	static inline T toNext64(T val)
	{
		return (val + (T)63) & ~((T)63);
	}

#ifdef ENABLE_DECRYPTION
	// CBC reader for the main data area
	LibRpBase::CBCReaderPtr cbcReader;
	LibRpBase::RomDataPtr mainContent;	// WiiWIBN or NintendoDS

	// Decrypted title key
	uint8_t dec_title_key[16];

	// Main data headers
	Wii_IMET_t imet;	// NOTE: May be WIBN.
#endif /* ENABLE_DECRYPTION */

	// Key index
	WiiTicket::EncryptionKeys key_idx;
	// Key status
	LibRpBase::KeyManager::VerifyResult key_status;

	/**
	 * Get the game information string from the banner.
	 * @return Game information string, or empty string on error.
	 */
	std::string getGameInfo(void);

#ifdef ENABLE_DECRYPTION
	/**
	 * Open the SRL if it isn't already opened.
	 * This operation only works for DSi TAD packages.
	 * @return 0 on success; non-zero on error.
	 */
	int openSRL(void);
#endif /* ENABLE_DECRYPTION */
};

}

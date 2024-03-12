/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiTMD.hpp: Nintendo Wii (and Wii U) title metadata reader.             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

// Uninitialized vector class
#include "uvector.h"

#include "wii_structs.h"
#include "wiiu_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiTMD)
ROMDATA_DECL_METADATA()

public:
	/** TMD accessors **/

	/**
	 * Get the TMD format version.
	 * @return TMD format version
	 */
	unsigned int tmdFormatVersion(void) const;

	/**
	 * Get the TMD header.
	 * @return TMD header
	 */
	const RVL_TMD_Header *tmdHeader(void) const;

	/**
	 * Get the number of content metadata groups. (for TMD v1)
	 * @return Number of content metadata groups, or 0 on error.
	 */
	unsigned int cmdGroupCountV1(void);

	/**
	 * Get the contents table. (for TMD v1)
	 * @param grpIdx CMD group index
	 * @return Contents table, or empty rp::uvector<> on error.
	 */
	rp::uvector<WUP_Content_Entry> contentsTableV1(unsigned int grpIdx);

ROMDATA_DECL_END()

}

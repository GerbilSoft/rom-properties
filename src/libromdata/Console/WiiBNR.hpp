/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiBNR.hpp: Nintendo Wii banner reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiBNR)

public:
	/**
	 * Read a Nintendo Wii banner file.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file		[in] Open banner file
	 * @param gcnRegion	[in] GameCube region code from the boot block
	 * @param id4_region	[in] ID4 region
	 */
	explicit WiiBNR(const LibRpFile::IRpFilePtr &file, uint32_t gcnRegion, char id4_region);

private:
	/**
	 * Common initialization function for the constructors.
	 */
	void init(void);

ROMDATA_DECL_METADATA()
ROMDATA_DECL_END()

}

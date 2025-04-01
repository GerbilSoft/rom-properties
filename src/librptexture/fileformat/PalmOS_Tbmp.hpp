/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PalmOS_Tbmp.cpp: Palm OS Tbmp texture reader.                           *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN_NO_CTOR(PalmOS_Tbmp)

public:
	/**
	 * Read a Palm OS Tbmp image file.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open Palm OS Tbmp image file
	 */
	PalmOS_Tbmp(const LibRpFile::IRpFilePtr &file)
		: PalmOS_Tbmp(file, 0)
	{}

	/**
	 * Read a Palm OS Tbmp image file.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open Palm OS Tbmp image file
	 * @param bitmapTypeAddr Starting address of the BitmapType header in the file.
	 */
	PalmOS_Tbmp(const LibRpFile::IRpFilePtr &file, uint32_t bitmapTypeAddr);

public:
	/**
	 * Get the address of the next bitmap. (for tAIB resources)
	 * @return Address of the next bitmap, or 0 if none.
	 */
	uint32_t getNextTbmpAddress(void) const;

FILEFORMAT_DECL_COMMON_FNS()
FILEFORMAT_DECL_END()

} // namespace LibRpTexture

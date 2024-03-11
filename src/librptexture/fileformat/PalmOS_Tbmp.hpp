/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PalmOS_Tbmp.cpp: Palm OS Tbmp texture reader.                           *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(PalmOS_Tbmp)

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
	 * @param file Open file.
	 * @param bitmapTypeAddr Starting address of the BitmapType header in the file.
	 */
	PalmOS_Tbmp(const std::shared_ptr<LibRpFile::IRpFile> &file, off64_t bitmapTypeAddr);

private:
	void init(void);

public:
	/**
	 * Get the address of the next bitmap. (for tAIB resources)
	 * @return Address of the next bitmap, or 0 if none.
	 */
	uint32_t getNextTbmpAddress(void) const;

FILEFORMAT_DECL_END()

}

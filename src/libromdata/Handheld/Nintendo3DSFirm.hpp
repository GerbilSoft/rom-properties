/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DSFirm.hpp: Nintendo 3DS firmware reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN_NO_CTOR(Nintendo3DSFirm)

public:
	enum class StorageType {
		Unknown = -1,

		NAND = 0,	// Standard NAND boot; not encrypted
		NTR = 1,	// NTRBOOT; encrypted
		SPI = 2,	// SPIBOOT; encrypted
	};

	/**
	 * Read a Nintendo 3DS firmware binary.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open ROM image
	 */
	explicit Nintendo3DSFirm(const LibRpFile::IRpFilePtr &file)
		: Nintendo3DSFirm(file, StorageType::Unknown)
	{}

	/**
	 * Read a Nintendo 3DS firmware binary.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open ROM image
	 * @param type Storage type; used to determine if decryption is needed.
	 */
	explicit Nintendo3DSFirm(const LibRpFile::IRpFilePtr &file, StorageType type);

ROMDATA_DECL_COMMON_FNS()
ROMDATA_DECL_END()

} // namespace LibRomData

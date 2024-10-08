/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationEXE.hpp: PlayStation PS-X executable reader.                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN_NO_CTOR(PlayStationEXE)

public:
	/**
	 * Read a PlayStation PS-X executable file.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the disc image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open PS-X executable file
	 */
	PlayStationEXE(const LibRpFile::IRpFilePtr &file)
		: PlayStationEXE(file, 0)
	{}

	/**
	 * Read a PlayStation PS-X executable file.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the disc image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open PS-X executable file
	 * @param sp_override Stack pointer override  (0 for none)
	 */
	PlayStationEXE(const LibRpFile::IRpFilePtr &file, uint32_t sp_override);

ROMDATA_DECL_COMMON_FNS()
ROMDATA_DECL_END()

} // namespace LibRomData

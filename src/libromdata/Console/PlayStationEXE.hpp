/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PlayStationEXE.hpp: PlayStation PS-X executable reader.                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(PlayStationEXE)

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
		 * @param file Open PS-X executable file.
		 * @param sp_override Stack pointer override.
		 */
		PlayStationEXE(const LibRpFile::IRpFilePtr &file, uint32_t sp_override);

	private:
		/**
		 * Common initialization function for the constructors.
		 */
		void init(void);

ROMDATA_DECL_END()

}

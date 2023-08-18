/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameCubeBNR.hpp: Nintendo GameCube banner reader.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(GameCubeBNR)

	public:
		/**
		 * Read a Nintendo GameCube banner file.
		 *
		 * A ROM image must be opened by the caller. The file handle
		 * will be ref()'d and must be kept open in order to load
		 * data from the ROM image.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * @param file Open banner file
		 * @param gcnRegion GameCube region for BNR1 encoding
		 */
		explicit GameCubeBNR(const std::shared_ptr<LibRpFile::IRpFile> &file, uint32_t gcnRegion);

	private:
		/**
		 * Common initialization function for the constructors.
		 */
		void init(void);

ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

	public:
		/** GameCubeBNR accessors. **/

		/**
		 * Add a field for the GameCube banner.
		 *
		 * This adds an RFT_STRING field for BNR1, and
		 * RFT_STRING_MULTI for BNR2.
		 *
		 * @param fields RomFields*
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int addField_gameInfo(LibRpBase::RomFields *fields) const;

ROMDATA_DECL_END()

}

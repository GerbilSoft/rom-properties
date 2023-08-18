/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(NintendoDS)

	public:
		/**
		 * Read a Nintendo DS ROM image.
		 *
		 * A ROM image must be opened by the caller. The file handle
		 * will be ref()'d and must be kept open in order to load
		 * data from the ROM image.
		 *
		 * To close the file, either delete this object or call close().
		 *
		 * NOTE: Check isValid() to determine if this is a valid ROM.
		 *
		 * @param file Open ROM image.
		 * @param cia If true, hide fields that aren't relevant to DSiWare in 3DS CIA packages.
		 */
		explicit NintendoDS(const std::shared_ptr<LibRpFile::IRpFile> &file, bool cia);

	private:
		/**
		 * Common initialization function for the constructors.
		 */
		void init(void);

ROMDATA_DECL_DANGEROUS()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_ROMOPS();
ROMDATA_DECL_END()

}

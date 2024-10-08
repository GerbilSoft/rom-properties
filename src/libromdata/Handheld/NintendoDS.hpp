/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN_NO_CTOR(NintendoDS)

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
	 * @param file Open ROM image
	 */
	explicit NintendoDS(const LibRpFile::IRpFilePtr &file)
		: NintendoDS(file, false)
	{}

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
	 * @param file Open ROM image
	 * @param cia If true, hide fields that aren't relevant to DSiWare in 3DS CIA packages.
	 */
	explicit NintendoDS(const LibRpFile::IRpFilePtr &file, bool cia);

ROMDATA_DECL_COMMON_FNS()
ROMDATA_DECL_DANGEROUS()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_IMGEXT()
ROMDATA_DECL_ROMOPS();
ROMDATA_DECL_END()

} // namespace LibRomData

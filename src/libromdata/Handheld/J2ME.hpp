/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * J2ME.hpp: Java 2 Micro Edition package reader.                          *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "compat/unzip.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN_NO_CTOR(J2ME)

public:
	/**
	 * Read a J2ME .jar or .jad file.
	 *
	 * A ROM file must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open ROM file.
	 */
	J2ME(const LibRpFile::IRpFilePtr &file)
		: J2ME(file, nullptr)
	{}

	/**
	 * Read a J2ME .jar or .jad file.
	 *
	 * A ROM file must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open ROM file.
	 * @param jarFile .jar file opened with MiniZip. (this object takes ownership)
	 */
	J2ME(const LibRpFile::IRpFilePtr &file, unzFile jarFile);

ROMDATA_DECL_COMMON_FNS()
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_END()

}

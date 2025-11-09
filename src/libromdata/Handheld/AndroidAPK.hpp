/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * AndroidAPK.hpp: Android APK package reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "compat/unzip.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN_NO_CTOR(AndroidAPK)

public:
	/**
	 * Read an AndroidAPK .apk file.
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
	explicit AndroidAPK(const LibRpFile::IRpFilePtr &file)
		: AndroidAPK(file, nullptr)
	{}

	/**
	 * Read an AndroidAPK .apk file.
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
	 * @param apkFile .apk file opened with MiniZip. (this object takes ownership)
	 */
	explicit AndroidAPK(const LibRpFile::IRpFilePtr &file, unzFile apkFile);

ROMDATA_DECL_COMMON_FNS()
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_DANGEROUS()
ROMDATA_DECL_END()

}

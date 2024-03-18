/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage.hpp: Wii U NUS Package reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(WiiUPackage)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGINT()

public:
	/**
	 * Read a Wii U NUS package.
	 *
	 * NOTE: Wii U NUS packages are directories. This constructor
	 * takes a local directory path.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param path Local directory path (UTF-8)
	 */
	WiiUPackage(const char *path);

#ifdef _WIN32
	/**
	 * Read a Wii U NUS package.
	 *
	 * NOTE: Wii U NUS packages are directories. This constructor
	 * takes a local directory path.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param path Local directory path (UTF-16)
	 */
	WiiUPackage(const wchar_t *path);
#endif /* _WIN32 */

private:
	/**
	 * Internal initialization function for the two constructors.
	 */
	void init(void);

public:
	/**
	 * Is a directory supported by this class?
	 * @param path Directory to check
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static int isDirSupported_static(const char *path);

#ifdef _WIN32
	/**
	 * Is a directory supported by this class?
	 * @param path Directory to check
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static int isDirSupported_static(const wchar_t *path);
#endif /* _WIN32 */

ROMDATA_DECL_END()

}

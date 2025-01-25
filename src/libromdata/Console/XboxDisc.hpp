/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * XboxDisc.hpp: Microsoft Xbox disc image parser.                         *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "../iso_structs.h"

namespace LibRomData {

ROMDATA_DECL_BEGIN(XboxDisc)
ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_VIEWED_ACHIEVEMENTS()

public:
	/**
	 * Read an extracted Microsoft Xbox disc file system.
	 *
	 * NOTE: Extracted Xbox disc file systems are directories.
	 * This constructor takes a local directory path.
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
	XboxDisc(const char *path);

#if defined(_WIN32) && defined(_UNICODE)
	/**
	 * Read an extracted Microsoft Xbox disc file system.
	 *
	 * NOTE: Extracted Xbox disc file systems are directories.
	 * This constructor takes a local directory path.
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
	XboxDisc(const wchar_t *path);
#endif /* _WIN32 && _UNICODE */

private:
	/**
	 * Internal initialization function for the three constructors.
	 */
	void init();

public:
	/**
	 * Is a ROM image supported by this class?
	 * @param pvd ISO-9660 Primary Volume Descriptor.
	 * @param pWave If non-zero, receives the wave number. (0 if none; non-zero otherwise.)
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static int isRomSupported_static(const ISO_Primary_Volume_Descriptor *pvd, uint8_t *pWave);

	/**
	 * Is a ROM image supported by this class?
	 * @param pvd ISO-9660 Primary Volume Descriptor.
	 * @return Class-specific system ID (>= 0) if supported; -1 if not.
	 */
	static inline int isRomSupported_static(const ISO_Primary_Volume_Descriptor *pvd)
	{
		return isRomSupported_static(pvd, nullptr);
	}

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

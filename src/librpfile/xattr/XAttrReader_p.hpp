/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_p.hpp: Extended Attribute reader (PRIVATE CLASS)            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Common macros
#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC
#include "tcharx.h"

// C includes (C++ namespace)
#include <cstdint>

// C++ includes
#include <list>
#include <string>
#include <utility>

namespace LibRpFile {

class XAttrReaderPrivate
{
public:
	explicit XAttrReaderPrivate(const char *filename);
#if defined(_WIN32) && defined(UNICODE)
	explicit XAttrReaderPrivate(const wchar_t *filename);
#endif /* _WIN32 && UNICODE */
#ifndef _WIN32
	~XAttrReaderPrivate();
#endif /* !_WIN32 */

private:
	RP_DISABLE_COPY(XAttrReaderPrivate)

public:
	/**
	 * Load Ext2 attributes, if available.
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadExt2Attrs(void);

	/**
	 * Load XFS attributes, if available.
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadXfsAttrs(void);

	/**
	 * Load MS-DOS attributes, if available.
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadDosAttrs(void);

#ifdef _WIN32
#  ifdef UNICODE
	/**
	 * Load generic xattrs, if available.
	 * (POSIX xattr on Linux; ADS on Windows)
	 * FindFirstStreamW() version; requires Windows Vista or later.
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadGenericXattrs_FindFirstStreamW(void);
#  endif /* UNICODE */

	/**
	 * Load generic xattrs, if available.
	 * (POSIX xattr on Linux; ADS on Windows)
	 * BackupRead() version.
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadGenericXattrs_BackupRead(void);
#endif /* _WIN32 */

	/**
	 * Load generic xattrs, if available.
	 * (POSIX xattr on Linux; ADS on Windows)
	 * Internal fd (filename on Windows) must be set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadGenericXattrs(void);

public:
#ifdef _WIN32
	// Windows: Need to store the filename.
	std::tstring filename;
#else /* !_WIN32 */
	// Other: Need to store the open fd.
	int fd;
#endif /* _WIN32 */

	int lastError;

	bool hasExt2Attributes;
	bool hasXfsAttributes;
	bool hasDosAttributes;
	bool hasGenericXAttrs;

	int ext2Attributes;
	uint32_t xfsXFlags;
	uint32_t xfsProjectId;
	unsigned int dosAttributes;
	unsigned int validDosAttributes;
	XAttrReader::XAttrList genericXAttrs;
};

} // namespace LibRpFile

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_dummy.cpp: Extended Attribute reader (dummy version)        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

// XAttrReader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
	unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
}

namespace LibRpFile {

/** XAttrReaderPrivate **/

XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
	: fd(-1)
	, lastError(0)
	, hasExt2Attributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, linuxAttributes(0)
	, dosAttributes(0)
{
	RP_UNUSED(filename);
}

/**
 * Initialize attributes.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::init(void)
{
	RP_UNUSED(fd);
	return -ENOTSUP;
}

/**
 * Load Ext2 attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadExt2Attrs(void)
{
	RP_UNUSED(fd);
	return -ENOTSUP;
}

/**
 * Load MS-DOS attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(void)
{
	RP_UNUSED(fd);
	return -ENOTSUP;
}

/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs(void)
{
	RP_UNUSED(fd);
	return -ENOTSUP;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader.cpp: Extended Attribute reader (common functions)           *
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
	extern unsigned char RP_LibRpFile_XAttrReader_ForceLinkage;
	unsigned char RP_LibRpFile_XAttrReader_ForceLinkage;
}

namespace LibRpFile {

/** XAttrReader **/

XAttrReader::XAttrReader(const char *filename)
	: d_ptr(new XAttrReaderPrivate(filename))
{ }

#if defined(_WIN32)
XAttrReader::XAttrReader(const wchar_t *filename)
	: d_ptr(new XAttrReaderPrivate(filename))
{ }
#endif /* _WIN32 */

XAttrReader::~XAttrReader()
{
	delete d_ptr;
}

/**
 * Get the last error number.
 * @return Last error number (POSIX error code)
 */
int XAttrReader::lastError(void) const
{
	RP_D(const XAttrReader);
	return d->lastError;
}

/**
 * Does this file have Ext2 attributes?
 * @return True if it does; false if not.
 */
bool XAttrReader::hasExt2Attributes(void) const
{
	RP_D(const XAttrReader);
	return d->hasExt2Attributes;
}

/**
 * Get this file's Ext2 attributes.
 * @return Ext2 attributes
 */
int XAttrReader::ext2Attributes(void) const
{
	RP_D(const XAttrReader);
	return d->ext2Attributes;
}

/**
 * Does this file have XFS attributes?
 * @return True if it does; false if not.
 */
bool XAttrReader::hasXfsAttributes(void) const
{
	RP_D(const XAttrReader);
	return d->hasExt2Attributes;
}

/**
 * Get this file's XFS xflags.
 * @return XFS xflags
 */
uint32_t XAttrReader::xfsXFlags(void) const
{
	RP_D(const XAttrReader);
	return d->xfsXFlags;
}

/**
 * Get this file's XFS project ID.
 * @return XFS project ID
 */
uint32_t XAttrReader::xfsProjectId(void) const
{
	RP_D(const XAttrReader);
	return d->xfsProjectId;
}

/**
 * Does this file have MS-DOS attributes?
 * @return True if it does; false if not.
 */
bool XAttrReader::hasDosAttributes(void) const
{
	RP_D(const XAttrReader);
	return d->hasDosAttributes;
}

/**
 * Get this file's MS-DOS attributes.
 * @return MS-DOS attributes.
 */
unsigned int XAttrReader::dosAttributes(void) const
{
	RP_D(const XAttrReader);
	return d->dosAttributes;
}

/**
 * Does this file have generic extended attributes?
 * (POSIX xattr on Linux; ADS on Windows)
 * @return True if it does; false if not.
 */
bool XAttrReader::hasGenericXAttrs(void) const
{
	RP_D(const XAttrReader);
	return d->hasGenericXAttrs;
}

/**
 * Get the list of extended attributes.
 * @return Extended attributes
 */
const XAttrReader::XAttrList &XAttrReader::genericXAttrs(void) const
{
	RP_D(const XAttrReader);
	return d->genericXAttrs;
}

}

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
	extern uint8_t RP_LibRpFile_XAttrReader_ForceLinkage;
	uint8_t RP_LibRpFile_XAttrReader_ForceLinkage;
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
 * Does this file have Linux attributes?
 * @return True if it does; false if not.
 */
bool XAttrReader::hasLinuxAttributes(void) const
{
	RP_D(const XAttrReader);
	return d->hasLinuxAttributes;
}

/**
 * Get this file's Linux attributes.
 * @return Linux attributes
 */
int XAttrReader::linuxAttributes(void) const
{
	RP_D(const XAttrReader);
	return d->linuxAttributes;
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

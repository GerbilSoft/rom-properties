/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader.hpp: Extended Attribute reader                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Common macros
#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
#include <stdint.h>

// C++ includes
#include <map>
#include <string>
#include <utility>

namespace LibRpFile {

class XAttrReaderPrivate;
class RP_LIBROMDATA_PUBLIC XAttrReader
{
public:
	explicit XAttrReader(const char *filename);
#if defined(_WIN32)
	explicit XAttrReader(const wchar_t *filename);
#endif /* _WIN32 */
public:
	~XAttrReader();

private:
	RP_DISABLE_COPY(XAttrReader)
protected:
	friend class XAttrReaderPrivate;
	XAttrReaderPrivate *const d_ptr;

public:
	/**
	 * Get the last error number.
	 * @return Last error number (POSIX error code)
	 */
	int lastError(void) const;

public:
	/**
	 * Does this file have Ext2 attributes?
	 * @return True if it does; false if not.
	 */
	bool hasExt2Attributes(void) const;

	/**
	 * Get this file's Ext2 attributes.
	 * @return Ext2 attributes
	 */
	int ext2Attributes(void) const;

	/**
	 * Does this file have XFS attributes?
	 * @return True if it does; false if not.
	 */
	bool hasXfsAttributes(void) const;

	/**
	 * Get this file's XFS xflags.
	 * @return XFS xflags
	 */
	uint32_t xfsXFlags(void) const;

	/**
	 * Get this file's XFS project ID.
	 * @return XFS project ID
	 */
	uint32_t xfsProjectId(void) const;

	/**
	 * Does this file have MS-DOS attributes?
	 * @return True if it does; false if not.
	 */
	bool hasDosAttributes(void) const;

	/**
	 * Can we write MS-DOS attributes to this file?
	 *
	 * NOTE: setDosAttributes() is a *static* function.
	 * This is merely used as an advisory for the GUI.
	 *
	 * @return True if we can; false if we can't.
	 */
	bool canWriteDosAttributes(void) const;

	/**
	 * Get this file's MS-DOS attributes.
	 * @return MS-DOS attributes.
	 */
	unsigned int dosAttributes(void) const;

	/**
	 * Does this file have generic extended attributes?
	 * (POSIX xattr on Linux; ADS on Windows)
	 * @return True if it does; false if not.
	 */
	bool hasGenericXAttrs(void) const;

	/**
	 * Extended attribute map (UTF-8)
	 * - Key: Name
	 * - Value: Value
	 */
	typedef std::map<std::string, std::string> XAttrList;

	/**
	 * Get the list of extended attributes.
	 * @return Extended attributes
	 */
	const XAttrList &genericXAttrs(void) const;

public:
	/** Attribute setters **/

	/**
	 * Set the MS-DOS attributes for the file.
	 *
	 * NOTE: Only the RHAS attributes will be written.
	 * Other attributes will be preserved.
	 *
	 * @param attrs MS-DOS attributes to set.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int setDosAttributes(uint32_t attrs);
};

}

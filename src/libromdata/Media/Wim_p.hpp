/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim_p.hpp: Microsoft WIM header reader (PRIVATE CLASS)                  *
 *                                                                         *
 * Copyright (c) 2023 by ecumber.                                          *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librpbase.h"
#include "wim_structs.h"

// librpfile
#include "librpfile/IRpFile.hpp"

#ifdef ENABLE_XML
// PugiXML
#  include <pugixml.hpp>
using namespace pugi;
#endif /* ENABLE_XML */

namespace LibRomData {

class WimPrivate final : public LibRpBase::RomDataPrivate
{
public:
	explicit WimPrivate(const LibRpFile::IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WimPrivate)

public:
	/** RomDataInfo **/
	static const std::array<const char*, 3+1> exts;
	static const std::array<const char*, 1+1> mimeTypes;
	static const LibRpBase::RomDataInfo romDataInfo;

public:
	// WIM header
	WIM_Header wimHeader;

	// WIM version
	// NOTE: WIMs pre-1.13 are being detected but won't
	// be read due to the format being different.
	WIM_Version_Type versionType;

#ifdef ENABLE_XML
public:
	/**
	 * Add fields from the WIM image's XML manifest.
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_XML();
#endif /* ENABLE_XML */
};

} // namespace LibRomData

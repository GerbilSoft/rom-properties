/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Xbox360_XDBF.hpp: Microsoft Xbox 360 game resource reader.              *
 * Handles XDBF files and sections.                                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"
#include "librpbase/RomMetaData.hpp"
#include <string>

namespace LibRomData {

class Xbox360_XDBF_Private;
ROMDATA_DECL_BEGIN(Xbox360_XDBF)
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()

public:
	/**
	 * Read an Xbox 360 XDBF file and/or section.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param file Open XDBF file and/or section.
	 * @param xex If true, hide fields that are displayed separately in XEX executables.
	 */
	explicit Xbox360_XDBF(const LibRpFile::IRpFilePtr &file, bool xex);

private:
	/**
	 * Common initialization function for the constructors.
	 */
	void init(void);

public:
	/** Special XDBF accessor functions **/

	/**
	 * Add the various XDBF string fields.
	 * @param fields RomFields*
	 * @return 0 on success; non-zero on error.
	 */
	int addFields_strings(LibRpBase::RomFields *fields) const;

	/**
	 * Get a particular string property for RomMetaData.
	 * @param property Property
	 * @return String, or empty string if not found.
	 */
	std::string getString(LibRpBase::Property property) const;

ROMDATA_DECL_END()

}

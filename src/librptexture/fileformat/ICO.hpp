/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ICO.hpp: Windows icon and cursor image reader.                          *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "FileFormat.hpp"

// IResourceReader for loading icons from .exe/.dll files.
#include "librpbase/disc/IResourceReader.hpp"

namespace LibRpTexture {

FILEFORMAT_DECL_BEGIN(ICO)

public:
	/**
	 * Read an icon or cursor from a Windows executable.
	 *
	 * A ROM image must be opened by the caller. The file handle
	 * will be ref()'d and must be kept open in order to load
	 * data from the ROM image.
	 *
	 * To close the file, either delete this object or call close().
	 *
	 * NOTE: Check isValid() to determine if this is a valid ROM.
	 *
	 * @param resReader	[in] IResourceReader
	 * @param type		[in] Resource type ID (RT_GROUP_ICON or RT_GROUP_CURSOR)
	 * @param id		[in] Resource ID (-1 for "first entry")
	 * @param lang		[in] Language ID (-1 for "first entry")
	 */
	explicit ICO(const LibRpBase::IResourceReaderPtr &resReader, uint16_t type, int id, int lang);

private:
	/**
	 * Common initialization function.
	 * @param res True if the icon is in a Windows executable; false if not.
	 */
	void init(bool res);

FILEFORMAT_DECL_END()

}

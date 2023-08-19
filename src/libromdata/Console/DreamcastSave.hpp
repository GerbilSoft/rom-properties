/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DreamcastSave.hpp: Sega Dreamcast save file reader.                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/RomData.hpp"

namespace LibRomData {

ROMDATA_DECL_BEGIN(DreamcastSave)

	public:
		/**
		 * Read a Sega Dreamcast save file. (.VMI+.VMS pair)
		 *
		 * This constructor requires two files:
		 * - .VMS file (main save file)
		 * - .VMI file (directory entry)
		 *
		 * Both files will be dup()'d.
		 * The .VMS file will be used as the main file for the RomData class.
		 *
		 * To close the files, either delete this object or call close().
		 * NOTE: Check isValid() to determine if this is a valid save file.
		 *
		 * @param vms_file Open .VMS save file.
		 * @param vmi_file Open .VMI save file.
		 */
		DreamcastSave(const LibRpFile::IRpFilePtr &vms_file,
		              const LibRpFile::IRpFilePtr &vmi_file);

ROMDATA_DECL_CLOSE()
ROMDATA_DECL_METADATA()
ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_END()

}

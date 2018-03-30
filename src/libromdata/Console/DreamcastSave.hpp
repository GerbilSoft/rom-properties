/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DreamcastSave.hpp: Sega Dreamcast save file reader.                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__

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
		DreamcastSave(LibRpBase::IRpFile *vms_file, LibRpBase::IRpFile *vmi_file);

ROMDATA_DECL_IMGSUPPORT()
ROMDATA_DECL_IMGPF()
ROMDATA_DECL_IMGINT()
ROMDATA_DECL_ICONANIM()
ROMDATA_DECL_END()

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DREAMCASTSAVE_HPP__ */

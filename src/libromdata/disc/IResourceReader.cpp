/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IResourceReader.cpp: Interface for Windows resource readers.            *
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

#include "IResourceReader.hpp"

#include "librpbase/file/IRpFile.hpp"
using LibRpBase::IRpFile;

namespace LibRomData {

/**
 * DWORD alignment function.
 * @param file	[in] File to DWORD align.
 * @return 0 on success; non-zero on error.
 */
int IResourceReader::alignFileDWORD(IRpFile *file)
{
	int ret = 0;
	int64_t pos = file->tell();
	if (pos % 4 != 0) {
		pos = ALIGN(4, pos);
		ret = file->seek(pos);
	}
	return ret;
}

}

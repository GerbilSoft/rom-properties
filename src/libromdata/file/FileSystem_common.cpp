/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * FileSystem_common.cpp: File system functions. (Common functions)        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "FileSystem.hpp"

namespace LibRomData { namespace FileSystem {

/**
 * Get the file extension from a filename or pathname.
 * @param filename Filename.
 * @return File extension, including the leading dot. (pointer to within the filename) [nullptr if no extension]
 */
const rp_char *file_ext(const rp_string &filename)
{
	size_t dotpos = filename.find_last_of(_RP_CHR('.'));
	size_t slashpos = filename.find_last_of(_RP_CHR(DIR_SEP_CHR));
	if (dotpos == rp_string::npos ||
	    dotpos >= filename.size()-1 ||
	    (slashpos != rp_string::npos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		return nullptr;
	}

	// Return the file extension. (pointer to within the filename)
	return &filename[dotpos];
}

} }

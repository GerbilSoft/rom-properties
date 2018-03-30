/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * FileSystem_common.cpp: File system functions. (Common functions)        *
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

#include "FileSystem.hpp"

// C includes.
#include <stddef.h>	/* size_t */

// C++ includes.
#include <string>
using std::string;

namespace LibRpBase { namespace FileSystem {

/**
 * Get the file extension from a filename or pathname.
 * @param filename Filename.
 * @return File extension, including the leading dot. (pointer to within the filename) [nullptr if no extension]
 */
const char *file_ext(const string &filename)
{
	size_t dotpos = filename.find_last_of('.');
	size_t slashpos = filename.find_last_of(DIR_SEP_CHR);
	if (dotpos == string::npos ||
	    dotpos >= filename.size()-1 ||
	    (slashpos != string::npos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		return nullptr;
	}

	// Return the file extension. (pointer to within the filename)
	return &filename[dotpos];
}

} }

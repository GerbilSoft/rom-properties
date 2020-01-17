/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * FileSystem_common.cpp: File system functions. (Common functions)        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FileSystem.hpp"

// C++ STL classes.
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

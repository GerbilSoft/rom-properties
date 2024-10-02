/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <cstdint>
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#include "tcharx.h"

// C++ STL classes
#include <forward_list>
#include <string>
#include <utility>

namespace LibRpFile { 

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, and "version.txt".
 *
 * @param path	[in] Path to scan.
 * @param rlist	[in/out] Return list for filenames and file types. (d_type)
 * @return 0 on success; non-zero on error.
 */
RP_LIBROMDATA_PUBLIC
int recursiveScan(const TCHAR *path, std::forward_list<std::pair<std::tstring, uint8_t> > &rlist);

}

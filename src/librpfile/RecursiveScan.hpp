/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <cstdint>
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#include "tcharx.h"

// C++ STL classes
#include <string>
#include <utility>

namespace LibRpFile { 

enum RecursiveScanError {
	RSE_OK				= 0,
	RSE_FOUND_UNEXPECTED_FILE_TYPE	= 1,
	RSE_FOUND_SYMBOLIC_LINK		= 2,
	RSE_FOUND_UNEXPECTED_FILE_EXT	= 3,
};

struct RecursiveScanState_t {
	unsigned int dirCount;	// Number of directories found
	unsigned int fileCount;	// Number of files found

	// Error counters are used when actually deleting files.
	unsigned int dirErrs;	// Number of errors encountered when removing directories
	unsigned int fileErrs;	// Number of errors encountered when deleting files

	RecursiveScanState_t()
		: dirCount(0)
		, fileCount(0)
		, dirErrs(0)
		, fileErrs(0)
	{}

	void reset(void)
	{
		dirCount = 0;
		fileCount = 0;
		dirErrs = 0;
		fileErrs = 0;
	}

	/**
	 * Get the total count of directories and files.
	 * @return Total count
	 */
	size_t totalCount(void) const
	{
		return dirCount + fileCount;
	}

	/**
	 * Is this state empty? (zero directories, zero files)
	 * @return True if it's empty.
	 */
	bool isEmpty(void) const
	{
		return (totalCount() == 0);
	}

	/**
	 * Have any errors occurred?
	 * @return True if any errors occurred.
	 */
	bool hasErrors(void) const
	{
		return (dirErrs + fileErrs) > 0;
	}
};

enum class RecursiveScanMode {
	ScanOnly,
	ScanAndDelete,
};

typedef void (*RecursiveScanCallback_t)(const RecursiveScanState_t &state, void *user_data);

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, and "version.txt".
 *
 * @param state		[in/out] Current scan state (use zero-init struct on first call)
 * @param path		[in] Path to scan
 * @param mode		[in] Scan mode
 * @param user_data	[in,opt] User data for the callback function
 * @param callback	[in,opt] Callback function (at the end to make it easier to use lambdas)
 * @return RecursiveScanRet_t
 */
RP_LIBROMDATA_PUBLIC
int recursiveScan(RecursiveScanState_t &state,
	const TCHAR *path, RecursiveScanMode mode,
	void *user_data, RecursiveScanCallback_t callback);

}

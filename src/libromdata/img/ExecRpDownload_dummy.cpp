/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ExecRpDownload_dummy.cpp: Execute rp-download.exe. (Dummy version)      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CacheManager.hpp"

// C++ includes.
#include <string>
using std::string;

namespace LibRomData {

/**
 * Execute rp-download. (Dummy version)
 * @param filteredCacheKey Filtered cache key.
 * @return 0 on success; negative POSIX error code on error.
 */
int CacheManager::execRpDownload(const string &filteredCacheKey)
{
#pragma message("*** WARNING: CacheManager::execRpDownload() is not implemented!")
	return -ENOSYS;
}

}

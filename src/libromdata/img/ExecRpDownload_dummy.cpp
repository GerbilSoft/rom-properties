/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ExecRpDownload_dummy.cpp: Execute rp-download.exe. (Dummy version)      *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "CacheManager.hpp"

// C includes (C++ namespace)
#include <cerrno>

// C++ includes
#include <string>
using std::string;

namespace LibRomData {

/**
 * Execute rp-download. (Dummy version)
 * @param filteredCacheKey Filtered cache key.
 * @return DownloadError::Success on success; other DownloadError on error.
 */
CacheManager::DownloadError CacheManager::execRpDownload(const string &filteredCacheKey)
{
#pragma message("*** WARNING: CacheManager::execRpDownload() is not implemented!")
	((void)filteredCacheKey);
	return DownloadError::ExecRpDownload_Not_Implemented;
}

} // namespace LibRomData

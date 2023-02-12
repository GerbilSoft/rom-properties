/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CacheManager.cpp: Local cache manager.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.libromdata.h"
#include "CacheManager.hpp"

// Other rom-properties libraries
#include "librpfile/RpFile.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpthreads/Semaphore.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpThreads::Semaphore;
using LibRpThreads::SemaphoreLocker;

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

// OS-specific includes
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  include "librptext/wchar.hpp"
#endif /* _WIN32 */

// C includes (C++ namespace)
#include <ctime>

// C++ STL classes
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibRomData {

// Semaphore used to limit the number of simultaneous downloads.
// TODO: Determine the best number of simultaneous downloads.
// TODO: Test this on XP with IEIFLAG_ASYNC.
Semaphore CacheManager::m_dlsem(2);

/** Proxy server functions. **/
// NOTE: This is only useful for downloaders that
// can't retrieve the system proxy server normally.

/**
 * Get the proxy server.
 * @return Proxy server URL.
 */
string CacheManager::proxyUrl(void) const
{
	return m_proxyUrl;
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
 */
void CacheManager::setProxyUrl(const char *proxyUrl)
{
	if (proxyUrl) {
		m_proxyUrl = proxyUrl;
	} else {
		m_proxyUrl.clear();
	}
}

/**
 * Download a file.
 *
 * @param cache_key Cache key
 *
 * The URL will be determined based on the cache key.
 *
 * If the file is present in the cache, the cached version
 * will be retrieved. Otherwise, the file will be downloaded.
 *
 * If the file was not found on the server, or it was not found
 * the last time it was requested, an empty string will be
 * returned, and a zero-byte file will be stored in the cache.
 *
 * @return Absolute path to the cached file.
 */
string CacheManager::download(const char *cache_key)
{
	// TODO: Only filter the cache key once.
	// Currently it's filtered twice:
	// - getCacheFilename() filters it
	// - We call filterCacheKey() before passing it to rp-download.

	// Check the main cache key.
	string cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// If the cache key begins with "sys/", then we have to
	// attempt to download the file, since it may be updated
	// with e.g. new version information.
	const bool check_newer = (!strncmp(cache_key, "sys/", 4));

	// Lock the semaphore to make sure we don't
	// download too many files at once.
	SemaphoreLocker locker(m_dlsem);

	if (!check_newer) {
		// Check if the file already exists.
		off64_t filesize = 0;
		time_t filemtime = 0;
		int ret = FileSystem::get_file_size_and_mtime(cache_filename, &filesize, &filemtime);
		if (ret == 0) {
			// Check if the file is 0 bytes.
			// TODO: How should we handle errors?
			if (filesize == 0) {
				// File is 0 bytes, which indicates it didn't exist
				// on the server. If the file is older than a week,
				// try to redownload it.
				// TODO: Configurable time.
				const time_t systime = time(nullptr);
				if ((systime - filemtime) < (86400*7)) {
					// Less than a week old.
					return string();
				}

				// More than a week old.
				// Delete the cache file and try to download it again.
				if (FileSystem::delete_file(cache_filename) != 0) {
					// Unable to delete the cache file.
					return string();
				}
			} else if (filesize > 0) {
				// File is larger than 0 bytes, which indicates
				// it was cached successfully.
				return cache_filename;
			}
		} else if (ret != -ENOENT) {
			// Some error other than "file not found" occurred.
			return string();
		}
	}

	// TODO: Add an option for "offline only".
	// Previously this was done by checking for a blank URL.
	// We don't have any offline-only databases right now, so
	// this has been temporarily removed.

	// Subdirectories will be created by rp-download to
	// ensure they keep the "low integrity" label on Win7.

	// Execute rp-download.
	// NOTE: Using the unfiltered cache key, since filtering it
	// results in slashes being changed to backslashes on Windows.
	// rp-download will filter the key itself.
	int ret = execRpDownload(cache_key);
	printf("ret == %d\n", ret);
	if (ret != 0) {
		// rp-download failed for some reason.
		return string();
	}

	// rp-download has successfully downloaded the file.
	return cache_filename;
}

/**
 * Check if a file has already been cached.
 * @param cache_key Cache key
 * @return Filename in the cache, or empty string if not found.
 */
string CacheManager::findInCache(const char *cache_key)
{
	// Get the cache key filename.
	string cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// Return the filename if the file exists.
	if (FileSystem::access(cache_filename.c_str(), R_OK) != 0) {
		// Unable to read the cache file.
		cache_filename.clear();
	}
	return cache_filename;
}

}

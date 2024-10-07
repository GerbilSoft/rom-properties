/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CacheManager.hpp: Local cache manager.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"

// librpthreads
namespace LibRpThreads {
	class Semaphore;
}

// C++ includes.
#include <string>

namespace LibRomData {

class CacheManager
{
public:
	CacheManager() = default;
	~CacheManager() = default;

private:
	RP_DISABLE_COPY(CacheManager)

public:
	/** Proxy server functions. **/
	// NOTE: This is only useful for downloaders that
	// can't retrieve the system proxy server normally.

	/**
	 * Get the proxy server.
	 * @return Proxy server URL.
	 */
	std::string proxyUrl(void) const;

	/**
	 * Set the proxy server.
	 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
	 */
	RP_LIBROMDATA_PUBLIC
	void setProxyUrl(const char *proxyUrl);

	/**
	 * Set the proxy server.
	 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
	 */
	inline void setProxyUrl(const std::string &proxyUrl)
	{
		setProxyUrl(proxyUrl.c_str());
	}

public:
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
	RP_LIBROMDATA_PUBLIC
	std::string download(const char *cache_key);

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
	std::string download(const std::string &cache_key)
	{
		return download(cache_key.c_str());
	}

	/**
	 * Check if a file has already been cached.
	 * @param cache_key Cache key
	 * @return Filename in the cache, or empty string if not found.
	 */
	RP_LIBROMDATA_PUBLIC
	std::string findInCache(const char *cache_key);

	/**
	 * Check if a file has already been cached.
	 * @param cache_key Cache key
	 * @return Filename in the cache, or empty string if not found.
	 */
	std::string findInCache(const std::string &cache_key)
	{
		return findInCache(cache_key.c_str());
	}

protected:
	/**
	 * Execute rp-download.
	 * @param filtered_cache_key Filtered cache key.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int execRpDownload(const std::string &filteredCacheKey);

protected:
	std::string m_proxyUrl;

	// Semaphore used to limit the number of simultaneous downloads.
	static LibRpThreads::Semaphore m_dlsem;
};

}

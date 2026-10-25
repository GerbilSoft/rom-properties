/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CacheManager.hpp: Local cache manager.                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"

// C++ includes.
#include <string>

namespace LibRomData {

class Semaphore;

class CacheManager
{
public:
	CacheManager() = default;
	~CacheManager() = default;

public:
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
	 * download() error codes.
	 * Maps to RpCreateThumbnailError.
	 */
	enum class DownloadError {
		Success = 0,

		// External image caching
		Cache_CannotGetFilename			= 22,	// LibCacheCommon::getCacheFilename() failed.
		Cache_NegativeCacheUnexpired		= 23,	// File has a negative cache entry that has not expired yet.
		Cache_CannotDeleteNegativeCacheEntry	= 24,	// Could not delete an expired negative cache entry.
		Cache_CannotReadCachedFile		= 25,	// Cannot read a file in the cache.
		Cache_UnknownCacheEntryError		= 26,	// Unknown cache entry error...

		// `rp-download` for external image downloads
		RpDownload_missing			= 30,	// `rp-download` executable is missing?
		RpDownload_failed			= 31,	// `rp-download` executable failed to start for some reason.
		RpDownload_hung				= 32,	// `rp-download` never completed...
		ExtFileNotFound				= 33,	// External URL returned 404.
		ExtOtherError				= 34,	// External URL returned a different error.
		ExtFileNotInCache			= 35,	// External URL is not in the local cache, and a cache-only search was requested.

		InternalError				= 126,
	};

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
	 * @param out_filename	[out] Returned filename (empty on error)
	 * @param cache_key	[in] Cache key
	 * @return DownloadError::Success on success; other DownloadError on error.
	 */
	RP_LIBROMDATA_PUBLIC
	DownloadError download(std::string &out_filename, const char *cache_key);

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
	 * @param out_filename	[out] Filename in the cache (empty on error)
	 * @param cache_key	[in] Cache key
	 * @return DownloadError::Success on success; other DownloadError on error.
	 */
	DownloadError download(std::string &out_filename, const std::string &cache_key)
	{
		return download(out_filename, cache_key.c_str());
	}

	/**
	 * Check if a file has already been cached.
	 * @param out_filename	[out] Filename in the cache (empty on error)
	 * @param cache_key	[in] Cache key
	 * @return DownloadError::Success on success; other DownloadError on error.
	 */
	RP_LIBROMDATA_PUBLIC
	DownloadError findInCache(std::string &out_filename, const char *cache_key);

	/**
	 * Check if a file has already been cached.
	 * @param out_filename	[out] Filename in the cache (empty on error)
	 * @param cache_key	[in] Cache key
	 * @return DownloadError::Success on success; other DownloadError on error.
	 */
	DownloadError findInCache(std::string &out_filename, const std::string &cache_key)
	{
		return findInCache(out_filename, cache_key.c_str());
	}

protected:
	/**
	 * Execute rp-download.
	 * @param filtered_cache_key Filtered cache key.
	 * @return DownloadError::Success on success; other DownloadError on error.
	 */
	DownloadError execRpDownload(const std::string &filteredCacheKey);

protected:
	std::string m_proxyUrl;

	// Semaphore used to limit the number of simultaneous downloads.
	static Semaphore m_dlsem;
};

} // namespace LibRomData

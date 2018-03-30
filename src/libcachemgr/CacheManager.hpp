/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * CacheManager.hpp: Local cache manager.                                  *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__

// librpbase
#include "librpbase/common.h"
#include "librpbase/threads/Semaphore.hpp"

// C++ includes.
#include <string>

namespace LibCacheMgr {

class IDownloader;
class CacheManager
{
	public:
		CacheManager();
		~CacheManager();

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
		void setProxyUrl(const char *proxyUrl);

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
		 */
		void setProxyUrl(const std::string &proxyUrl);

	protected:
		/**
		 * Get a cache filename.
		 * @param cache_key Cache key. (Will be filtered using filterCacheKey().)
		 * @return Cache filename, or empty string on error.
		 */
		std::string getCacheFilename(const std::string &cache_key);

	public:
		/**
		 * Filter invalid characters from a cache key.
		 * @param cache_key Cache key.
		 * @return Filtered cache key.
		 */
		static std::string filterCacheKey(const std::string &cache_key);

	public:
		/**
		 * Download a file.
		 *
		 * @param url URL.
		 * @param cache_key Cache key.
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
		std::string download(
			const std::string &url,
			const std::string &cache_key);

		/**
		 * Check if a file has already been cached.
		 * @param cache_key Cache key.
		 * @return Filename in the cache, or empty string if not found.
		 */
		std::string findInCache(const std::string &cache_key);

	protected:
		std::string m_proxyUrl;
		IDownloader *m_downloader;

		// Semaphore used to limit the number of simultaneous downloads.
		static LibRpBase::Semaphore m_dlsem;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * CacheManager.hpp: Local cache manager.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__

#include "libromdata/config.libromdata.h"
#include "libromdata/common.h"
#include "libromdata/threads/Semaphore.hpp"

namespace LibRomData {
	class RomData;
}

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
		LibRomData::rp_string proxyUrl(void) const;

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
		 */
		void setProxyUrl(const rp_char *proxyUrl);

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
		 */
		void setProxyUrl(const LibRomData::rp_string &proxyUrl);

	protected:
		/**
		 * Get a cache filename.
		 * @param cache_key Cache key.
		 * @return Cache filename, or empty string on error.
		 */
		LibRomData::rp_string getCacheFilename(const LibRomData::rp_string &cache_key);

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
		LibRomData::rp_string download(
			const LibRomData::rp_string &url,
			const LibRomData::rp_string &cache_key);

	protected:
		LibRomData::rp_string m_proxyUrl;
		IDownloader *m_downloader;

		// Semaphore used to limit the number of simultaneous downloads.
		static LibRomData::Semaphore m_dlsem;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__ */

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

namespace LibCacheMgr {

class IDownloader;
class CacheManager
{
	public:
		CacheManager();
		~CacheManager();

	private:
		CacheManager(const CacheManager &);
		CacheManager &operator=(const CacheManager &);

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
		 * Get the ROM Properties cache directory.
		 * @return Cache directory.
		 */
		const LibRomData::rp_string &cacheDir(void);

		/**
		 * Recursively mkdir() subdirectories.
		 *
		 * The last element in the path will be ignored, so if
		 * the entire pathname is a directory, a trailing slash
		 * must be included.
		 *
		 * @param path Path to recursively mkdir. (last component is ignored)
		 * @return 0 on success; non-zero on error.
		 */
		static int rmkdir(const LibRomData::rp_string &path);

		/**
		 * Does a file exist? [wrapper function]
		 * @param pathname Pathname.
		 * @param mode Mode.
		 * @return 0 if the file exists with the specified mode; non-zero if not.
		 */
		static int access(const LibRomData::rp_string &pathname, int mode);

		/**
		 * Get a file's size.
		 * @param filename Filename.
		 * @return Size on success; -1 on error.
		 */
		static int64_t filesize(const LibRomData::rp_string &filename);

	public:
		/**
		 * Download a file.
		 * @param url URL.
		 * @param cacheKey Cache key.
		 *
		 * If the file is present in the cache, the cached version
		 * will be retrieved. Otherwise, the file will be downloaded.
		 *
		 * If the file was not found on the server, or it was not found
		 * the last time it was requested, an empty string will be
		 * returned, and a zero-byte file will be stored in the cache.
		 *
		 * @return Absolute path to cached file.
		 */
		LibRomData::rp_string download(const LibRomData::rp_string &url, const LibRomData::rp_string &cacheKey);

	protected:
		LibRomData::rp_string m_proxyUrl;
		LibRomData::rp_string m_cacheDir;

		IDownloader *m_downloader;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHEMANAGER_HPP__ */

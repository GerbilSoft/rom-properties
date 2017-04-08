/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * CacheManager.cpp: Local cache manager.                                  *
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

#ifdef _WIN32
#include "stdafx.h"
#endif
#include "CacheManager.hpp"

#include "libromdata/TextFuncs.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/file/FileSystem.hpp"
using namespace LibRomData;
using namespace LibRomData::FileSystem;

// Windows includes.
#ifdef _WIN32
#include "libromdata/RpWin32.hpp"
#endif /* _WIN32 */

// gettimeofday()
#ifdef _MSC_VER
#include <time.h>
#else
#include <sys/time.h>
#endif

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::string;

// TODO: DownloaderFactory?
#ifdef _WIN32
#include "UrlmonDownloader.hpp"
#else
#include "CurlDownloader.hpp"
#endif

namespace LibCacheMgr {

// Semaphore used to limit the number of simultaneous downloads.
// TODO: Determine the best number of simultaneous downloads.
// TODO: Test this on XP with IEIFLAG_ASYNC.
Semaphore CacheManager::m_dlsem(2);

CacheManager::CacheManager()
{
	// TODO: DownloaderFactory?
#ifdef _WIN32
	m_downloader = new UrlmonDownloader();
#else
	m_downloader = new CurlDownloader();
#endif

	// TODO: Configure this somewhere?
	m_downloader->setMaxSize(4*1024*1024);
}

CacheManager::~CacheManager()
{
	delete m_downloader;
}

/** Proxy server functions. **/
// NOTE: This is only useful for downloaders that
// can't retrieve the system proxy server normally.

/**
 * Get the proxy server.
 * @return Proxy server URL.
 */
rp_string CacheManager::proxyUrl(void) const
{
	return m_proxyUrl;
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
 */
void CacheManager::setProxyUrl(const rp_char *proxyUrl)
{
	if (proxyUrl) {
		m_proxyUrl = proxyUrl;
	} else {
		m_proxyUrl.clear();
	}
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
 */
void CacheManager::setProxyUrl(const rp_string &proxyUrl)
{
	m_proxyUrl = proxyUrl;
}

/**
 * Get a cache filename.
 * @param cache_key Cache key.
 * @return Cache filename, or empty string on error.
 */
rp_string CacheManager::getCacheFilename(const rp_string &cache_key)
{
	// Get the cache filename.
	// This is the cache directory plus the cache key.
	rp_string cache_filename = getCacheDirectory();

	if (cache_filename.empty())
		return rp_string();
	if (cache_filename.at(cache_filename.size()-1) != DIR_SEP_CHR)
		cache_filename += DIR_SEP_CHR;

	// The cache key uses slashes. This must be converted
	// to backslashes on Windows.
#ifdef _WIN32
	cache_filename.reserve(cache_filename.size() + cache_key.size());
	for (size_t i = 0; i < cache_key.size(); i++) {
		if (cache_key[i] == '/') {
			cache_filename += DIR_SEP_CHR;
		} else {
			cache_filename += cache_key[i];
		}
	}
#else
	// Use slashes in the cache key.
	cache_filename += cache_key;
#endif

	// NOTE: The filename portion MUST be kept in cache_filename,
	// since the last component is ignored by rmkdir().
	if (rmkdir(cache_filename) != 0) {
		// Error creating subdirectories.
		return rp_string();
	}

	// Cache filename created.
	return cache_filename;
}

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
rp_string CacheManager::download(
	const rp_string &url,
	const rp_string &cache_key)
{
	rp_string filter_cache_key = cache_key;
	bool foundSlash = true;
	int dotCount = 0;
	for (int i = (int)filter_cache_key.size()-1; i >= 0; i--) {
		// Don't allow control characters,
		// invalid FAT32 characters, or dots.
		// (NOTE: '/' and '.' are allowed for extensions and cache hierarchy.)
		// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
		static const uint8_t valid_ascii_tbl[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x00
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
			1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, // 0x20 (", *, .)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0,	// 0x30 (:, <, >, ?)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, // 0x50 (\\)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, // 0x80 (|)
		};
		unsigned int chr = (unsigned int)filter_cache_key[i];
		if (chr < 0x80 && !valid_ascii_tbl[chr]) {
			// Invalid character.
			filter_cache_key[i] = _RP_CHR('_');
		}

		// Check for "../" (or ".." at the end of the cache key).
		if (foundSlash && chr == '.') {
			dotCount++;
			if (dotCount >= 2) {
				// Invalid cache key.
				return rp_string();
			}
		} else if (chr == '/') {
			foundSlash = true;
			dotCount = 0;
		} else {
			foundSlash = false;
		}
	}

	SemaphoreLocker locker(m_dlsem);

	// Check the main cache key.
	rp_string cache_filename = getCacheFilename(filter_cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return rp_string();
	}

	// Check if the file already exists.
	if (!access(cache_filename, R_OK)) {
		// File exists.
		// Is it larger than 0 bytes?
		int64_t sz = filesize(cache_filename);
		if (sz == 0) {
			// File is 0 bytes, which indicates it didn't exist
			// on the server. If the file is older than a week,
			// try to redownload it.
			// TODO: Configurable time.
			// TODO: How should we handle errors?
			time_t filetime;
			if (get_mtime(cache_filename, &filetime) != 0)
				return rp_string();

			struct timeval systime;
			if (gettimeofday(&systime, nullptr) != 0)
				return rp_string();
			if ((systime.tv_sec - filetime) < (86400*7)) {
				// Less than a week old.
				return rp_string();
			}

			// More than a week old.
			// Delete the cache file and redownload it.
			if (delete_file(cache_filename) != 0)
				return rp_string();
		} else if (sz > 0) {
			// File is larger than 0 bytes, which indicates
			// it was cached successfully.
			return cache_filename;
		}
	}

	// Check if the URL is blank.
	// This is allowed for some databases that are only available offline.
	if (url.empty()) {
		// Blank URL. Don't try to download anything.
		// Don't mark the file as unavailable by creating a
		// 0-byte dummy file, either.
		return rp_string();
	}

	// TODO: Keep-alive cURL connections (one per server)?
	m_downloader->setUrl(url);
	m_downloader->setProxyUrl(m_proxyUrl);
	int ret = m_downloader->download();

	// Write the file to the cache.
	unique_ptr<IRpFile> file(new RpFile(cache_filename, RpFile::FM_CREATE_WRITE));

	if (ret != 0 || !file || !file->isOpen()) {
		// Error downloading the file, or error opening
		// the file in the local cache.

		// TODO: Only keep a negative cache if it's a 404.
		// Keep the cached file as a 0-byte file to indicate
		// a "negative" hit, but return an empty filename.
		return rp_string();
	}

	// Write the file.
	file->write((void*)m_downloader->data(), m_downloader->dataSize());
	file->close();

	// Set the file's mtime if it was obtained by the downloader.
	// TODO: IRpFile::set_mtime()?
	time_t mtime = m_downloader->mtime();
	if (mtime >= 0) {
		set_mtime(cache_filename, mtime);
	}

	// Return the cache filename.
	return cache_filename;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * CacheManager.cpp: Local cache manager.                                  *
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

#ifdef _WIN32
# include "stdafx.h"
#endif
#include "CacheManager.hpp"

// librpbase
#include "librpbase/file/RpFile.hpp"
#include "librpbase/file/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpBase::FileSystem;

// Windows includes.
#ifdef _WIN32
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// gettimeofday()
// NOTE: MSVC doesn't actually have gettimeofday().
// We have our own version in msvc_common.h.
#ifdef _MSC_VER
# include "libwin32common/msvc_common.h"
#else /* !_MSC_VER */
# include <sys/time.h>
#endif /* _MSC_VER */

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;
using std::string;

// TODO: DownloaderFactory?
#ifdef _WIN32
# include "UrlmonDownloader.hpp"
#else
# include "CurlDownloader.hpp"
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
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
 */
void CacheManager::setProxyUrl(const string &proxyUrl)
{
	m_proxyUrl = proxyUrl;
}

/**
 * Get a cache filename.
 * @param cache_key Cache key. (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
string CacheManager::getCacheFilename(const string &cache_key)
{
	// Filter invalid characters from the cache key.
	const string filtered_cache_key = filterCacheKey(cache_key);
	if (filtered_cache_key.empty()) {
		// Invalid cache key.
		return string();
	}

	// Get the cache filename.
	// This is the cache directory plus the cache key.
	string cache_filename = getCacheDirectory();

	if (cache_filename.empty())
		return string();
	if (cache_filename.at(cache_filename.size()-1) != DIR_SEP_CHR)
		cache_filename += DIR_SEP_CHR;

	// Append the filtered cache key.
	cache_filename += filtered_cache_key;

	// Cache filename created.
	return cache_filename;
}

/**
 * Filter invalid characters from a cache key.
 * @param cache_key Cache key.
 * @return Filtered cache key.
 */
string CacheManager::filterCacheKey(const string &cache_key)
{
	// Quick check: Ensure the cache key is not empty and
	// that it doesn't start with a path separator.
	if (cache_key.empty() ||
	    cache_key[0] == '/' || cache_key[0] == '\\')
	{
		// Cache key is either empty or starts with
		// a path separator.
		return string();
	}

	string filtered_cache_key = cache_key;
	bool foundSlash = true;
	int dotCount = 0;
	for (auto iter = filtered_cache_key.begin(); iter != filtered_cache_key.end(); ++iter) {
		// Don't allow control characters, invalid FAT32 characters, or dots.
		// '/' is allowed for cache hierarchy. (Converted to '\\' on Windows.)
		// '.' is allowed for file extensions.
		// (NOTE: '/' and '.' are allowed for extensions and cache hierarchy.)
		// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
		// Values:
		// - 0: Not allowed (converted to '_')
		// - 1: Allowed
		// - 2: Dot
		// - 3: Slash
		// - 4: Backslash or colon (error)
		static const uint8_t valid_ascii_tbl[0x80] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x00
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
			1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 3, // 0x20 (", *, ., /)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0, 1, 0, 0,	// 0x30 (:, <, >, ?)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, // 0x50 (\\)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, // 0x80 (|)
		};
		uint8_t chr = (uint8_t)*iter;
		if (chr & 0x80) {
			// Start of UTF-8 sequence.
			// Verify that the sequence is valid.
			// NOTE: Checking for 0x80 first because most cache keys
			// will be ASCII, not UTF-8.

			// TODO: Remove extra bytes?
			// NOTE: NULL check isn't needed, since these tests will all
			// fail if a NULL byte is encountered.
			if (((chr     & 0xE0) == 0xC0) &&
			    ((iter[1] & 0xC0) == 0x80))
			{
				// Two-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr2 =
					((chr     & 0x1F) <<  6) |
					 (iter[1] & 0x3F);
				if (uchr2 < 0x80) {
					// Overlong sequence. Not allowed.
					*iter = '_';
					continue;
				}

				// Sequence is not overlong.
				iter++;
				continue;
			}
			else if (((chr     & 0xF0) == 0xE0) &&
				 ((iter[1] & 0xC0) == 0x80) &&
				 ((iter[2] & 0xC0) == 0x80))
			{
				// Three-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr3 =
					((chr     & 0x0F) << 12) |
					((iter[1] & 0x3F) <<  6) |
					 (iter[2] & 0x3F);
				if (uchr3 < 0x800) {
					// Overlong sequence. Not allowed.
					*iter = '_';
					continue;
				}

				// Sequence is not overlong.
				iter += 2;
				continue;
			}
			else if (((chr     & 0xF8) == 0xF0) &&
				 ((iter[1] & 0xC0) == 0x80) &&
				 ((iter[2] & 0xC0) == 0x80) &&
				 ((iter[3] & 0xC0) == 0x80))
			{
				// Four-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr4 =
					((chr     & 0x07) << 18) |
					((iter[1] & 0x3F) << 12) |
					((iter[2] & 0x3F) <<  6) |
					 (iter[3] & 0x3F);
				if (uchr4 < 0x10000) {
					// Overlong sequence. Not allowed.
					*iter = '_';
					continue;
				}

				// Sequence is not overlong.
				iter += 3;
				continue;
			}

			// Invalid UTF-8 sequence.
			*iter = '_';
			continue;
		}

		switch (valid_ascii_tbl[chr] & 7) {
			case 0:
			default:
				// Invalid character.
				*iter = '_';
				foundSlash = false;
				break;

			case 1:
				// Valid character.
				foundSlash = false;
				break;

			case 2:
				// Dot.
				// Check for "../" (or ".." at the end of the cache key).
				if (foundSlash) {
					dotCount++;
					if (dotCount >= 2) {
						// Invalid cache key.
						return string();
					}
				}
				break;

			case 3:
				// Slash.
#ifdef _WIN32
				// Convert to backslash on Windows.
				*iter = '\\';
#endif /* _WIN32 */
				foundSlash = true;
				dotCount = 0;
				break;

			case 4:
				// Backslash or colon.
				// Not allowed at all.
				return string();
		}
	}

	return filtered_cache_key;
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
string CacheManager::download(
	const string &url,
	const string &cache_key)
{
	// Check the main cache key.
	string cache_filename = getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// Lock the semaphore to make sure we don't
	// download too many files at once.
	SemaphoreLocker locker(m_dlsem);

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
				return string();

			struct timeval systime;
			if (gettimeofday(&systime, nullptr) != 0)
				return string();
			if ((systime.tv_sec - filetime) < (86400*7)) {
				// Less than a week old.
				return string();
			}

			// More than a week old.
			// Delete the cache file and redownload it.
			if (delete_file(cache_filename) != 0)
				return string();
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
		return string();
	}

	// Make sure the subdirectories exist.
	// NOTE: The filename portion MUST be kept in cache_filename,
	// since the last component is ignored by rmkdir().
	if (rmkdir(cache_filename) != 0) {
		// Error creating subdirectories.
		return string();
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
		return string();
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

/**
 * Check if a file has already been cached.
 * @param cache_key Cache key.
 * @return Filename in the cache, or empty string if not found.
 */
string CacheManager::findInCache(const string &cache_key)
{
	// Get the cache key filename.
	string cache_filename = getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Error obtaining the cache key filename.
		return string();
	}

	// Return the filename if the file exists.
	return (!access(cache_filename, R_OK) ? cache_filename : string());
}

}

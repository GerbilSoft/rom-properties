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

#include "CacheManager.hpp"

#include "libromdata/TextFuncs.hpp"
#include "libromdata/RpFile.hpp"
using LibRomData::rp_string;
using LibRomData::IRpFile;
using LibRomData::RpFile;

// Windows includes.
#ifdef _WIN32
#ifndef RP_UTF16
#error CacheManager only supports RP_UTF16 on Windows.
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#endif /* _WIN32 */

// POSIX includes.
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif /* !_WIN32 */

// C++ includes.
#include <string>
using std::string;

// TODO: DownloaderFactory?
#ifdef _WIN32
#include "UrlmonDownloader.hpp"
#else
#include "CurlDownloader.hpp"
#endif

// File system wrapper functions.
#include "FileSystem.hpp"
using namespace LibCacheMgr::FileSystem;

namespace LibCacheMgr {

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
void CacheManager::setProxyUrl(const LibRomData::rp_string &proxyUrl)
{
	m_proxyUrl = proxyUrl;
}

/**
 * Get the ROM Properties cache directory.
 * @return Cache directory.
 */
const LibRomData::rp_string &CacheManager::cacheDir(void)
{
	if (!m_cacheDir.empty()) {
		// We already got the cache directory.
		return m_cacheDir;
	}

#ifdef _WIN32
	// Windows: Get CSIDL_LOCAL_APPDATA.
	// XP: C:\Documents and Settings\username\Local Settings\Application Data
	// Vista+: C:\Users\username\AppData\Local
	wchar_t path[MAX_PATH];
	HRESULT hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr != S_OK)
		return m_cacheDir;

	// FIXME: Only works with RP_UTF16.
	m_cacheDir = reinterpret_cast<const char16_t*>(path);
	if (m_cacheDir.empty())
		return m_cacheDir;

	// Add a trailing backslash if necessary.
	if (m_cacheDir.at(m_cacheDir.size()-1) != _RP_CHR('\\'))
		m_cacheDir += _RP_CHR('\\');

	// Append "rom-properties\\cache".
	m_cacheDir += _RP("rom-properties\\cache");
#else
	// Linux: Cache directory is ~/.cache/rom-properties/.
	// TODO: Mac OS X.
	// TODO: What if the HOME environment variable isn't set?
	const char *home = getenv("HOME");
	if (!home || home[0] == 0)
		return m_cacheDir;
	string path = home;

	// Add a trailing slash if necessary.
	if (path.at(path.size()-1) != '/')
		path += '/';

	// Append ".cache/rom-properties".
	path += ".cache/rom-properties";

	// Convert to rp_string.
	m_cacheDir = LibRomData::utf8_to_rp_string(path);
#endif

	return m_cacheDir;
}

/**
 * Get a cache filename.
 * @param cache_key Cache key.
 * @return Cache filename, or empty string on error.
 */
rp_string CacheManager::getCacheFilename(const LibRomData::rp_string &cache_key)
{
	// Get the cache filename.
	// This is the cache directory plus the cache key.
	rp_string cache_filename = cacheDir();

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
 * @param url URL.
 * @param cache_key Cache key.
 * @param cache_key_fb Fallback cache key.
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
LibRomData::rp_string CacheManager::download(const rp_string &url,
	const rp_string &cache_key, const rp_string &cache_key_fb)
{
	// Check the main cache key.
	rp_string cache_filename = getCacheFilename(cache_key);
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
			// on the server.
			return rp_string();
		} else if (sz > 0) {
			// File is larger than 0 bytes, which indicates
			// it was cached successfully.
			return cache_filename;
		}
	}

	// Check the fallback cache key.
	if (!cache_key_fb.empty()) {
		rp_string cache_fb_filename = getCacheFilename(cache_key_fb);
		if (!cache_fb_filename.empty()) {
			// Check if the file already exists.
			if (!access(cache_fb_filename, R_OK)) {
				// File exists.
				// Is it larger than 0 bytes?
				int64_t sz = filesize(cache_fb_filename);
				if (sz > 0) {
					// File is larger than 0 bytes, which indicates
					// the fallback entry is valid.
					return cache_fb_filename;
				}
			}
		}
	}

	// TODO: Keep-alive cURL connections (one per server)?
	m_downloader->setUrl(url);
	int ret = m_downloader->download();

	// Write the file to the cache.
	IRpFile *file = new RpFile(cache_filename, RpFile::FM_CREATE_WRITE);

	if (ret != 0) {
		// Error downloading the file.
		// TODO: Only keep a negative cache if it's a 404.
		// Keep the cached file as a 0-byte file to indicate
		// a "negative" hit, but return an empty filename.
		file->close();
		delete file;
		return rp_string();
	}

	// Write the file.
	file->write((void*)m_downloader->data(), m_downloader->dataSize());
	file->close();
	delete file;

	// Return the cache filename.
	return cache_filename;
}

}

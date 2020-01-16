/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * rp-download.cpp: Standalone cache downloader.                           *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

// OS-specific security options.
#include "os-secure.h"

// C includes.
#include <stdlib.h>
#include <sys/stat.h>
#ifndef _WIN32
# include <unistd.h>
#endif /* _WIN32 */

#ifndef __S_ISTYPE
# define __S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))
#endif
#if defined(__S_IFDIR) && !defined(S_IFDIR)
# define S_IFDIR __S_IFDIR
#endif
#ifndef S_ISDIR
# define S_ISDIR(mode) __S_ISTYPE((mode), S_IFDIR)
#endif /* !S_ISTYPE */

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
using std::unique_ptr;

#ifdef _WIN32
// libwin32common
# include "libwin32common/RpWin32_sdk.h"
#endif /* _WIN32 */

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

// librpbase
#include "librpbase/common.h"

// tcharx
#include "tcharx.h"

#ifdef _WIN32
# define _TMKDIR(dirname) _tmkdir(dirname)
using std::wstring;
#else /* !_WIN32 */
# define _TMKDIR(dirname) _tmkdir((dirname), 0777)
using std::string;
#endif /* _WIN32 */

#ifndef _countof
# define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

// TODO: IDownloaderFactory?
#ifdef _WIN32
# include "WinInetDownloader.hpp"
#else
# include "CurlDownloader.hpp"
#endif
#include "SetFileOriginInfo.hpp"
using namespace RpDownload;

static const TCHAR *argv0 = nullptr;
static bool verbose = false;

/**
 * Show command usage.
 */
static void show_usage(void)
{
	_ftprintf(stderr, _T("Syntax: %s [-v] cache_key\n"), argv0);
}

// TODO: SHOW_ERROR() macro that checks for verbose before calling show_error().
/**
 * Show an error message.
 * @param format printf() format string.
 * @param ... printf() format parameters.
 */
static void
#ifndef _MSC_VER
__attribute__ ((format (printf, 1, 2)))
#endif /* _MSC_VER */
show_error(const TCHAR *format, ...)
{
	va_list ap;

	_ftprintf(stderr, _T("%s: "), argv0);
	va_start(ap, format);
	_vftprintf(stderr, format, ap);
	va_end(ap);
	_fputtc(_T('\n'), stderr);
}

/**
 * Check if the file exists.
 * @param filename Filename.
 * @return True if the file exists; false if not.
 */
static bool check_file_exists(const TCHAR *filename)
{
#ifdef _WIN32
	return (GetFileAttributes(filename) != INVALID_FILE_ATTRIBUTES);
#else /* !_WIN32 */
	return (access(filename, F_OK) == 0);
#endif /* _WIN32 */
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return File size on success; negative POSIX error code on error.
 */
static int64_t get_file_size(const TCHAR *filename)
{
#ifdef _WIN32
	struct _stati64 sb;
	int ret = ::_tstati64(filename, &sb);
#else /* _WIN32 */
	struct stat sb;
	int ret = stat(filename, &sb);
#endif /* _WIN32 */

	if (ret != 0) {
		// An error occurred.
		ret = -errno;
		if (ret == 0) {
			// No error?
			ret = -EIO;
		}
		return ret;
	}

	// Make sure this is not a directory.
	if (S_ISDIR(sb.st_mode)) {
		// It's a directory.
		return -EISDIR;
	}

	// Return the file size.
	return sb.st_size;
}

/**
 * Recursively mkdir() subdirectories.
 * (Copied from librpbase's FileSystem_win32.cpp.)
 *
 * The last element in the path will be ignored, so if
 * the entire pathname is a directory, a trailing slash
 * must be included.
 *
 * NOTE: Only native separators ('\\' on Windows, '/' on everything else)
 * are supported by this function.
 *
 * @param path Path to recursively mkdir. (last component is ignored)
 * @return 0 on success; negative POSIX error code on error.
 */
int rmkdir(const tstring &path)
{
#ifdef _WIN32
	// Check if "\\\\?\\" is at the beginning of path.
	tstring tpath;
	if (path.size() >= 4 && !_tcsncmp(path.data(), _T("\\\\?\\"), 4)) {
		// It's at the beginning of the path.
		// We don't want to use it here, though.
		tpath = path.substr(4);
	} else {
		// It's not at the beginning of the path.
		tpath = path;
	}
#else /* !_WIN32 */
	// Use the path as-is.
	tstring tpath = path;
#endif /* _WIN32 */

	if (tpath.size() == 3) {
		// 3 characters. Root directory is always present.
		return 0;
	} else if (tpath.size() < 3) {
		// Less than 3 characters. Path isn't valid.
		return -EINVAL;
	}

	// Find all backslashes and ensure the directory component exists.
	// (Skip the drive letter and root backslash.)
	size_t slash_pos = 4;
	while ((slash_pos = tpath.find(DIR_SEP_CHR, slash_pos)) != tstring::npos) {
		// Temporarily NULL out this slash.
		tpath[slash_pos] = _T('\0');

		// Attempt to create this directory.
		if (::_TMKDIR(tpath.c_str()) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		tpath[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
	}

	// rmkdir() succeeded.
	return 0;
}

/**
 * rp-download: Download an image from a supported online database.
 * @param cache_key Cache key, e.g. "ds/cover/US/ADAE.png"
 * @return 0 on success; non-zero on error.
 *
 * TODO:
 * - More error codes based on the error.
 */
int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Create a downloader based on OS:
	// - Linux: CurlDownloader
	// - Windows: WinInetDownloader

	// Syntax: rp-download cache_key
	// Example: rp-download ds/coverM/US/ADAE.png

	// If http_proxy or https_proxy are set, they will be used
	// by the downloader code if supported.

	// Set OS-specific security options.
	rp_download_os_secure();

	// Store argv[0] globally.
	argv0 = argv[0];

	const TCHAR *cache_key = argv[1];
	if (argc < 2) {
		// TODO: Add a verbose option to print messages.
		// Normally, the only output is a return value.
		show_usage();
		return EXIT_FAILURE;
	}

	// Check for "-v" or "--verbose".
	if (!_tcscmp(argv[1], _T("-v")) || !_tcscmp(argv[1], _T("--verbose"))) {
		// Verbose mode is enabled.
		verbose = true;
		// We need at least three parameters now.
		if (argc < 3) {
			show_error(_T("No cache key specified."));
			show_usage();
			return EXIT_FAILURE;
		}
		cache_key = argv[2];
	}

	// Check the cache key prefix. The prefix indicates the system
	// and identifies the online database used.
	// [key] indicates the cache key without the prefix.
	// - wii:    https://art.gametdb.com/wii/[key]
	// - wiiu:   https://art.gametdb.com/wiiu/[key]
	// - 3ds:    https://art.gametdb.com/3ds/[key]
	// - ds:     https://art.gametdb.com/3ds/[key]
	// - amiibo: https://amiibo.life/[key]/image
	const TCHAR *slash_pos = _tcschr(cache_key, _T('/'));
	if (slash_pos == nullptr) {
		// No slash. Not a valid cache key.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	} else if (slash_pos[1] == '\0') {
		// Slash is the last character.
		// Not a valid cache key.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	} else if (slash_pos == cache_key) {
		// Slash is the first character.
		// Not a valid cache key.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	}

	const ptrdiff_t prefix_len = (slash_pos - cache_key);
	if (prefix_len <= 0) {
		// Empty prefix.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	}

	// Cache key must include a lowercase file extension.
	const TCHAR *const lastdot = _tcsrchr(cache_key, _T('.'));
	if (!lastdot) {
		// No dot...
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	}
	if (_tcscmp(lastdot, _T(".png")) != 0 &&
	    _tcscmp(lastdot, _T(".jpg")) != 0)
	{
		// Not a supported file extension.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	}

	// Determine the full URL based on the cache key.
	TCHAR full_url[256];
	if ((prefix_len == 3 && !_tcsncmp(cache_key, _T("wii"), 3)) ||
	    (prefix_len == 5 && !_tcsncmp(cache_key, _T("wiiu"), 4)) ||
	    (prefix_len == 3 && !_tcsncmp(cache_key, _T("3ds"), 3)) ||
	    (prefix_len == 2 && !_tcsncmp(cache_key, _T("ds"), 2)))
	{
		// Wii, Wii U, Nintendo 3DS, Nintendo DS
		_sntprintf(full_url, _countof(full_url),
			_T("https://art.gametdb.com/%s"), cache_key);
	} else if (prefix_len == 6 && !_tcsncmp(cache_key, _T("amiibo"), 6)) {
		// amiibo.
		// NOTE: We need to remove the file extension.
		size_t filename_len = _tcslen(slash_pos+1);
		if (filename_len <= 4) {
			// Can't remove the extension...
			if (verbose) {
				show_error(_T("Cache key '%s' is invalid."), cache_key);
			}
			return EXIT_FAILURE;
		}
		filename_len -= 4;

		_sntprintf(full_url, _countof(full_url),
			_T("https://amiibo.life/nfc/%.*s/image"),
			static_cast<int>(filename_len), slash_pos+1);
	} else {
		// Prefix is not supported.
		if (verbose) {
			show_error(_T("Cache key '%s' has an unsupported prefix."), cache_key);
		}
		return EXIT_FAILURE;
	}

	if (verbose) {
		_ftprintf(stderr, _T("URL: %s\n"), full_url);
	}

	// Get the cache filename.
	tstring cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Invalid cache filename.
		if (verbose) {
			show_error(_T("Cache key '%s' is invalid."), cache_key);
		}
		return EXIT_FAILURE;
	}
	if (verbose) {
		_ftprintf(stderr, _T("Cache Filename: %s\n"), cache_filename.c_str());
	}

	// If the cache_filename is >= 240 characters, prepend "\\\\?\\".
	if (cache_filename.size() >= 240) {
		cache_filename.reserve(cache_filename.size() + 8);
		cache_filename.insert(0, _T("\\\\?\\"));
	}

	// Does the cache file already exist?
	// TODO: Open the file to prevent TOCTOU issues?
	if (check_file_exists(cache_filename.c_str())) {
		// Check if the file is 0 bytes.
		// If it is, we'll redownload it.
		// (TODO: Implement mtime check here or no?)
		int64_t filesize = get_file_size(cache_filename.c_str());
		if (filesize < 0) {
			// Error obtaining the file size.
			if (verbose) {
				show_error(_T("Error checking cache file: %s"), _tcserror((int)-filesize));
			}
			return EXIT_FAILURE;
		}

		if (filesize > 0) {
			// Filesize is non-zero. The file doesn't need to be downloaded.
			if (verbose) {
				show_error(_T("Cache file for '%s' is already downloaded."), cache_key);
			}
			return EXIT_SUCCESS;
		}

		// Delete the file and try to download it again.
		if (_tremove(cache_filename.c_str()) != 0) {
			if (verbose) {
				show_error(_T("Error deleting 0-byte cache file: %s"), _tcserror(errno));
			}
			return EXIT_FAILURE;
		}
	} else {
		// Make sure the path structure exists.
		int ret = rmkdir(cache_filename.c_str());
		if (ret != 0) {
			if (verbose) {
				show_error(_T("Error creating directory structure: %s"), _tcserror(errno));
			}
			return EXIT_FAILURE;
		}
	}

	// Attempt to download the file.
	// TODO: IDownloaderFactory?
#ifdef _WIN32
	unique_ptr<IDownloader> m_downloader(new WinInetDownloader());
#else /* !_WIN32 */
	unique_ptr<IDownloader> m_downloader(new CurlDownloader());
#endif /* _WIN32 */

	// TODO: Configure this somewhere?
	m_downloader->setMaxSize(4*1024*1024);

	m_downloader->setUrl(full_url);
	int ret = m_downloader->download();
	if (ret != 0) {
		// Error downloading the file.
		// TODO: HTTP error, cURL error, etc.
		if (verbose) {
			show_error(_T("Error downloading file: %d"), ret);
		}
		return EXIT_FAILURE;
	}

	if (m_downloader->dataSize() <= 0) {
		// No data downloaded...
		if (verbose) {
			show_error(_T("Error downloading file: 0 bytes received"));
		}
		return EXIT_FAILURE;
	}

	// Write the file to the cache.
	FILE *f_out = _tfopen(cache_filename.c_str(), _T("wb"));
	if (!f_out) {
		// Error opening the cache file.
		if (verbose) {
			show_error(_T("Error writing to cache file: %s"), _tcserror(errno));
		}
		return EXIT_FAILURE;
	}

	// TODO: Verify the size.
	size_t size = fwrite(m_downloader->data(), 1, m_downloader->dataSize(), f_out);

	// Save the file origin information.
#ifdef _WIN32
	// TODO: Figure out how to setFileOriginInfo() on Windows
	// using an open file handle.
	setFileOriginInfo(f_out, cache_filename.c_str(), full_url, m_downloader->mtime());
#else /* !_WIN32 */
	setFileOriginInfo(f_out, full_url, m_downloader->mtime());
#endif /* _WIN32 */
	fclose(f_out);

	// Success.
	return EXIT_SUCCESS;
}

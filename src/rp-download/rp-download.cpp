/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * rp-download.cpp: Standalone cache downloader.                           *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rp-download.h"

// OS-specific security options
#include "rp-download_secure.h"

// librpfile
#include "librpfile/FileSystem.hpp"
using namespace LibRpFile;

// C includes.
#ifndef _WIN32
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif /* _WIN32 */

#ifndef __S_ISTYPE
#  define __S_ISTYPE(mode, mask) (((mode) & S_IFMT) == (mask))
#endif
#if defined(__S_IFDIR) && !defined(S_IFDIR)
#  define S_IFDIR __S_IFDIR
#endif
#ifndef S_ISDIR
#  define S_ISDIR(mode) __S_ISTYPE((mode), S_IFDIR)
#endif /* !S_ISTYPE */

// C includes. (C++ namespace)
#include <cerrno>
#include <cstdarg>
#include <cstdio>

// C++ includes.
#include <memory>
using std::string;
using std::tstring;
using std::unique_ptr;

#ifdef _WIN32
// libwin32common
#  include "libwin32common/RpWin32_sdk.h"
#  include "libwin32common/w32err.hpp"
#  include "libwin32common/w32time.h"
#endif /* _WIN32 */

// libcachecommon
#include "libcachecommon/CacheDir.hpp"
#include "libcachecommon/CacheKeys.hpp"

// TODO: IDownloaderFactory?
#ifdef _WIN32
#  include "WinInetDownloader.hpp"
#else
#  include "CurlDownloader.hpp"
#endif
#include "SetFileOriginInfo.hpp"
using namespace RpDownload;

// HTTP status codes
#include "http-status.hpp"

// CacheKeyVerify
#include "CacheKeyVerify.hpp"

static const TCHAR *argv0 = nullptr;
static bool verbose = false;

#if defined(_WIN32) && defined(_UNICODE)
// Cannot do ATTR_PRINTF on Win32 if using Unicode.
#  define ATTR_TPRINTF(fmt, args)
#else
#  define ATTR_TPRINTF ATTR_PRINTF
#endif

/**
 * Show command usage.
 */
static void show_usage(void)
{
	_ftprintf(stderr, _T("Syntax: %s [-v] [-f] cache_key\n"), argv0);
}

/**
 * Show an error message.
 * This includes the program's filename from argv[0].
 * @param format printf() format string.
 * @param ... printf() format parameters.
 */
static void ATTR_TPRINTF(1, 2) show_error(const TCHAR *format, ...)
{
	va_list ap;

	_ftprintf(stderr, _T("%s: "), argv0);
	va_start(ap, format);
	_vftprintf(stderr, format, ap);
	va_end(ap);
	_fputtc(_T('\n'), stderr);
}

#define SHOW_ERROR(...) if (verbose) show_error(__VA_ARGS__)

/**
 * Show an information message.
 * This does *not* include the program's filename from argv[0].
 * @param format printf() format string.
 * @param ... printf() format parameters.
 */
static void ATTR_TPRINTF(1, 2) show_info(const TCHAR *format, ...)
{
	va_list ap;

	va_start(ap, format);
	_vftprintf(stderr, format, ap);
	va_end(ap);
	_fputtc(_T('\n'), stderr);
}

#define SHOW_INFO(...) if (verbose) show_info(__VA_ARGS__)

/**
 * Get a file's size and time.
 * @param filename	[in] Filename.
 * @param pFileSize	[out] File size.
 * @param pMtime	[out] Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
static int get_file_size_and_mtime(const TCHAR *filename, off64_t *pFileSize, time_t *pMtime)
{
	assert(pFileSize != nullptr);
	assert(pMtime != nullptr);

#if defined(_WIN32)
	// Windows: Use FindFirstFile(), since the stat() functions
	// have to do a lot more processing.
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(filename, &ffd);
	if (!hFind || hFind == INVALID_HANDLE_VALUE) {
		// An error occurred.
		const int err = w32err_to_posix(GetLastError());
		return (err != 0 ? -err : -EIO);
	}

	// Make sure this is not a directory.
	if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// It's a directory.
		FindClose(hFind);
		return -EISDIR;
	}

	// Convert the file size from two DWORDs to off64_t.
	LARGE_INTEGER fileSize;
	fileSize.LowPart = ffd.nFileSizeLow;
	fileSize.HighPart = ffd.nFileSizeHigh;
	*pFileSize = fileSize.QuadPart;

	// Convert mtime from FILETIME.
	*pMtime = FileTimeToUnixTime(&ffd.ftLastWriteTime);
	FindClose(hFind);
#elif defined(HAVE_STATX)
	// Linux or UNIX system with statx()
	struct statx sbx;
	if (statx(AT_FDCWD, filename, 0, STATX_TYPE | STATX_MTIME | STATX_SIZE, &sbx) != 0) {
		// An error occurred.
		const int err = errno;
		return (err != 0 ? -err : -EIO);
	}

	// Make sure this is not a directory.
	if ((sbx.stx_mask & STATX_TYPE) && S_ISDIR(sbx.stx_mode)) {
		// It's a directory.
		return -EISDIR;
	}

	// Make sure we got the file size and mtime.
	if ((sbx.stx_mask & (STATX_MTIME | STATX_SIZE)) != (STATX_MTIME | STATX_SIZE)) {
		// One of these is missing...
		return -EIO;
	}

	// Return the file size and mtime.
	*pFileSize = sbx.stx_size;
	*pMtime = sbx.stx_mtime.tv_sec;
#else
	// Linux/UNIX, and statx() isn't available. Use stat().
	struct stat sb;
	if (stat(filename, &sb) != 0) {
		// An error occurred.
		const int err = errno;
		return (err != 0 ? -err : -EIO);
	}

	// Make sure this is not a directory.
	if (S_ISDIR(sb.st_mode)) {
		// It's a directory.
		return -EISDIR;
	}

	// Return the file size and mtime.
	*pFileSize = sb.st_size;
	*pMtime = sb.st_mtime;
#endif

	return 0;
}

/**
 * Get the MIME type for the specified cache key (or filename).
 * @param cache_key Cache key (or filename)
 * @return MIME type, or nullptr if not supported
 */
static const TCHAR *getMimeType(const TCHAR *cache_key)
{
	// Find the last dot.
	const TCHAR *const dotpos = _tcsrchr(cache_key, _T('.'));
	if (!dotpos) {
		return nullptr;
	}

	// Check the file extension.
	const TCHAR *mimeType = nullptr;

	if (!_tcscmp(dotpos, _T(".png"))) {
		mimeType = _T("image/png");
	} else if (!_tcscmp(dotpos, _T(".jpg"))) {
		mimeType = _T("image/jpg");
	} else if (!_tcscmp(dotpos, _T(".txt"))) {
		mimeType = _T("text/plain");
	}

	return mimeType;
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

	// Enable security options.
	rp_download_do_security_options();

#ifdef __GLIBC__
	// Reduce /etc/localtime stat() calls.
	// References:
	// - https://lwn.net/Articles/944499/
	// - https://gitlab.com/procps-ng/procps/-/merge_requests/119
	setenv("TZ", ":/etc/localtime", 0);
#endif /* __GLIBC__ */

	// Store argv[0] globally.
	argv0 = argv[0];

	if (argc < 2) {
		show_usage();
		return EXIT_FAILURE;
	}

	// Check for arguments. (simple non-getopt version)
	bool force = false;
	int optind = 1;
	for (; optind < argc; optind++) {
		if (!argv[optind] || argv[optind][0] != '-') {
			// End of options.
			break;
		}

		// Allow multiple options in one argument, e.g. '-vf'.
		for (int i = 1; argv[optind][i] != '\0'; i++) {
			switch (argv[optind][i]) {
				case 'v':
					// Verbose mode is enabled.
					verbose = true;
					break;
				case 'f':
					// Force download is enabled.
					force = true;
					break;
				default:
					// Invalid parameter.
					show_error(_T("Unrecognized option: %c"), argv[optind][i]);
					show_usage();
					return EXIT_FAILURE;
			}
		}
	}

	if (optind >= argc) {
		show_error(_T("No cache key specified."));
		show_usage();
		return EXIT_FAILURE;
	}
	const TCHAR *const cache_key = argv[optind];

	tstring full_url;
	bool check_newer = false;
	CacheKeyError ckerr = verifyCacheKey(full_url, check_newer, cache_key);
	switch (ckerr) {
		case CacheKeyError::OK:
			break;

		default:
		case CacheKeyError::Invalid:
			SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
			return EXIT_FAILURE;

		case CacheKeyError::PrefixNotSupported:
			SHOW_ERROR(_T("Cache key '%s' has an unsupported prefix."), cache_key);
			return EXIT_FAILURE;
	}

	// TODO: IDownloaderFactory?
#ifdef _WIN32
	WinInetDownloader downloader;
#else /* !_WIN32 */
	CurlDownloader downloader;
#endif /* _WIN32 */

	if (verbose) {
		_ftprintf(stderr, _T("User-Agent: %s\nURL: %s\n"),
			downloader.userAgent().c_str(), full_url.c_str());
	}

	// Make sure we have a valid cache directory.
	const string &cache_dir = LibCacheCommon::getCacheDirectory();
	if (cache_dir.empty()) {
		// Cache directory is invalid...
		// This may happen if bubblewrap is in use.
		SHOW_ERROR(_T("Unable to access cache directory. Check the sandbox environment!"));
		return EXIT_FAILURE;
	}

	// Get the cache filename.
	tstring cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Invalid cache filename.
		SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
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

	// Get the cache file information.
	off64_t filesize = 0;
	time_t filemtime = -1;
	int ret = get_file_size_and_mtime(cache_filename.c_str(), &filesize, &filemtime);
	if (ret == 0) {
		// Check if the file is 0 bytes.
		// TODO: How should we handle errors?
		if (filesize == 0 && !check_newer) {
			// File is 0 bytes, which indicates it didn't exist on the server.
			// If the file is older than a week, try to redownload it.
			// NOTE: Not used for "check_newer" files, e.g. "sys/".
			// TODO: Configurable time.
			const time_t systime = time(nullptr);
			if ((systime - filemtime) < (86400*7)) {
				// Less than a week old.
				if (likely(!force)) {
					SHOW_INFO(_T("Negative cache file for '%s' has not expired; not redownloading."), cache_key);
					return EXIT_FAILURE;
				} else {
					SHOW_INFO(_T("Negative cache file for '%s' has not expired, but -f was specified. Redownloading anyway."), cache_key);
				}
			}

			// More than a week old.
			// Delete the cache file and try to download it again.
			if (_tremove(cache_filename.c_str()) != 0) {
				SHOW_ERROR(_T("Error deleting negative cache file for '%s': %s"), cache_key, _tcserror(errno));
				return EXIT_FAILURE;
			}
		} else if (filesize > 0) {
			// File is larger than 0 bytes, which indicates
			// it was previously cached successfully
			if (unlikely(check_newer)) {
				SHOW_INFO(_T("Cache file for '%s' is already downloaded, but this cache key is set to download-if-newer."), cache_key);
			} else if (unlikely(force)) {
				SHOW_INFO(_T("Cache file for '%s' is already downloaded, but -f was specified. Redownloading anyway."), cache_key);
				if (_tremove(cache_filename.c_str()) != 0) {
					SHOW_ERROR(_T("Error deleting cache file for '%s': %s"), cache_key, _tcserror(errno));
					return EXIT_FAILURE;
				}
			} else {
				SHOW_INFO(_T("Cache file for '%s' is already downloaded."), cache_key);
				return EXIT_SUCCESS;
			}
		}
	} else if (ret == -ENOENT) {
		// File not found. We'll need to download it.
		// Make sure the path structure exists.
		int ret = FileSystem::rmkdir(cache_filename, 0700);
		if (ret != 0) {
			SHOW_ERROR(_T("Error creating directory structure: %s"), _tcserror(-ret));
			return EXIT_FAILURE;
		}
	} else {
		// Other error.
		SHOW_ERROR(_T("Error checking cache file for '%s': %s"), cache_key, _tcserror(-ret));
		return EXIT_FAILURE;
	}

	// Attempt to download the file.
	// TODO: Configure this somewhere?
	downloader.setMaxSize(4*1024*1024);

	if (check_newer && filemtime >= 0) {
		// Only download if the file on the server is newer than
		// what's in our cache directory.
		downloader.setIfModifiedSince(filemtime);
	}

	// Set the MIME type, if available.
	const TCHAR *mimeType = getMimeType(cache_key);
	if (mimeType) {
		downloader.setRequestedMimeType(mimeType);
	}

	downloader.setUrl(full_url);
	ret = downloader.download();
	if (ret != 0) {
		// Error downloading the file.
		if (ret < 0) {
			// POSIX error code
			SHOW_ERROR(_T("Error downloading file: %s"), _tcserror(-ret));
			// Create a 0-byte file to indicate an error occurred.
			FILE *f_out = _tfopen(cache_filename.c_str(), _T("wb"));
			if (f_out) {
				fclose(f_out);
			}
		} else if (ret == 304 && check_newer) {
			// HTTP 304 Not Modified
			SHOW_ERROR(_T("File has not been modified on the server. Not redownloading."));
			return EXIT_SUCCESS;
		} else /*if (ret > 0)*/ {
			// HTTP status code
			if (verbose) {
				const TCHAR *msg = http_status_string(ret);
				if (msg) {
					show_error(_T("Error downloading file: HTTP %d %s"), ret, msg);
				} else {
					show_error(_T("Error downloading file: HTTP %d"), ret);
				}
			}
			// Create a 0-byte file to indicate an error occurred.
			FILE *f_out = _tfopen(cache_filename.c_str(), _T("wb"));
			if (f_out) {
				fclose(f_out);
			}
		}
		return EXIT_FAILURE;
	}

	if (downloader.dataSize() <= 0) {
		// No data downloaded...
		SHOW_ERROR(_T("Error downloading file: 0 bytes received"));
		return EXIT_FAILURE;
	}

	FILE *f_out = _tfopen(cache_filename.c_str(), _T("wb"));
	if (!f_out) {
		// Error opening the cache file.
		SHOW_ERROR(_T("Error writing to cache file: %s"), _tcserror(errno));
		return EXIT_FAILURE;
	}

	// Write the file to the cache.
	// TODO: Verify the size.
	const size_t dataSize = downloader.dataSize();
	size_t size = fwrite(downloader.data(), 1, dataSize, f_out);
	fflush(f_out);

	// Save the file origin information.
	setFileOriginInfo(f_out, full_url.c_str(), downloader.mtime());
	fclose(f_out);

	// Success.
	SHOW_INFO(_T("Downloaded cache file for '%s': %u byte%s."),
		cache_key, static_cast<unsigned int>(dataSize),
		unlikely(dataSize == 1) ? "" : "s");
	return EXIT_SUCCESS;
}

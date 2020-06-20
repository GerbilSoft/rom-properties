/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * rp-download.cpp: Standalone cache downloader.                           *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.rp-download.h"

// OS-specific security options.
#include "librpsecure/os-secure.h"

// C includes.
#ifndef _WIN32
# include <fcntl.h>
# include <sys/stat.h>
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

// C++ includes.
#include <memory>
using std::tstring;
using std::unique_ptr;

#ifdef _WIN32
// libwin32common
# include "libwin32common/RpWin32_sdk.h"
# include "libwin32common/w32err.h"
# include "libwin32common/w32time.h"
#endif /* _WIN32 */

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

#ifdef _WIN32
# include <direct.h>
# define _TMKDIR(dirname) _tmkdir(dirname)
#else /* !_WIN32 */
# define _TMKDIR(dirname) _tmkdir((dirname), 0777)
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

// HTTP status codes.
#include "http-status.h"

static const TCHAR *argv0 = nullptr;
static bool verbose = false;

/**
 * Show command usage.
 */
static void show_usage(void)
{
	_ftprintf(stderr, _T("Syntax: %s [-v] cache_key\n"), argv0);
}

/**
 * Show an error message.
 * @param format printf() format string.
 * @param ... printf() format parameters.
 */
static void
#if !defined(_MSC_VER) && !defined(_UNICODE)
__attribute__ ((format (printf, 1, 2)))
#endif /* !_MSC_VER && !_UNICODE */
show_error(const TCHAR *format, ...)
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

	// We don't need the Find handle anymore.
	FindClose(hFind);

	// Make sure this is not a directory.
	if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// It's a directory.
		return -EISDIR;
	}

	// Convert the file size from two DWORDs to off64_t.
	LARGE_INTEGER fileSize;
	fileSize.LowPart = ffd.nFileSizeLow;
	fileSize.HighPart = ffd.nFileSizeHigh;
	*pFileSize = fileSize.QuadPart;

	// Convert mtime from FILETIME.
	*pMtime = FileTimeToUnixTime(&ffd.ftLastWriteTime);
#elif defined(HAVE_STATX)
	// Linux or UNIX system with statx()
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename, 0, STATX_TYPE | STATX_MTIME | STATX_SIZE, &sbx);
	if (ret != 0) {
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
	int ret = stat(filename, &sb);
	if (ret != 0) {
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

	// Reduce process integrity, if available.
	rp_secure_reduce_integrity();

	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.

		// NOTE: Special case for clone(). If it's the first syscall
		// in the list, it has a parameter restriction added that
		// ensures it can only be used to create threads.
		SCMP_SYS(clone),
		// Other multi-threading syscalls
		SCMP_SYS(set_robust_list),

		SCMP_SYS(access), SCMP_SYS(clock_gettime),
#if defined(__SNR_clock_gettime64) || defined(__NR_clock_gettime64)
		SCMP_SYS(clock_gettime64),
#endif /* __SNR_clock_gettime64 || __NR_clock_gettime64 */
		SCMP_SYS(close),
		SCMP_SYS(fcntl), SCMP_SYS(fcntl64),
		SCMP_SYS(fsetxattr),
		SCMP_SYS(fstat),     SCMP_SYS(fstat64),		// __GI___fxstat() [printf()]
		SCMP_SYS(fstatat64), SCMP_SYS(newfstatat),	// Ubuntu 19.10 (32-bit)
		SCMP_SYS(futex),
		SCMP_SYS(getdents), SCMP_SYS(getdents64),
		SCMP_SYS(getrusage),
		SCMP_SYS(gettimeofday),	// 32-bit only?
		SCMP_SYS(getuid),
		SCMP_SYS(lseek), SCMP_SYS(_llseek),
		//SCMP_SYS(lstat), SCMP_SYS(lstat64),	// Not sure if used?
		SCMP_SYS(mkdir), SCMP_SYS(mmap), SCMP_SYS(mmap2),
		SCMP_SYS(munmap),
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#if defined(__SNR_openat2)
		SCMP_SYS(openat2),	// Linux 5.6
#elif defined(__NR_openat2)
		__NR_openat2,		// Linux 5.6
#endif /* __SNR_openat2 || __NR_openat2 */
		SCMP_SYS(poll), SCMP_SYS(select),
		SCMP_SYS(stat), SCMP_SYS(stat64),
		SCMP_SYS(utimensat),

#if defined(__SNR_statx) || defined(__NR_statx)
		SCMP_SYS(getcwd),	// called by glibc's statx()
		SCMP_SYS(statx),
#endif /* __SNR_statx || __NR_statx */

		// glibc ncsd
		// TODO: Restrict connect() to AF_UNIX.
		SCMP_SYS(connect), SCMP_SYS(recvmsg), SCMP_SYS(sendto),
		SCMP_SYS(sendmmsg),	// getaddrinfo() (32-bit only?)
		SCMP_SYS(ioctl),	// getaddrinfo() (32-bit only?) [FIXME: Filter for FIONREAD]
		SCMP_SYS(recvfrom),	// getaddrinfo() (32-bit only?)

		// cURL and OpenSSL
		SCMP_SYS(bind),		// getaddrinfo() [curl_thread_create_thunk(), curl-7.68.0]
#ifdef __SNR_getrandom
		SCMP_SYS(getrandom),
#endif /* __SNR_getrandom */
		SCMP_SYS(getpeername), SCMP_SYS(getsockname),
		SCMP_SYS(getsockopt), SCMP_SYS(madvise), SCMP_SYS(mprotect),
		SCMP_SYS(setsockopt), SCMP_SYS(socket),
		SCMP_SYS(socketcall),	// FIXME: Enhanced filtering? [cURL+GnuTLS only?]
		SCMP_SYS(socketpair), SCMP_SYS(sysinfo),

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - inet: Internet access.
	// - fattr: Modify file attributes, e.g. mtime.
	// - dns: Resolve hostnames.
	// - getpw: Get user's home directory if HOME is empty.
	param.promises = "stdio rpath wpath cpath inet fattr dns getpw";
#elif defined(HAVE_TAME)
	// NOTE: stdio includes fattr, e.g. utimes().
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH |
	                   TAME_INET | TAME_DNS | TAME_GETPW;
#else
	param.dummy = 0;
#endif
	rp_secure_enable(param);

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
	// - gba:    https://rpdb.gerbilsoft.com/gba/[key]
	// - gb:     https://rpdb.gerbilsoft.com/gb/[key]
	// - snes:   https://rpdb.gerbilsoft.com/snes/[key]
	const TCHAR *slash_pos = _tcschr(cache_key, _T('/'));
	if (slash_pos == nullptr || slash_pos == cache_key ||
		slash_pos[1] == '\0')
	{
		// Invalid cache key:
		// - Does not contain any slashes.
		// - First slash is either the first or the last character.
		SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
		return EXIT_FAILURE;
	}

	const ptrdiff_t prefix_len = (slash_pos - cache_key);
	if (prefix_len <= 0) {
		// Empty prefix.
		SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
		return EXIT_FAILURE;
	}

	// Cache key must include a lowercase file extension.
	const TCHAR *const lastdot = _tcsrchr(cache_key, _T('.'));
	if (!lastdot) {
		// No dot...
		SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
		return EXIT_FAILURE;
	}
	if (_tcscmp(lastdot, _T(".png")) != 0 &&
	    _tcscmp(lastdot, _T(".jpg")) != 0)
	{
		// Not a supported file extension.
		SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
		return EXIT_FAILURE;
	}

	// urlencode the cache key.
	const tstring cache_key_urlencode = LibCacheCommon::urlencode(cache_key);
	// Update the slash position based on the urlencoded string.
	slash_pos = _tcschr(cache_key_urlencode.data(), _T('/'));

	// Determine the full URL based on the cache key.
	TCHAR full_url[256];
	if ((prefix_len == 3 && !_tcsncmp(cache_key, _T("wii"), 3)) ||
	    (prefix_len == 4 && !_tcsncmp(cache_key, _T("wiiu"), 4)) ||
	    (prefix_len == 3 && !_tcsncmp(cache_key, _T("3ds"), 3)) ||
	    (prefix_len == 2 && !_tcsncmp(cache_key, _T("ds"), 2)))
	{
		// Wii, Wii U, Nintendo 3DS, Nintendo DS
		_sntprintf(full_url, _countof(full_url),
			_T("https://art.gametdb.com/%s"), cache_key_urlencode.c_str());
	} else if (prefix_len == 6 && !_tcsncmp(cache_key, _T("amiibo"), 6)) {
		// amiibo.
		// NOTE: We need to remove the file extension.
		size_t filename_len = _tcslen(slash_pos+1);
		if (filename_len <= 4) {
			// Can't remove the extension...
			SHOW_ERROR(_T("Cache key '%s' is invalid."), cache_key);
			return EXIT_FAILURE;
		}
		filename_len -= 4;

		_sntprintf(full_url, _countof(full_url),
			_T("https://amiibo.life/nfc/%.*s/image"),
			static_cast<int>(filename_len), slash_pos+1);
	} else if ((prefix_len == 3 && !_tcsncmp(cache_key, _T("gba"), 3)) ||
		   (prefix_len == 2 && !_tcsncmp(cache_key, _T("gb"), 2)) ||
		   (prefix_len == 4 && !_tcsncmp(cache_key, _T("snes"), 4))) {
		// Game Boy, Game Boy Color, Game Boy Advance, Super NES
		_sntprintf(full_url, _countof(full_url),
			_T("https://rpdb.gerbilsoft.com/%s"), cache_key_urlencode.c_str());
	} else {
		// Prefix is not supported.
		SHOW_ERROR(_T("Cache key '%s' has an unsupported prefix."), cache_key);
		return EXIT_FAILURE;
	}

	if (verbose) {
		_ftprintf(stderr, _T("URL: %s\n"), full_url);
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

#if defined(_WIN32) && defined(_UNICODE)
	// If the cache_filename is >= 240 characters, prepend "\\\\?\\".
	if (cache_filename.size() >= 240) {
		cache_filename.reserve(cache_filename.size() + 8);
		cache_filename.insert(0, _T("\\\\?\\"));
	}
#endif /* _WIN32 && _UNICODE */

	// Get the cache file information.
	off64_t filesize = 0;
	time_t filemtime = 0;
	int ret = get_file_size_and_mtime(cache_filename.c_str(), &filesize, &filemtime);
	if (ret == 0) {
		// Check if the file is 0 bytes.
		// TODO: How should we handle errors?
		if (filesize == 0) {
			// File is 0 bytes, which indicates it didn't exist
			// on the server. If the file is older than a week,
			// try to redownload it.
			// TODO: Configurable time.
			const time_t systime = time(nullptr);
			if ((systime - filemtime) < (86400*7)) {
				// Less than a week old.
				SHOW_ERROR(_T("Negative cache file for '%s' has not expired; not redownloading."), cache_key);
				return EXIT_FAILURE;
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
			SHOW_ERROR(_T("Cache file for '%s' is already downloaded."), cache_key);
			return EXIT_SUCCESS;
		}
	} else if (ret == -ENOENT) {
		// File not found. We'll need to download it.
		// Make sure the path structure exists.
		int ret = rmkdir(cache_filename.c_str());
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
	// TODO: IDownloaderFactory?
#ifdef _WIN32
	unique_ptr<IDownloader> m_downloader(new WinInetDownloader());
#else /* !_WIN32 */
	unique_ptr<IDownloader> m_downloader(new CurlDownloader());
#endif /* _WIN32 */

	// Open the cache file now so we can use it as a negative hit
	// if the download fails.
	FILE *f_out = _tfopen(cache_filename.c_str(), _T("wb"));
	if (!f_out) {
		// Error opening the cache file.
		SHOW_ERROR(_T("Error writing to cache file: %s"), _tcserror(errno));
		return EXIT_FAILURE;
	}

	// TODO: Configure this somewhere?
	m_downloader->setMaxSize(4*1024*1024);

	m_downloader->setUrl(full_url);
	ret = m_downloader->download();
	if (ret != 0) {
		// Error downloading the file.
		if (verbose) {
			if (ret < 0) {
				// POSIX error code
				show_error(_T("Error downloading file: %s"), _tcserror(errno));
			} else /*if (ret > 0)*/ {
				// HTTP status code
				const TCHAR *msg = http_status_string(ret);
				if (msg) {
					show_error(_T("Error downloading file: HTTP %d %s"), ret, msg);
				} else {
					show_error(_T("Error downloading file: HTTP %d"), ret);
				}
			}
		}
		fclose(f_out);
		return EXIT_FAILURE;
	}

	if (m_downloader->dataSize() <= 0) {
		// No data downloaded...
		SHOW_ERROR(_T("Error downloading file: 0 bytes received"));
		fclose(f_out);
		return EXIT_FAILURE;
	}

	// Write the file to the cache.
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

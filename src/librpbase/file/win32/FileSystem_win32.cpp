/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * FileSystem_posix.cpp: File system functions. (Win32 implementation)     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "../FileSystem.hpp"

// libromdata
#include "TextFuncs.hpp"

// One-time initialization.
#include "threads/pthread_once.h"

// C includes.
#include <sys/utime.h>

// C includes. (C++ namespace)
#include <cstring>
#include <ctime>
#include <cwctype>

// C++ includes.
#include <string>
using std::string;
using std::u16string;
using std::wstring;

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.h"
#include "libwin32common/w32time.h"

// Windows includes.
#include <shlobj.h>
#include <direct.h>

// FIXME: This is Vista only.
// On XP, symlink resolution should be disabled.
extern "C"
DWORD WINAPI GetFinalPathNameByHandleW(
  _In_  HANDLE hFile,
  _Out_ LPWSTR lpszFilePath,
  _In_  DWORD  cchFilePath,
  _In_  DWORD  dwFlags
);

namespace LibRpBase { namespace FileSystem {

// pthread_once() control variable.
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

// Configuration directories.

// User's cache directory.
static string cache_dir;
// User's configuration directory.
static string config_dir;

/**
 * Prepend "\\\\?\\" to an absolute Windows path.
 * This is needed in order to support filenames longer than MAX_PATH.
 * @param filename Original Windows filename.
 * @return Windows filename with "\\\\?\\" prepended.
 */
static inline wstring makeWinPath(const char *filename)
{
	if (unlikely(!filename || filename[0] == 0))
		return wstring();

	wstring filenameW;
	if (isascii(filename[0]) && isalpha(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += RP2W_c(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_c(filename);
	}
	return filenameW;
}

/**
 * Prepend "\\\\?\\" to an absolute Windows path.
 * This is needed in order to support filenames longer than MAX_PATH.
 * @param filename Original Windows filename.
 * @return Windows filename with "\\\\?\\" prepended.
 */
static inline wstring makeWinPath(const string &filename)
{
	if (filename.empty())
		return wstring();

	wstring filenameW;
	if (isascii(filename[0]) && isalpha(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += RP2W_s(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_s(filename);
	}
	return filenameW;
}

/**
 * Recursively mkdir() subdirectories.
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
int rmkdir(const string &path)
{
	// Windows uses UTF-16 natively, so handle as UTF-16.
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else
#error Win32 must have a 16-bit wchar_t.
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif

	// TODO: makeWinPath()?
	wstring path16 = RP2W_s(path);
	if (path16.size() == 3) {
		// 3 characters. Root directory is always present.
		return 0;
	} else if (path16.size() < 3) {
		// Less than 3 characters. Path isn't valid.
		return -EINVAL;
	}

	// Find all backslashes and ensure the directory component exists.
	// (Skip the drive letter and root backslash.)
	size_t slash_pos = 4;
	while ((slash_pos = path16.find((char16_t)DIR_SEP_CHR, slash_pos)) != string::npos) {
		// Temporarily NULL out this slash.
		path16[slash_pos] = 0;

		// Attempt to create this directory.
		if (::_wmkdir(path16.c_str()) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		path16[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
	}

	// rmkdir() succeeded.
	return 0;
}

/**
 * Does a file exist?
 * @param pathname Pathname.
 * @param mode Mode.
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const string &pathname, int mode)
{
	// Windows doesn't recognize X_OK.
	const wstring pathnameW = makeWinPath(pathname);
	mode &= ~X_OK;
	return ::_waccess(pathnameW.c_str(), mode);
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const string &filename)
{
	const wstring filenameW = makeWinPath(filename);
	struct _stati64 buf;
	int ret = _wstati64(filenameW.c_str(), &buf);

	if (ret != 0) {
		// stat() failed.
		ret = -errno;
		if (ret == 0) {
			// Something happened...
			ret = -EINVAL;
		}

		return ret;
	}

	// Return the file size.
	return buf.st_size;
}

/**
 * Initialize the configuration directory paths.
 * Called by pthread_once().
 */
static void initConfigDirectories(void)
{
	wchar_t path[MAX_PATH];
	HRESULT hr;

	/** Cache directory. **/

	// Windows: Get CSIDL_LOCAL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Local Settings\Application Data
	// - Windows Vista: C:\Users\username\AppData\Local
	hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		cache_dir = W2U8(path);
		if (!cache_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (cache_dir.at(cache_dir.size()-1) != '\\') {
				cache_dir += '\\';
			}

			// Append "rom-properties\\cache".
			cache_dir += "rom-properties\\cache";
		}
	}

	/** Configuration directory. **/

	// Windows: Get CSIDL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Application Data
	// - Windows Vista: C:\Users\username\AppData\Roaming
	hr = SHGetFolderPath(nullptr, CSIDL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		config_dir = W2U8(path);
		if (!config_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (config_dir.at(config_dir.size()-1) != '\\') {
				config_dir += '\\';
			}

			// Append "rom-properties".
			config_dir += "rom-properties";
		}
	}
}

/**
 * Get the user's cache directory.
 * This is usually one of the following:
 * - Windows XP: %APPDATA%\Local Settings\rom-properties\cache
 * - Windows Vista: %LOCALAPPDATA%\rom-properties\cache
 * - Linux: ~/.cache/rom-properties
 *
 * @return User's rom-properties cache directory, or empty string on error.
 */
const string &getCacheDirectory(void)
{
	// TODO: Handle errors.
	pthread_once(&once_control, initConfigDirectories);
	return cache_dir;
}

/**
 * Get the user's rom-properties configuration directory.
 * This is usually one of the following:
 * - Windows: %APPDATA%\rom-properties
 * - Linux: ~/.config/rom-properties
 *
 * @return User's rom-properties configuration directory, or empty string on error.
 */
const string &getConfigDirectory(void)
{
	// TODO: Handle errors.
	pthread_once(&once_control, initConfigDirectories);
	return config_dir;
}

/**
 * Set the modification timestamp of a file.
 * @param filename Filename.
 * @param mtime Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const string &filename, time_t mtime)
{
	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#error 32-bit time_t is not supported. Get a newer compiler.
#endif
	const wstring filenameW = makeWinPath(filename);

	struct __utimbuf64 utbuf;
	utbuf.actime = _time64(nullptr);
	utbuf.modtime = mtime;
	int ret = _wutime64(filenameW.c_str(), &utbuf);

	return (ret == 0 ? 0 : -errno);
}

/**
 * Get the modification timestamp of a file.
 * @param filename Filename.
 * @param pMtime Buffer for the modification timestamp.
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const string &filename, time_t *pMtime)
{
	if (!pMtime) {
		return -EINVAL;
	}
	const wstring filenameW = makeWinPath(filename);

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#error 32-bit time_t is not supported. Get a newer compiler.
#endif
	// Use GetFileTime() instead of _stati64().
	HANDLE hFile = CreateFile(filenameW.c_str(),
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile) {
		// Error opening the file.
		return -w32err_to_posix(GetLastError());
	}

	FILETIME mtime;
	BOOL bRet = GetFileTime(hFile, nullptr, nullptr, &mtime);
	CloseHandle(hFile);
	if (!bRet) {
		// Error getting the file time.
		return -w32err_to_posix(GetLastError());
	}

	// Convert to Unix timestamp.
	*pMtime = FileTimeToUnixTime(&mtime);
	return 0;
}

/**
 * Delete a file.
 * @param filename Filename.
 * @return 0 on success; negative POSIX error code on error.
 */
int delete_file(const char *filename)
{
	if (unlikely(!filename || filename[0] == 0))
		return -EINVAL;
	int ret = 0;
	const wstring filenameW = makeWinPath(filename);

	BOOL bRet = DeleteFile(filenameW.c_str());
	if (!bRet) {
		// Error deleting file.
		ret = -w32err_to_posix(GetLastError());
	}

	return ret;
}

/**
 * Check if the specified file is a symbolic link.
 * @return True if the file is a symbolic link; false if not.
 */
bool is_symlink(const char *filename)
{
	if (unlikely(!filename || filename[0] == 0))
		return false;
	const wstring filenameW = makeWinPath(filename);

	// Check the reparse point type.
	// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20100212-00/?p=14963
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(filenameW.c_str(), &findFileData);
	if (!hFind || hFind == INVALID_HANDLE_VALUE) {
		// Cannot find the file.
		return false;
	}
	FindClose(hFind);

	if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		// This is a reparse point.
		return (findFileData.dwReserved0 == IO_REPARSE_TAG_SYMLINK);
	}

	// Not a reparse point.
	return false;
}

// GetFinalPathnameByHandleW() lookup.
static pthread_once_t once_gfpbhw = PTHREAD_ONCE_INIT;
typedef DWORD (WINAPI *PFNGETFINALPATHNAMEBYHANDLEW)(
	_In_  HANDLE hFile,
	_Out_ LPWSTR lpszFilePath,
	_In_  DWORD  cchFilePath,
	_In_  DWORD  dwFlags
);
static PFNGETFINALPATHNAMEBYHANDLEW pfnGetFinalPathnameByHandleW = nullptr;

/**
 * Look up GetFinalPathnameByHandleW().
 */
static void LookupGetFinalPathnameByHandleW(void)
{
	HMODULE hKernel32 = GetModuleHandle(L"kernel32");
	if (hKernel32) {
		pfnGetFinalPathnameByHandleW = (PFNGETFINALPATHNAMEBYHANDLEW)
			GetProcAddress(hKernel32, "GetFinalPathNameByHandleW");
	}
}

/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of symbolic link.
 * @return Resolved symbolic link, or empty string on error.
 */
string resolve_symlink(const char *filename)
{
	if (unlikely(!filename || filename[0] == 0))
		return string();

	pthread_once(&once_gfpbhw, LookupGetFinalPathnameByHandleW);
	if (!pfnGetFinalPathnameByHandleW) {
		// GetFinalPathnameByHandleW() not available.
		return string();
	}

	// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20100212-00/?p=14963
	// TODO: Enable write sharing in regular IRpFile?
	HANDLE hFile = CreateFile(RP2W_c(filename),
		GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) {
		// Unable to open the file.
		return string();
	}

	// NOTE: GetFinalPathNameByHandle() always returns "\\\\?\\" paths.
	DWORD cchDeref = pfnGetFinalPathnameByHandleW(hFile, nullptr, 0, VOLUME_NAME_DOS);
	if (cchDeref == 0) {
		// Error...
		CloseHandle(hFile);
		return string();
	}

	wchar_t *szDeref = new wchar_t[cchDeref+1];
	pfnGetFinalPathnameByHandleW(hFile, szDeref, cchDeref+1, VOLUME_NAME_DOS);
	string ret = W2U8(szDeref, cchDeref);
	delete[] szDeref;
	CloseHandle(hFile);
	return ret;
}

} }

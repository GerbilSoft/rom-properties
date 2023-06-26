/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * FileSystem_win32.cpp: File system functions. (Win32 implementation)     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "../FileSystem.hpp"

// librpthreads
#include "librpthreads/pthread_once.h"

// C includes
#include <sys/stat.h>
#include <sys/utime.h>

// DT_* enumeration
#include "d_type.h"

// C++ STL classes
using std::string;
using std::u16string;
using std::wstring;

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.hpp"
#include "libwin32common/w32time.h"

// librptext
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"

// Windows includes.
#include <direct.h>

namespace LibRpFile { namespace FileSystem {

#ifdef UNICODE
/**
 * Prepend "\\\\?\\" to an absolute Windows path.
 * This is needed in order to support filenames longer than MAX_PATH.
 * @param filename Original Windows filename.
 * @return Windows filename with "\\\\?\\" prepended.
 */
static inline wstring makeWinPath(const char *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return wstring();
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (ISASCII(filename[0]) && ISALPHA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\\\?\\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += U82W_c(filename);
	} else {
		// Not an absolute path, or "\\\\?\\" is already
		// prepended. Use it as-is.
		filenameW = U82W_c(filename);
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
	assert(!filename.empty());
	if (unlikely(filename.empty())) {
		return wstring();
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (ISASCII(filename[0]) && ISALPHA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += U82W_s(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = U82W_s(filename);
	}
	return filenameW;
}

/**
 * Prepend "\\\\?\\" to an absolute Windows path.
 * This is needed in order to support filenames longer than MAX_PATH.
 * @param filename Original Windows filename.
 * @return Windows filename with "\\\\?\\" prepended.
 */
static inline wstring makeWinPath(const wchar_t *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == L'\0')) {
		return wstring();
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (ISASCII(filename[0]) && ISALPHA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\\\?\\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += filename;
	} else {
		// Not an absolute path, or "\\\\?\\" is already
		// prepended. Use it as-is.
		filenameW = filename;
	}
	return filenameW;
}

/**
 * Prepend "\\\\?\\" to an absolute Windows path.
 * This is needed in order to support filenames longer than MAX_PATH.
 * @param filename Original Windows filename.
 * @return Windows filename with "\\\\?\\" prepended.
 */
static inline wstring makeWinPath(const wstring &filename)
{
	assert(!filename.empty());
	if (unlikely(filename.empty())) {
		return wstring();
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (ISASCII(filename[0]) && ISALPHA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += filename;
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = filename;
	}
	return filenameW;
}
#else /* !UNICODE */
/**
 * Convert a path from ANSI to UTF-8.
 *
 * Windows' ANSI functions doesn't support the use of
 * "\\\\?\\" for paths longer than MAX_PATH.
 *
 * @param filename UTF-8 filename.
 * @return ANSI filename.
 */
static inline tstring makeWinPath(const char *filename)
{
	return U82T_c(filename);
}

/**
 * Convert a path from ANSI to UTF-8.
 *
 * Windows' ANSI functions doesn't support the use of
 * "\\\\?\\" for paths longer than MAX_PATH.
 *
 * @param filename UTF-8 filename.
 * @return ANSI filename.
 */
static inline tstring makeWinPath(const string &filename)
{
	return U82T_s(filename);
}

/**
 * Convert a path from ANSI to UTF-8.
 *
 * Windows' ANSI functions doesn't support the use of
 * "\\\\?\\" for paths longer than MAX_PATH.
 *
 * @param filename UTF-8 filename.
 * @return ANSI filename.
 */
static inline tstring makeWinPath(const wchar_t *filename)
{
	return W2U8(filename);
}

/**
 * Convert a path from ANSI to UTF-8.
 *
 * Windows' ANSI functions doesn't support the use of
 * "\\\\?\\" for paths longer than MAX_PATH.
 *
 * @param filename UTF-8 filename.
 * @return ANSI filename.
 */
static inline tstring makeWinPath(const wstring &filename)
{
	return W2U8(filename);
}
#endif /* UNICODE */

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
	// Windows uses UTF-16 natively, so handle it as UTF-16.
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t is not 16-bit!");

	// TODO: makeWinPath()?
	tstring tpath = U82T_s(path);

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
	while ((slash_pos = tpath.find(static_cast<char16_t>(DIR_SEP_CHR), slash_pos)) != string::npos) {
		// Temporarily NULL out this slash.
		tpath[slash_pos] = _T('\0');

		// Attempt to create this directory.
		if (::_tmkdir(tpath.c_str()) != 0) {
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
 * Does a file exist?
 * @param pathname Pathname
 * @param mode Mode
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const char *pathname, int mode)
{
	// Windows doesn't recognize X_OK.
	const tstring tpathname = makeWinPath(pathname);
	mode &= ~X_OK;
	return ::_taccess(tpathname.c_str(), mode);
}

/**
 * Does a file exist?
 * @param pathname Pathname
 * @param mode Mode
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int waccess(const wchar_t *pathname, int mode)
{
	// Windows doesn't recognize X_OK.
	const tstring tpathname = makeWinPath(pathname);
	mode &= ~X_OK;
	return ::_taccess(tpathname.c_str(), mode);
}

/**
 * Get a file's size. (internal function)
 * @param tfilename Filename
 * @return Size on success; -1 on error.
 */
static off64_t filesize_int(const tstring &tfilename)
{
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
# error 32-bit time_t is not supported. Get a newer compiler.
#endif
	// Use GetFileSize() instead of _stati64().
	HANDLE hFile = CreateFile(tfilename.c_str(),
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile) {
		// Error opening the file.
		return -w32err_to_posix(GetLastError());
	}

	LARGE_INTEGER liFileSize;
	BOOL bRet = GetFileSizeEx(hFile, &liFileSize);
	CloseHandle(hFile);
	if (!bRet) {
		// Error getting the file size.
		return -w32err_to_posix(GetLastError());
	}

	// Return the file size.
	return liFileSize.QuadPart;
}

/**
 * Get a file's size.
 * @param filename Filename
 * @return Size on success; -1 on error.
 */
off64_t filesize(const char *filename)
{
	return filesize_int(makeWinPath(filename));
}

/**
 * Get a file's size.
 * @param filename Filename
 * @return Size on success; -1 on error.
 */
off64_t wfilesize(const wchar_t *filename)
{
	return filesize_int(makeWinPath(filename));
}

/**
 * Set the modification timestamp of a file.
 * @param filename Filename.
 * @param mtime Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const string &filename, time_t mtime)
{
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
# error 32-bit time_t is not supported. Get a newer compiler.
#endif
	const tstring tfilename = makeWinPath(filename);

	struct __utimbuf64 utbuf;
	utbuf.actime = _time64(nullptr);
	utbuf.modtime = mtime;
	int ret = _tutime64(tfilename.c_str(), &utbuf);

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
	assert(pMtime != nullptr);
	if (!pMtime) {
		return -EINVAL;
	}

	const tstring tfilename = makeWinPath(filename);

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
# error 32-bit time_t is not supported. Get a newer compiler.
#endif
	// Use GetFileTime() instead of _stati64().
	HANDLE hFile = CreateFile(tfilename.c_str(),
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
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return -EINVAL;
	}

	int ret = 0;
	const tstring tfilename = makeWinPath(filename);
	if (!DeleteFile(tfilename.c_str())) {
		// Error deleting file.
		ret = -w32err_to_posix(GetLastError());
	}

	return ret;
}

/**
 * Check if the specified file is a symbolic link.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @return True if the file is a symbolic link; false if not.
 */
bool is_symlink(const char *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return false;
	}
	const tstring tfilename = makeWinPath(filename);

	// Check the reparse point type.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20100212-00/?p=14963
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(tfilename.c_str(), &findFileData);
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
static pthread_once_t once_gfpbh = PTHREAD_ONCE_INIT;
typedef DWORD (WINAPI *PFNGETFINALPATHNAMEBYHANDLEA)(
	_In_  HANDLE hFile,
	_Out_ LPSTR lpszFilePath,
	_In_  DWORD  cchFilePath,
	_In_  DWORD  dwFlags
);
typedef DWORD (WINAPI *PFNGETFINALPATHNAMEBYHANDLEW)(
	_In_  HANDLE hFile,
	_Out_ LPWSTR lpszFilePath,
	_In_  DWORD  cchFilePath,
	_In_  DWORD  dwFlags
);
#ifdef UNICODE
#  define PFNGETFINALPATHNAMEBYHANDLE PFNGETFINALPATHNAMEBYHANDLEW
#  define GETFINALPATHNAMEBYHANDLE_FN "GetFinalPathNameByHandleW"
#else /* !UNICODE */
#  define PFNGETFINALPATHNAMEBYHANDLE PFNGETFINALPATHNAMEBYHANDLEA
#  define GETFINALPATHNAMEBYHANDLE_FN "GetFinalPathNameByHandleA"
#endif /* UNICODE */
static PFNGETFINALPATHNAMEBYHANDLE pfnGetFinalPathnameByHandle = nullptr;

/**
 * Look up GetFinalPathnameByHandleW().
 */
static void LookupGetFinalPathnameByHandle(void)
{
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
	if (hKernel32) {
		pfnGetFinalPathnameByHandle = reinterpret_cast<PFNGETFINALPATHNAMEBYHANDLE>(
			GetProcAddress(hKernel32, GETFINALPATHNAMEBYHANDLE_FN));
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
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return string();
	}

	pthread_once(&once_gfpbh, LookupGetFinalPathnameByHandle);
	if (!pfnGetFinalPathnameByHandle) {
		// GetFinalPathnameByHandle() not available.
		return string();
	}

	// Reference: https://devblogs.microsoft.com/oldnewthing/20100212-00/?p=14963
	// TODO: Enable write sharing in regular IRpFile?
	const tstring tfilename = makeWinPath(filename);
	HANDLE hFile = CreateFile(tfilename.c_str(),
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
	DWORD cchDeref = pfnGetFinalPathnameByHandle(hFile, nullptr, 0, VOLUME_NAME_DOS);
	if (cchDeref == 0) {
		// Error...
		CloseHandle(hFile);
		return string();
	}

	// NOTE: cchDeref may include the NULL terminator on ANSI systems.
	// We'll add one anyway, just in case it doesn't.
	TCHAR *szDeref = new TCHAR[cchDeref+1];
	pfnGetFinalPathnameByHandle(hFile, szDeref, cchDeref+1, VOLUME_NAME_DOS);
	if (szDeref[cchDeref-1] == '\0') {
		// Extra NULL terminator found.
		cchDeref--;
	}

	string ret = T2U8(szDeref, cchDeref);
	delete[] szDeref;
	CloseHandle(hFile);
	return ret;
}

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @return True if the file is a directory; false if not.
 */
bool is_directory(const char *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return false;
	}
	const tstring tfilename = makeWinPath(filename);

	const DWORD attrs = GetFileAttributes(tfilename.c_str());
	return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

/**
 * Is a file located on a "bad" file system?
 *
 * We don't want to check files on e.g. procfs,
 * or on network file systems if the option is disabled.
 *
 * @param filename Filename.
 * @param netFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
bool isOnBadFS(const char *filename, bool netFS)
{
	// TODO: More comprehensive check.
	// For now, merely checking if it starts with "\\\\"
	// and the third character is not '?' or '.'.
	if (filename[0] == '\\' && filename[1] == '\\' &&
	    filename[2] != '\0' && filename[2] != '?' && filename[2] != '.')
	{
		// This file is located on a network share.
		return !netFS;
	}

	// Not on a network share.
	return false;
}

/**
 * Get a file's size and mtime.
 * @param filename	[in] Filename.
 * @param pFileSize	[out] File size.
 * @param pMtime	[out] Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const string &filename, off64_t *pFileSize, time_t *pMtime)
{
	assert(!filename.empty());
	assert(pFileSize != nullptr);
	assert(pMtime != nullptr);
	if (unlikely(filename.empty() || !pFileSize || !pMtime)) {
		return -EINVAL;
	}
	const tstring tfilename = makeWinPath(filename);

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
# error 32-bit time_t is not supported. Get a newer compiler.
#endif

	// Use FindFirstFile() to get the file information.
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(tfilename.c_str(), &ffd);
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

	// We're done here.
	return 0;
}

/**
 * Convert Win32 attributes to d_type.
 * @param dwAttrs Win32 attributes (NOTE: Can't use DWORD here)
 * @return d_type, or DT_UNKNOWN on error.
 */
uint8_t win32_attrs_to_d_type(uint32_t dwAttrs)
{
	if (dwAttrs == INVALID_FILE_ATTRIBUTES)
		return DT_UNKNOWN;

	uint8_t d_type;

	if (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) {
		d_type = DT_DIR;
	} else if (dwAttrs & FILE_ATTRIBUTE_REPARSE_POINT) {
		// TODO: Check WIN32_FIND_DATA::dwReserved0 for IO_REPARSE_TAG_SYMLINK.
		d_type = DT_LNK;
	} else if (dwAttrs & FILE_ATTRIBUTE_DEVICE) {
		// TODO: Is this correct?
		d_type = DT_CHR;
	} else {
		d_type = DT_REG;
	}

	return d_type;
}

/**
 * Get a file's d_type.
 * @param filename Filename
 * @param deref If true, dereference symbolic links (lstat)
 * @return File d_type
 */
uint8_t get_file_d_type(const char *filename, bool deref)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return DT_UNKNOWN;
	}

	// TODO: Handle dereferencing.
	RP_UNUSED(deref);

	const tstring tfilename = makeWinPath(filename);
	return win32_attrs_to_d_type(GetFileAttributes(tfilename.c_str()));
}

} }

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
using std::unique_ptr;
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

static inline constexpr bool IsDriveLetterA(char letter)
{
	return (letter >= 'A') && (letter <= 'Z');
}
static inline constexpr bool IsDriveLetterW(wchar_t letter)
{
	return (letter >= L'A') && (letter <= L'Z');
}
#ifdef _UNICODE
#  define IsDriveLetter(x) IsDriveLetterW(x)
#else /* !_UNICODE */
#  define IsDriveLetter(x) IsDriveLetterA(x)
#endif /* _UNICODE */

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
		return {};
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (IsDriveLetterA(filename[0]) &&
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
		return {};
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (IsDriveLetterA(filename[0]) &&
	    filename[1] == ':' && filename[2] == '\\')
	{
		// Absolute path. Prepend "\\\\?\\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += U82W_s(filename);
	} else {
		// Not an absolute path, or "\\\\?\\" is already
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
		return {};
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (IsDriveLetterW(filename[0]) &&
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
		return {};
	}

	// TODO: Don't bother if the filename is <= 240 characters?
	wstring filenameW;
	if (IsDriveLetterW(filename[0]) &&
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
 * @param path Path to recursively mkdir (last component is ignored)
 * @param mode File mode (defaults to 0777; ignored on Windows)
 * @return 0 on success; negative POSIX error code on error.
 */
int rmkdir(const string &path, int mode)
{
	// Windows uses UTF-16 natively, so handle it as UTF-16.
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t is not 16-bit!");
	RP_UNUSED(mode);

	// NOTE: Explicitly calling U82W_s(), not U82T_s(), to prevent
	// infinite recursion in ANSI builds.
	// NOTE 2: This probably won't work in ANSI builds anyway...
	// TODO: makeWinPath()?
	return rmkdir(U82W_s(path));
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
 * @param path Path to recursively mkdir (last component is ignored)
 * @param mode File mode (defaults to 0777; ignored on Windows)
 * @return 0 on success; non-zero on error.
 */
RP_LIBROMDATA_PUBLIC
int rmkdir(const wstring &path, int mode)
{
	// Windows uses UTF-16 natively, so handle it as UTF-16.
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t is not 16-bit!");
	RP_UNUSED(mode);

	// TODO: makeWinPath()? [and ANSI handling, or not...]
	wstring wpath = path;

	if (wpath.size() == 3) {
		// 3 characters. Root directory is always present.
		return 0;
	} else if (wpath.size() < 3) {
		// Less than 3 characters. Path isn't valid.
		return -EINVAL;
	}

	// Find all backslashes and ensure the directory component exists.
	// (Skip the drive letter and root backslash.)
	size_t slash_pos = 4;
	while ((slash_pos = wpath.find(static_cast<char16_t>(DIR_SEP_CHR), slash_pos)) != wstring::npos) {
		// Temporarily NULL out this slash.
		wpath[slash_pos] = _T('\0');

		// Attempt to create this directory.
		// FIXME: This won't work on ANSI...
		if (::_wmkdir(wpath.c_str()) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		wpath[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
	}

	// rmkdir() succeeded.
	return 0;
}

/**
 * Does a file exist?
 * @param pathname Pathname (UTF-8)
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
 * @param pathname Pathname (UTF-16)
 * @param mode Mode
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const wchar_t *pathname, int mode)
{
	// Windows doesn't recognize X_OK.
	const tstring tpathname = makeWinPath(pathname);
	mode &= ~X_OK;
	return ::_taccess(tpathname.c_str(), mode);
}

/**
 * Get a file's size. (internal function)
 * @param tfilename Filename (TCHAR)
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
		GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
 * @param filename Filename (UTF-8)
 * @return Size on success; -1 on error.
 */
off64_t filesize(const char *filename)
{
	return filesize_int(makeWinPath(filename));
}

/**
 * Get a file's size.
 * @param filename Filename (UTF-16)
 * @return Size on success; -1 on error.
 */
off64_t filesize(const wchar_t *filename)
{
	return filesize_int(makeWinPath(filename));
}

/**
 * Set the modification timestamp of a file.
 * @param tfilename	[in] Filename (TCHAR)
 * @param mtime		[in] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static int set_mtime_int(const tstring &tfilename, time_t mtime)
{
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#  error 32-bit time_t is not supported. Get a newer compiler.
#endif

	// TODO: Use Win32 API directly instead of MSVCRT?
	struct __utimbuf64 utbuf;
	utbuf.actime = _time64(nullptr);
	utbuf.modtime = mtime;
	int ret = _tutime64(tfilename.c_str(), &utbuf);

	return (ret == 0 ? 0 : -errno);
}

/**
 * Set the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-8)
 * @param mtime		[in] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const char *filename, time_t mtime)
{
	return set_mtime_int(makeWinPath(filename), mtime);
}

/**
 * Set the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-16)
 * @param mtime		[in] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const wchar_t *filename, time_t mtime)
{
	return set_mtime_int(makeWinPath(filename), mtime);
}

/**
 * Get the modification timestamp of a file. (internal function)
 * @param tfilename	[in] Filename (TCHAR)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static int get_mtime_int(const tstring &tfilename, time_t *pMtime)
{
	assert(pMtime != nullptr);
	if (!pMtime)
		return -EINVAL;

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#  error 32-bit time_t is not supported. Get a newer compiler.
#endif
	// Use GetFileTime() instead of _stati64().
	HANDLE hFile = CreateFile(tfilename.c_str(),
		GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-8)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const char *filename, time_t *pMtime)
{
	return get_mtime_int(makeWinPath(filename), pMtime);
}

/**
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-16)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const wchar_t *filename, time_t *pMtime)
{
	return get_mtime_int(makeWinPath(filename), pMtime);
}

/**
 * Delete a file.
 * @param filename Filename (UTF-8)
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
 * Internal function; has common code for after filename parsing.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @param tfilename Filename (TCHAR; makeWinPath() must have already been called)
 * @return True if the file is a symbolic link; false if not.
 */
static bool is_symlink_int(const TCHAR *tfilename)
{
	// Check the reparse point type.
	// Reference: https://devblogs.microsoft.com/oldnewthing/20100212-00/?p=14963
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(tfilename, &findFileData);
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

/**
 * Check if the specified file is a symbolic link.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @param filename Filename (UTF-8)
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
	return is_symlink_int(tfilename.c_str());
}

/**
 * Check if the specified file is a symbolic link.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @param filename Filename (UTF-16)
 * @return True if the file is a symbolic link; false if not.
 */
bool is_symlink(const wchar_t *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != L'\0');
	if (unlikely(!filename || filename[0] == L'\0')) {
		return false;
	}
	const tstring tfilename = makeWinPath(filename);
	return is_symlink_int(tfilename.c_str());
}

// GetFinalPathnameByHandle() lookup.
static pthread_once_t once_gfpbh = PTHREAD_ONCE_INIT;
typedef DWORD (WINAPI *pfnGetFinalPathNameByHandleA_t)(
	_In_  HANDLE hFile,
	_Out_ LPSTR lpszFilePath,
	_In_  DWORD  cchFilePath,
	_In_  DWORD  dwFlags
);
typedef DWORD (WINAPI *pfnGetFinalPathNameByHandleW_t)(
	_In_  HANDLE hFile,
	_Out_ LPWSTR lpszFilePath,
	_In_  DWORD  cchFilePath,
	_In_  DWORD  dwFlags
);
#ifdef UNICODE
using pfnGetFinalPathNameByHandle_t = pfnGetFinalPathNameByHandleW_t;
static constexpr char GetFinalPathNameByHandle_fn[] = "GetFinalPathNameByHandleW";
#else /* !UNICODE */
using pfnGetFinalPathNameByHandle_t = pfnGetFinalPathNameByHandleA_t;
static constexpr char GetFinalPathNameByHandle_fn[] = "GetFinalPathNameByHandleA";
#endif /* UNICODE */
static pfnGetFinalPathNameByHandle_t pfnGetFinalPathnameByHandle = nullptr;

/**
 * Look up GetFinalPathnameByHandleW().
 */
static void LookupGetFinalPathnameByHandle(void)
{
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	if (hKernel32) {
		pfnGetFinalPathnameByHandle = reinterpret_cast<pfnGetFinalPathNameByHandle_t>(
			GetProcAddress(hKernel32, GetFinalPathNameByHandle_fn));
	}
}

/**
 * Resolve a symbolic link.
 * Internal function; has common code for after filename parsing.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param tfilename Filename of symbolic link (UTF-16; makeWinPath() must have been called)
 * @return Resolved symbolic link, or empty string on error.
 */
static tstring resolve_symlink_int(const TCHAR *tfilename)
{
	pthread_once(&once_gfpbh, LookupGetFinalPathnameByHandle);
	if (!pfnGetFinalPathnameByHandle) {
		// GetFinalPathnameByHandle() not available.
		return {};
	}

	// Reference: https://devblogs.microsoft.com/oldnewthing/20100212-00/?p=14963
	// TODO: Enable write sharing in regular IRpFile?
	HANDLE hFile = CreateFile(tfilename,
		GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (!hFile || hFile == INVALID_HANDLE_VALUE) {
		// Unable to open the file.
		return {};
	}

	// NOTE: GetFinalPathNameByHandle() always returns "\\\\?\\" paths.
	DWORD cchDeref = pfnGetFinalPathnameByHandle(hFile, nullptr, 0, VOLUME_NAME_DOS);
	if (cchDeref == 0) {
		// Error...
		CloseHandle(hFile);
		return {};
	}

	// NOTE: cchDeref may include the NULL terminator on ANSI systems.
	// We'll add one anyway, just in case it doesn't.
	// TODO: Allocate std::wstring() here and read directly into data()?
	unique_ptr<TCHAR[]> szDeref(new TCHAR[cchDeref+1]);
	pfnGetFinalPathnameByHandle(hFile, szDeref.get(), cchDeref+1, VOLUME_NAME_DOS);
	if (szDeref[cchDeref-1] == '\0') {
		// Extra NULL terminator found.
		cchDeref--;
	}

	CloseHandle(hFile);
	return tstring(szDeref.get(), cchDeref);
}

/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of symbolic link (UTF-8)
 * @return Resolved symbolic link, or empty string on error.
 */
string resolve_symlink(const char *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == '\0')) {
		return {};
	}
	const tstring tfilename = makeWinPath(filename);
#ifdef UNICODE
	return W2U8(resolve_symlink_int(tfilename.c_str()));
#else /* !UNICODE */
	return resolve_symlink_int(tfilename.c_str());
#endif /* UNICODE */
}

/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of symbolic link (UTF-16)
 * @return Resolved symbolic link, or empty string on error.
 */
wstring resolve_symlink(const wchar_t *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != L'\0');
	if (unlikely(!filename || filename[0] == L'\0')) {
		return {};
	}
	const tstring tfilename = makeWinPath(filename);
#ifdef UNICODE
	return resolve_symlink_int(tfilename.c_str());
#else /* !UNICODE */
	return A2W_s(resolve_symlink_int(tfilename.c_str()));
#endif /* UNICODE */
}

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-8)
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
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-16)
 * @return True if the file is a directory; false if not.
 */
bool is_directory(const wchar_t *filename)
{
	assert(filename != nullptr);
	assert(filename[0] != '\0');
	if (unlikely(!filename || filename[0] == L'\0')) {
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
 * @tparam CharType Character type (char for UTF-8; wchar_t for Windows UTF-16)
 * @param filename Filename
 * @param allowNetFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
template<typename CharType>
static inline bool T_isOnBadFS(const CharType *filename, bool allowNetFS)
{
	// TODO: More comprehensive check.
	// For now, merely checking if it starts with "\\\\"
	// and the third character is not '?' or '.'.
	if (filename[0] == '\\' && filename[1] == '\\' &&
	    filename[2] != '\0' && filename[2] != '?' && filename[2] != '.')
	{
		// This file is located on a network share.
		return !allowNetFS;
	}

	// Not on a network share.
	return false;
}

/**
 * Is a file located on a "bad" file system?
 *
 * We don't want to check files on e.g. procfs,
 * or on network file systems if the option is disabled.
 *
 * @param filename Filename (UTF-8)
 * @param allowNetFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
bool isOnBadFS(const char *filename, bool allowNetFS)
{
	return T_isOnBadFS(filename, allowNetFS);
}

/**
 * Is a file located on a "bad" file system?
 *
 * We don't want to check files on e.g. procfs,
 * or on network file systems if the option is disabled.
 *
 * @param filename Filename (UTF-16)
 * @param allowNetFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
bool isOnBadFS(const wchar_t *filename, bool allowNetFS)
{
	return T_isOnBadFS(filename, allowNetFS);
}

/**
 * Get a file's size and time. (internal function)
 * @param tfilename	[in] Filename (TCHAR)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static int get_file_size_and_mtime_int(const tstring &tfilename, off64_t *pFileSize, time_t *pMtime)
{
	assert(pFileSize != nullptr);
	assert(pMtime != nullptr);
	if (unlikely(!pFileSize || !pMtime)) {
		return -EINVAL;
	}

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#  error 32-bit time_t is not supported. Get a newer compiler.
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
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-8)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const char *filename, off64_t *pFileSize, time_t *pMtime)
{
	return get_file_size_and_mtime_int(makeWinPath(filename), pFileSize, pMtime);
}

/**
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-16)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const wchar_t *filename, off64_t *pFileSize, time_t *pMtime)
{
	return get_file_size_and_mtime_int(makeWinPath(filename), pFileSize, pMtime);
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

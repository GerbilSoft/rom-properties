/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * FileSystem.hpp: File system functions.                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <cstdint>
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// access() macros.
#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

// Windows doesn't define X_OK, W_OK, or R_OK.
// NOTE: F_OK doesn't work properly on Windows.
#ifndef F_OK
#  define F_OK 0
#endif
#ifndef X_OK
#  define X_OK 1
#endif
#ifndef W_OK
#  define W_OK 2
#endif
#ifndef R_OK
#  define R_OK 4
#endif

// Directory separator characters.
#include "tcharx.h"

// C++ includes.
#include <string>

namespace LibRpFile { namespace FileSystem {

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
int rmkdir(const std::string &path, int mode = 0777);

#if defined(_WIN32)
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
int rmkdir(const std::wstring &path, int mode = 0777);
#endif /* _WIN32 */

/**
 * Does a file exist?
 * @param pathname Pathname (UTF-8)
 * @param mode Mode
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
RP_LIBROMDATA_PUBLIC
int access(const char *pathname, int mode);

#ifdef _WIN32
/**
 * Does a file exist?
 * @param pathname Pathname (UTF-16)
 * @param mode Mode
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
RP_LIBROMDATA_PUBLIC
int access(const wchar_t *pathname, int mode);
#endif /* _WIN32 */

/**
 * Get a file's size.
 * @param filename Filename (UTF-8)
 * @return Size on success; -1 on error.
 */
off64_t filesize(const char *filename);

#ifdef _WIN32
/**
 * Get a file's size.
 * @param filename Filename (UTF-16)
 * @return Size on success; -1 on error.
 */
off64_t filesize(const wchar_t *filename);
#endif /* _WIN32 */

/**
 * Get the user's cache directory.
 * This is usually one of the following:
 * - Windows XP: %APPDATA%\Local Settings\rom-properties\cache
 * - Windows Vista: %LOCALAPPDATA%\rom-properties\cache
 * - Linux: ~/.cache/rom-properties
 *
 * @return User's rom-properties cache directory, or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
const std::string &getCacheDirectory(void);

/**
 * Get the user's rom-properties configuration directory.
 * This is usually one of the following:
 * - Windows: %APPDATA%\rom-properties
 * - Linux: ~/.config/rom-properties
 *
 * @return User's rom-properties configuration directory, or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
const std::string &getConfigDirectory(void);

/**
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-8)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const char *filename, time_t *pMtime);

/**
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-8)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int get_mtime(const std::string &filename, time_t *pMtime)
{
	return get_mtime(filename.c_str(), pMtime);
}

#ifdef _WIN32
/**
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-16)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const wchar_t *filename, time_t *pMtime);

/**
 * Get the modification timestamp of a file.
 * @param filename	[in] Filename (UTF-16)
 * @param pMtime	[out] Buffer for the modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int get_mtime(const std::wstring &filename, time_t *pMtime)
{
	return get_mtime(filename.c_str(), pMtime);
}
#endif /* _WIN32 */

/**
 * Delete a file.
 * @param filename Filename.
 * @return 0 on success; negative POSIX error code on error.
 */
int delete_file(const char *filename);

/**
 * Delete a file.
 * @param filename Filename.
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int delete_file(const std::string &filename)
{
	return delete_file(filename.c_str());
}

/**
 * Get the file extension from a filename or pathname.
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename (UTF-8)
 * @return File extension, including the leading dot; nullptr if no extension.
 */
RP_LIBROMDATA_PUBLIC
const char *file_ext(const char *filename);

/**
 * Get the file extension from a filename or pathname.
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename (UTF-8)
 * @return File extension, including the leading dot; nullptr if no extension.
 */
static inline const char *file_ext(const std::string &filename)
{
	return file_ext(filename.c_str());
}

#ifdef _WIN32
/**
 * Get the file extension from a filename or pathname. (wchar_t version)
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename (UTF-16)
 * @return File extension, including the leading dot; nullptr if no extension.
 */
RP_LIBROMDATA_PUBLIC
const wchar_t *file_ext(const wchar_t *filename);

/**
 * Get the file extension from a filename or pathname. (wchar_t version)
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename (UTF-16)
 * @return File extension, including the leading dot; nullptr if no extension.
 */
static inline const wchar_t *file_ext(const std::wstring &filename)
{
	return file_ext(filename.c_str());
}
#endif /* _WIN32 */

/**
 * Replace the file extension from a filename.
 * @param filename	[in] Filename.
 * @param ext		[in] New extension.
 * @return Filename, with replaced extension.
 */
RP_LIBROMDATA_PUBLIC
std::string replace_ext(const char *filename, const char *ext);

/**
 * Check if the specified file is a symbolic link.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @param filename Filename (UTF-8)
 * @return True if the file is a symbolic link; false if not.
 */
bool is_symlink(const char *filename);

#ifdef _WIN32
/**
 * Check if the specified file is a symbolic link.
 *
 * Symbolic links are NOT resolved; otherwise wouldn't check
 * if the specified file was a symlink itself.
 *
 * @param filename Filename (UTF-16)
 * @return True if the file is a symbolic link; false if not.
 */
bool is_symlink(const wchar_t *filename);
#endif /* _WIN32 */

/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of symbolic link (UTF-8)
 * @return Resolved symbolic link, or empty string on error.
 */
std::string resolve_symlink(const char *filename);

#ifdef _WIN32
/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of symbolic link (UTF-16)
 * @return Resolved symbolic link, or empty string on error.
 */
std::wstring resolve_symlink(const wchar_t *filename);
#endif /* _WIN32 */

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-8)
 * @return True if the file is a directory; false if not.
 */
RP_LIBROMDATA_PUBLIC
bool is_directory(const char *filename);

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-8)
 * @return True if the file is a directory; false if not.
 */
static inline bool is_directory(const std::string &filename)
{
	return is_directory(filename.c_str());
}

#ifdef _WIN32
/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-16)
 * @return True if the file is a directory; false if not.
 */
RP_LIBROMDATA_PUBLIC
bool is_directory(const wchar_t *filename);

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename to check (UTF-16)
 * @return True if the file is a directory; false if not.
 */
static inline bool is_directory(const std::wstring &filename)
{
	return is_directory(filename.c_str());
}
#endif /* _WIN32 */

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
RP_LIBROMDATA_PUBLIC
bool isOnBadFS(const char *filename, bool allowNetFS = false);

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
static inline bool isOnBadFS(const std::string &filename, bool allowNetFS = false)
{
	return isOnBadFS(filename.c_str(), allowNetFS);
}

#ifdef _WIN32
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
RP_LIBROMDATA_PUBLIC
bool isOnBadFS(const wchar_t *filename, bool allowNetFS = false);

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
static inline bool isOnBadFS(const std::wstring &filename, bool allowNetFS = false)
{
	return isOnBadFS(filename.c_str(), allowNetFS);
}
#endif /* _WIN32 */

/**
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-8)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const char *filename, off64_t *pFileSize, time_t *pMtime);

/**
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-8)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int get_file_size_and_mtime(const std::string &filename, off64_t *pFileSize, time_t *pMtime)
{
	return get_file_size_and_mtime(filename.c_str(), pFileSize, pMtime);
}

#ifdef _WIN32
/**
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-16)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const wchar_t *filename, off64_t *pFileSize, time_t *pMtime);

/**
 * Get a file's size and time.
 * @param filename	[in] Filename (UTF-16)
 * @param pFileSize	[out] File size
 * @param pMtime	[out] Modification time (UNIX timestamp)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int get_file_size_and_mtime(const std::wstring &filename, off64_t *pFileSize, time_t *pMtime)
{
	return get_file_size_and_mtime(filename.c_str(), pFileSize, pMtime);
}
#endif /* _WIN32 */

#ifdef _WIN32
/**
 * Convert Win32 attributes to d_type.
 * @param dwAttrs Win32 attributes (NOTE: Can't use DWORD here)
 * @return d_type, or DT_UNKNOWN on error.
 */
uint8_t win32_attrs_to_d_type(uint32_t dwAttrs);
#endif /* _WIN32 */

/**
 * Get a file's d_type.
 * @param filename Filename
 * @param deref If true, dereference symbolic links (lstat)
 * @return File d_type
 */
RP_LIBROMDATA_PUBLIC
uint8_t get_file_d_type(const char *filename, bool deref = false);

} }

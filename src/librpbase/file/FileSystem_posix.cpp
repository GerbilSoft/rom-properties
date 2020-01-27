/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * FileSystem_posix.cpp: File system functions. (POSIX implementation)     *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FileSystem.hpp"

// C includes.
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>

#ifdef __linux__
// TODO: Remove once /proc/mounts parsing is implemented.
# include <sys/vfs.h>
# include <linux/magic.h>
// from `man 2 fstatfs`, but not present in linux/magic.h on 4.14-r1
# ifndef MQUEUE_MAGIC
#  define MQUEUE_MAGIC 0x19800202
# endif /* MQUEUE_MAGIC */
# ifndef TRACEFS_MAGIC
#  define TRACEFS_MAGIC 0x74726163
# endif /* TRACEFS_MAGIC */
# ifndef CIFS_MAGIC_NUMBER
#  define CIFS_MAGIC_NUMBER 0xff534d42
# endif /* CIFS_MAGIC_NUMBER */
# ifndef COH_SUPER_MAGIC
#  define COH_SUPER_MAGIC 0x012ff7b7
# endif /* COH_SUPER_MAGIC */
# ifndef FUSE_SUPER_MAGIC
#  define FUSE_SUPER_MAGIC 0x65735546
# endif /* FUSE_SUPER_MAGIC */
# ifndef OCFS2_SUPER_MAGIC
#  define OCFS2_SUPER_MAGIC 0x7461636f
# endif /* OCFS2_SUPER_MAGIC */
#endif /* __linux__ */

// C++ STL classes.
using std::string;
using std::u16string;

namespace LibRpBase { namespace FileSystem {

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
	// Linux (and most other systems) use UTF-8 natively.
	string path8 = path;

	// Find all slashes and ensure the directory component exists.
	size_t slash_pos = path8.find(DIR_SEP_CHR, 0);
	if (slash_pos == 0) {
		// Root is always present.
		slash_pos = path8.find(DIR_SEP_CHR, 1);
	}

	while (slash_pos != string::npos) {
		// Temporarily NULL out this slash.
		path8[slash_pos] = 0;

		// Attempt to create this directory.
		if (::mkdir(path8.c_str(), 0777) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		path8[slash_pos] = DIR_SEP_CHR;
		slash_pos++;

		// Find the next slash.
		slash_pos = path8.find(DIR_SEP_CHR, slash_pos);
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
	return ::access(pathname.c_str(), mode);
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const string &filename)
{
	struct stat sb;
	int ret = stat(filename.c_str(), &sb);
	if (ret != 0) {
		// stat() failed.
		ret = -errno;
		if (ret == 0) {
			// Something happened...
			ret = -EIO;
		}

		return ret;
	}

	// Return the file size.
	return sb.st_size;
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
	struct utimbuf utbuf;
	utbuf.actime = time(nullptr);
	utbuf.modtime = mtime;
	int ret = utime(filename.c_str(), &utbuf);

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

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
	struct stat buf;
	int ret = stat(filename.c_str(), &buf);
	if (ret == 0) {
		// stat() buffer retrieved.
		*pMtime = buf.st_mtime;
	}

	return (ret == 0 ? 0 : -errno);
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

	int ret = unlink(filename);
	if (ret != 0) {
		// Error deleting the file.
		ret = -errno;
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
		return -EINVAL;
	
	struct stat buf;
	int ret = lstat(filename, &buf);
	if (ret != 0) {
		// lstat() failed.
		// Assume this is not a symlink.
		return false;
	}
	return !!S_ISLNK(buf.st_mode);
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

	// NOTE: realpath() might not be available on some systems...
	string ret;
	char *const resolved_path = realpath(filename, nullptr);
	if (resolved_path != nullptr) {
		ret = resolved_path;
		free(resolved_path);
	}
	return ret;
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
	bool bRet = false;

#ifdef __linux__
	// TODO: Get the mount point, then look it up in /proc/mounts.

	struct statfs sfbuf;
	int ret = statfs(filename, &sfbuf);
	if (ret != 0) {
		// statfs() failed.
		// Assume this isn't a network file system.
		return false;
	}

	switch (static_cast<uint32_t>(sfbuf.f_type)) {
		case DEBUGFS_MAGIC:
		case DEVPTS_SUPER_MAGIC:
		case EFIVARFS_MAGIC:
		case FUTEXFS_SUPER_MAGIC:
		case MQUEUE_MAGIC:
		case PIPEFS_MAGIC:
		case PROC_SUPER_MAGIC:
		case PSTOREFS_MAGIC:
		case SECURITYFS_MAGIC:
		case SMACK_MAGIC:
		case SOCKFS_MAGIC:
		case TRACEFS_MAGIC:
		case USBDEVICE_SUPER_MAGIC:
			// Bad file systems.
			bRet = true;
			break;

		case AFS_SUPER_MAGIC:
		case CIFS_MAGIC_NUMBER:
		case CODA_SUPER_MAGIC:
		case COH_SUPER_MAGIC:
		case FUSE_SUPER_MAGIC:	// TODO: Check the actual fs type.
		case NCP_SUPER_MAGIC:
		case NFS_SUPER_MAGIC:
		case OCFS2_SUPER_MAGIC:
		case SMB_SUPER_MAGIC:
		case V9FS_MAGIC:
			// Network file system.
			// Allow it if we're allowing network file systems.
			bRet = !netFS;
			break;

		default:
			// Other file system.
			break;
	}
#else
# warning TODO: Implement "badfs" support for non-Linux systems.
	RP_UNUSED(filename);
	RP_UNUSED(netFS);
#endif

	return bRet;
}

/**
 * Get a file's size and time.
 * @param filename	[in] Filename.
 * @param pFileSize	[out] File size.
 * @param pMtime	[out] Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int get_file_size_and_mtime(const string &filename, int64_t *pFileSize, time_t *pMtime)
{
	assert(pFileSize != nullptr);
	assert(pMtime != nullptr);

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
	struct stat sb;
	if (stat(filename.c_str(), &sb) != 0) {
		// An error occurred.
		int ret = -errno;
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

	// Return the file size and mtime.
	*pFileSize = sb.st_size;
	*pMtime = sb.st_mtime;
	return 0;
}

} }

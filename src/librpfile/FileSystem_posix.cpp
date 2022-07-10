/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * FileSystem_posix.cpp: File system functions. (POSIX implementation)     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"
#include "FileSystem.hpp"

// C includes.
#include <fcntl.h>	// AT_FDCWD
#include <sys/stat.h>	// stat(), statx()
#include <utime.h>
#include <unistd.h>

#ifdef __linux__
// TODO: Remove once /proc/mounts parsing is implemented.
#  include <sys/vfs.h>
#  include <linux/magic.h>
// from `man 2 fstatfs`, but not present in linux/magic.h on 4.14-r1
#  ifndef MQUEUE_MAGIC
#    define MQUEUE_MAGIC 0x19800202
#  endif /* MQUEUE_MAGIC */
#  ifndef TRACEFS_MAGIC
#    define TRACEFS_MAGIC 0x74726163
#  endif /* TRACEFS_MAGIC */
#  ifndef CIFS_MAGIC_NUMBER
#    define CIFS_MAGIC_NUMBER 0xff534d42
#  endif /* CIFS_MAGIC_NUMBER */
#  ifndef COH_SUPER_MAGIC
#    define COH_SUPER_MAGIC 0x012ff7b7
#  endif /* COH_SUPER_MAGIC */
#  ifndef FUSE_SUPER_MAGIC
#    define FUSE_SUPER_MAGIC 0x65735546
#  endif /* FUSE_SUPER_MAGIC */
#  ifndef OCFS2_SUPER_MAGIC
#    define OCFS2_SUPER_MAGIC 0x7461636f
#  endif /* OCFS2_SUPER_MAGIC */
#endif /* __linux__ */

// C++ STL classes.
using std::string;
using std::u8string;
using std::u16string;

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
off64_t filesize(const string &filename)
{
#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename.c_str(), 0, STATX_SIZE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_SIZE)) {
		// statx() failed and/or did not return the file size.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	return sbx.stx_size;
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = stat(filename.c_str(), &sb);
	if (ret != 0) {
		// stat() failed.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	return sb.st_size;
#endif /* HAVE_STATX */
}

/**
 * Set the modification timestamp of a file.
 * @param filename Filename
 * @param mtime Modification time
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const char *filename, time_t mtime)
{
	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
	struct utimbuf utbuf;
	utbuf.actime = time(nullptr);
	utbuf.modtime = mtime;
	int ret = utime(filename, &utbuf);

	return (ret == 0 ? 0 : -errno);
}

/**
 * Get the modification timestamp of a file.
 * @param filename Filename
 * @param pMtime Buffer for the modification timestamp
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const char *filename, time_t *pMtime)
{
	assert(pMtime != nullptr);
	if (!pMtime) {
		return -EINVAL;
	}

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning

#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename, 0, STATX_MTIME, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_MTIME)) {
		// statx() failed and/or did not return the modification time.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	*pMtime = sbx.stx_mtime.tv_sec;
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = stat(filename, &sb);
	if (ret != 0) {
		// stat() failed.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	*pMtime = sb.st_mtime;
#endif /* HAVE_STATX */

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

	int ret = unlink(filename);
	if (ret != 0) {
		// Error deleting the file.
		ret = -errno;
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
bool is_symlink(const char8_t *filename)
{
	if (unlikely(!filename || filename[0] == 0)) {
		return -EINVAL;
	}
	
#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, reinterpret_cast<const char*>(filename), AT_SYMLINK_NOFOLLOW, STATX_TYPE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_TYPE)) {
		// statx() failed and/or did not return the file type.
		// Assume this is not a symlink.
		return false;
	}
	return !!S_ISLNK(sbx.stx_mode);
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = lstat(reinterpret_cast<const char*>(filename), &sb);
	if (ret != 0) {
		// lstat() failed.
		// Assume this is not a symlink.
		return false;
	}
	return !!S_ISLNK(sb.st_mode);
#endif /* HAVE_STATX */
}

/**
 * Resolve a symbolic link.
 *
 * If the specified filename is not a symbolic link,
 * the filename will be returned as-is.
 *
 * @param filename Filename of the symbolic link
 * @return Resolved symbolic link, or empty string on error.
 */
u8string resolve_symlink(const char8_t *filename)
{
	u8string s_ret;
	if (unlikely(!filename || filename[0] == 0)) {
		return s_ret;
	}

	// NOTE: realpath() might not be available on some systems...
	char *const resolved_path = realpath(reinterpret_cast<const char*>(filename), nullptr);
	if (resolved_path != nullptr) {
		s_ret = reinterpret_cast<const char8_t*>(resolved_path);
		free(resolved_path);
	}
	return s_ret;
}

/**
 * Check if the specified file is a directory.
 *
 * Symbolic links are resolved as per usual directory traversal.
 *
 * @param filename Filename of the file to check
 * @return True if the file is a directory; false if not.
 */
bool is_directory(const char8_t *filename)
{
	if (unlikely(!filename || filename[0] == 0)) {
		return -EINVAL;
	}

#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, reinterpret_cast<const char*>(filename), 0, STATX_TYPE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_TYPE)) {
		// statx() failed and/or did not return the file type.
		// Assume this is not a directory.
		return false;
	}
	return !!S_ISDIR(sbx.stx_mode);
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = stat(reinterpret_cast<const char*>(filename), &sb);
	if (ret != 0) {
		// stat() failed.
		// Assume this is not a directory.
		return false;
	}
	return !!S_ISDIR(sb.st_mode);
#endif /* HAVE_STATX */
}

/**
 * Is a file located on a "bad" file system?
 *
 * We don't want to check files on e.g. procfs,
 * or on network file systems if the option is disabled.
 *
 * @param filename Filename of the file to check
 * @param netFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
bool isOnBadFS(const char8_t *filename, bool netFS)
{
	bool bRet = false;

#ifdef __linux__
	// TODO: Get the mount point, then look it up in /proc/mounts.

	struct statfs sfbuf;
	int ret = statfs(reinterpret_cast<const char*>(filename), &sfbuf);
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
		case NCP_SUPER_MAGIC:
		case NFS_SUPER_MAGIC:
		case OCFS2_SUPER_MAGIC:
		case SMB_SUPER_MAGIC:
		case V9FS_MAGIC:
			// Network file system.
			// Allow it if we're allowing network file systems.
			bRet = !netFS;
			break;

		case FUSE_SUPER_MAGIC:	// TODO: Check the actual fs type.
			// Other file system.
			// FIXME: `fuse` is used for various local file systems
			// as well as sshfs. Local is more common, so let's assume
			// it's in use for a local file system.
			break;

		default:
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
int get_file_size_and_mtime(const string &filename, off64_t *pFileSize, time_t *pMtime)
{
	assert(pFileSize != nullptr);
	assert(pMtime != nullptr);

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning

#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename.c_str(), 0, STATX_TYPE | STATX_MTIME | STATX_SIZE, &sbx);
	if (ret != 0 || (sbx.stx_mask & (STATX_MTIME | STATX_SIZE)) != (STATX_MTIME | STATX_SIZE)) {
		// statx() failed and/or did not return the modification time or file size.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}

	// Make sure this is not a directory.
	if ((sbx.stx_mask & STATX_TYPE) && S_ISDIR(sbx.stx_mode)) {
		// It's a directory.
		return -EISDIR;
	}

	// Return the file size and mtime.
	*pFileSize = sbx.stx_size;
	*pMtime = sbx.stx_mtime.tv_sec;
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = stat(filename.c_str(), &sb);
	if (ret != 0) {
		// stat() failed.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}

	// Make sure this is not a directory.
	if (S_ISDIR(sb.st_mode)) {
		// It's a directory.
		return -EISDIR;
	}

	// Return the file size and mtime.
	*pFileSize = sb.st_size;
	*pMtime = sb.st_mtime;
#endif /* HAVE_STATX */

	return 0;
}

} }

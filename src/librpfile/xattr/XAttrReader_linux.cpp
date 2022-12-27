/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_linux.cpp: Extended Attribute reader (Linux version)        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "ext2_flags.h"

// for EXT2 flags [TODO: move to librpbase/libromdata]
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/xattr.h>
#include <fcntl.h>
// for FS_IOC_GETFLAGS (equivalent to EXT2_IOC_GETFLAGS)
#include <linux/fs.h>
// for FAT_IOCTL_GET_ATTRIBUTES
#include <linux/msdos_fs.h>

// C++ STL classes
using std::string;
using std::unique_ptr;

// XAttrReader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern uint8_t RP_LibRpFile_XAttrReader_impl_ForceLinkage;
	uint8_t RP_LibRpFile_XAttrReader_impl_ForceLinkage;
}

namespace LibRpFile {

/** XAttrReaderPrivate **/

XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
	: lastError(0)
	, hasLinuxAttributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, linuxAttributes(0)
	, dosAttributes(0)
{
	// Make sure this is a regular file.
	// TODO: Use statx() if available.
	struct stat sb;
	errno = 0;
	if (!stat(filename, &sb) && !S_ISREG(sb.st_mode) && !S_ISDIR(sb.st_mode)) {
		// stat() failed, or this is neither a regular file nor a directory.
		lastError = -errno;
		if (lastError == 0) {
			lastError = -ENOTSUP;
		}
		return;
	}

	// Open the file to get attributes.
	// TODO: Move this to librpbase or libromdata,
	// and add configure checks for FAT_IOCTL_GET_ATTRIBUTES.
#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK|O_LARGEFILE|O_NOFOLLOW)
	errno = 0;
	int fd = open(filename, OPEN_FLAGS);
	if (fd < 0) {
		// Error opening the file.
		lastError = -errno;
		if (lastError == 0) {
			lastError = -EIO;
		}
		return;
	}

	// Initialize attributes.
	lastError = init(fd);
	close(fd);
}

/**
 * Initialize attributes.
 * @param fd File descriptor
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::init(int fd)
{
	// Verify the file mode again using fstat().
	struct stat sb;
	errno = 0;
	if (!fstat(fd, &sb) && !S_ISREG(sb.st_mode) && !S_ISDIR(sb.st_mode)) {
		// fstat() failed, or this is neither a regular file nor a directory.
		int err = -errno;
		if (err == 0) {
			err = -EIO;
		}
		return err;
	}

	// Load the attributes.
	loadLinuxAttrs(fd);
	loadDosAttrs(fd);
	loadGenericXattrs(fd);
	return 0;
}

/**
 * Load Linux attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadLinuxAttrs(int fd)
{
	// Attempt to get EXT2 flags.
	// NOTE: The ioctl is defined as using long, but the actual
	// kernel code uses int.
	int ret;
	if (!ioctl(fd, FS_IOC_GETFLAGS, &linuxAttributes)) {
		// ioctl() succeeded. We have EXT2 flags.
		hasLinuxAttributes = true;
		ret = 0;
	} else {
		// No EXT2 flags on this file.
		// Assume this file system doesn't support them.
		ret = errno;
		if (ret == 0) {
			ret = -EIO;
		}

		linuxAttributes = 0;
		hasLinuxAttributes = false;
	}
	return ret;
}

/**
 * Load MS-DOS attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(int fd)
{
	// Attempt to get MS-DOS attributes.
	// TODO: Also check xattrs.
	// ntfs3 has: system.dos_attrib, system.ntfs_attrib
	// ntfs-3g has: system.ntfs_attrib, system.ntfs_attrib_be
	int ret;
	if (!ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &dosAttributes)) {
		// ioctl() succeeded. We have MS-DOS attributes.
		hasDosAttributes = true;
		ret = 0;
	} else {
		// No MS-DOS attributes on this file.
		// Assume this is not an MS-DOS file system.
		ret = errno;
		if (ret == 0) {
			ret = -EIO;
		}

		dosAttributes = 0;
		hasDosAttributes = false;
	}
	return ret;
}

/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs(int fd)
{
	genericXAttrs.clear();

	// Get the size of the xattr name list.
	ssize_t list_size = flistxattr(fd, nullptr, 0);
	if (list_size == 0) {
		// No xattrs. Show an empty list.
		hasGenericXAttrs = true;
		return 0;
	} else if (list_size < 0) {
		// Xattrs are not supported.
		return -ENOTSUP;
	}

	unique_ptr<char[]> list_buf(new char[list_size]);
	if (flistxattr(fd, list_buf.get(), list_size) != list_size) {
		// List size doesn't match. Something broke here...
		return -ENOTSUP;
	}
	// List should end with a NULL terminator.
	if (list_buf[list_size-1] != '\0') {
		// Not NULL-terminated...
		return -EIO;
	}

	// Value buffer
	size_t value_len = 256;
	unique_ptr<char[]> value_buf(new char[value_len]);

	// List contains NULL-terminated strings.
	// Process strings until we reach list_buf + list_size.
	const char *const list_end = &list_buf[list_size];
	const char *p = list_buf.get();
	while (p < list_end) {
		const char *name = p;
		if (name[0] == '\0') {
			// Empty name. Assume we're at the end of the list.
			break;
		}
		p += strlen(name) + 1;

		// Get the value for this attribute.
		// NOTE: vlen does *not* include a NULL-terminator.
		ssize_t vlen = fgetxattr(fd, name, nullptr, 0);
		if (vlen <= 0) {
			// Error retrieving attribute information.
			continue;
		} else if ((size_t)vlen > value_len) {
			// Need to reallocate the buffer.
			value_len = vlen;
			value_buf.reset(new char[value_len]);
		}
		if (fgetxattr(fd, name, value_buf.get(), value_len) != vlen) {
			// Failed to get this attribute. Skip it for now.
			continue;
		}

		// We have the attribute.
		genericXAttrs.emplace_back(string(name), string(value_buf.get(), vlen));
	}

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}

}

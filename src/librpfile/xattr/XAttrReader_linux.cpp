/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_linux.cpp: Extended Attribute reader (Linux version)        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

#include "librpcpu/byteswap_rp.h"

#include <fcntl.h>	// AT_FDCWD
#include <sys/stat.h>	// stat(), statx()
#include <sys/ioctl.h>
#include <sys/xattr.h>
#include <unistd.h>

// EXT2 flags (also used for EXT3, EXT4, and other Linux file systems)
#include "ext2_flags.h"
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
	: fd(-1)
	, lastError(0)
	, hasLinuxAttributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, linuxAttributes(0)
	, dosAttributes(0)
{
	// Make sure this is a regular file or a directory.
	mode_t mode;

#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, filename, 0, STATX_TYPE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_TYPE)) {
		// An error occurred.
		lastError = -errno;
		if (lastError == 0) {
			lastError = -ENOTSUP;
		}
		return;
	}

	mode = sbx.stx_mode;
#else /* !HAVE_STATX */
	struct stat sb;
	errno = 0;
	if (!stat(filename, &sb)) {
		// stat() failed.
		lastError = -errno;
		if (lastError == 0) {
			lastError = -ENOTSUP;
		}
		return;
	}

	mode = sb.st_mode;
#endif /* HAVE_STATX */

	if (!S_ISREG(mode) && !S_ISDIR(mode)) {
		// This is neither a regular file nor a directory.
		lastError = -ENOTSUP;
		return;
	}

	// Open the file to get attributes.
	// TODO: Move this to librpbase or libromdata,
	// and add configure checks for FAT_IOCTL_GET_ATTRIBUTES.
#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK|O_LARGEFILE)
	errno = 0;
	fd = open(filename, OPEN_FLAGS);
	if (fd < 0) {
		// Error opening the file.
		lastError = -errno;
		if (lastError == 0) {
			lastError = -EIO;
		}
		return;
	}

	// Initialize attributes.
	lastError = init();
	close(fd);
}

/**
 * Initialize attributes.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::init(void)
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
	loadLinuxAttrs();
	loadDosAttrs();
	loadGenericXattrs();
	return 0;
}

/**
 * Load Linux attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadLinuxAttrs(void)
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
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(void)
{
	// Attempt to get MS-DOS attributes.

	// ioctl (Linux vfat only)
	if (!ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &dosAttributes)) {
		// ioctl() succeeded. We have MS-DOS attributes.
		hasDosAttributes = true;
		return 0;
	}

	// Try system xattrs:
	// ntfs3 has: system.dos_attrib, system.ntfs_attrib
	// ntfs-3g has: system.ntfs_attrib, system.ntfs_attrib_be
	// Attribute is stored as a 32-bit DWORD.
	union {
		uint8_t u8[16];
		uint32_t u32;
	} buf;

	struct DosAttrName {
		const char name[23];
		bool be32;
	};
	static const DosAttrName dosAttrNames[] = {
		{"system.ntfs_attrib_be", true},
		{"system.ntfs_attrib", false},
		{"system.dos_attrib", false},
	};
	for (const auto &p : dosAttrNames) {
		ssize_t sz = fgetxattr(fd, p.name, buf.u8, sizeof(buf.u8));
		if (sz == 4) {
			dosAttributes = (p.be32) ? be32_to_cpu(buf.u32) : le32_to_cpu(buf.u32);
			hasDosAttributes = true;
			return 0;
		}
	}

	// No valid attributes found.
	return -ENOENT;
}

/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs(void)
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
		const ssize_t vlen = fgetxattr(fd, name, nullptr, 0);
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
		// NOTE: Not checking for duplicates, since there
		// shouldn't be duplicate attribute names.
		genericXAttrs.emplace(string(name), string(value_buf.get(), vlen));
	}

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}

}

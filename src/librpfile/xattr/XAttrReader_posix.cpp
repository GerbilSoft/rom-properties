/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_posix.cpp: Extended Attribute reader (POSIX version)        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

#include <fcntl.h>	// AT_FDCWD
#include <sys/stat.h>	// stat(), statx()
#include <sys/ioctl.h>
#include <unistd.h>

#if defined(HAVE_SYS_XATTR_H)
// NOTE: Mac fsetxattr() is the same as Linux but with an extra options parameter.
#  include <sys/xattr.h>
#elif defined(HAVE_SYS_EXTATTR_H)
#  include <sys/types.h>
#  include <sys/extattr.h>
#endif

// Ext2 flags (also used for Ext3, Ext4, and other Linux file systems)
#include "ext2_flags.h"

// DOS attributes
#include "dos_attrs.h"

#ifdef __linux__
// for the following ioctls:
// - FS_IOC_GETFLAGS (equivalent to EXT2_IOC_GETFLAGS)
// - FS_IOC_FSGETXATTR (equivalent to XFS_IOC_FSGETXATTR)
#  include <linux/fs.h>
#  ifndef FS_IOC_GETFLAGS
#    ifdef EXT2_IOC_GETFLAGS
#      define FS_IOC_GETFLAGS EXT2_IOC_GETFLAGS
#    endif /* EXT2_IOC_GETFLAGS */
#  endif /* !FS_IOC_GETFLAGS */
#  ifndef FS_IOC_FSGETXATTR
#    ifdef XFS_IOC_FSGETXATTR
#      define FS_IOC_FSGETXATTR XFS_IOC_FSGETXATTR
#    else /* !XFS_IOC_FSGETXATTR */
       // System headers are too old...
       // (Ubuntu 16.04 is missing this, even though XFS has been around for years?)
       // Defining everything here *does* work, so we'll do that.
struct fsxattr {
	uint32_t	fsx_xflags;	/* xflags field value (get/set) */
	uint32_t	fsx_extsize;	/* extsize field value (get/set)*/
	uint32_t	fsx_nextents;	/* nextents field value (get)	*/
	uint32_t	fsx_projid;	/* project identifier (get/set) */
	uint32_t	fsx_cowextsize;	/* CoW extsize field value (get/set)*/
	unsigned char	fsx_pad[8];
};
#      define FS_IOC_FSGETXATTR _IOR('X', 31, struct fsxattr)
#    endif /* XFS_IOC_FSGETXATTR */
#  endif /* !FS_IOC_FSGETXATTR */
// for the following ioctls:
// - FAT_IOCTL_GET_ATTRIBUTES
#  include <linux/msdos_fs.h>
#endif /* __linux__ */

// O_LARGEFILE is Linux-specific.
#ifndef O_LARGEFILE
#  define O_LARGEFILE 0
#endif /* !O_LARGEFILE */

// Uninitialized vector class
#include "uvector.h"

// C++ STL classes
using std::array;
using std::string;

// XAttrReader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
	unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
}

namespace LibRpFile {

// Valid MS-DOS attributes
static constexpr unsigned int VALID_DOS_ATTRIBUTES_FAT = \
	FILE_ATTRIBUTE_READONLY | \
	FILE_ATTRIBUTE_HIDDEN | \
	FILE_ATTRIBUTE_SYSTEM | \
	FILE_ATTRIBUTE_ARCHIVE;
static constexpr unsigned int VALID_DOS_ATTRIBUTES_NTFS = \
	VALID_DOS_ATTRIBUTES_FAT | \
	FILE_ATTRIBUTE_COMPRESSED | \
	FILE_ATTRIBUTE_ENCRYPTED;

/** XAttrReaderPrivate **/

XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
	: fd(-1)
	, lastError(0)
	, hasExt2Attributes(false)
	, hasXfsAttributes(false)
	, hasDosAttributes(false)
	, hasCompressionAlgorithm(false)
	, hasGenericXAttrs(false)
	, ext2Attributes(0)
	, xfsXFlags(0)
	, xfsProjectId(0)
	, dosAttributes(0)
	, validDosAttributes(0)
	, compressionAlgorithm(XAttrReader::ZAlgorithm::None)
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
	if (stat(filename, &sb) != 0) {
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
	static constexpr int OPEN_FLAGS = (O_RDONLY | O_NONBLOCK | O_LARGEFILE);
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
	loadExt2Attrs();
	loadXfsAttrs();
	loadDosAttrs();
	loadCompressionAlgorithm();
	loadGenericXattrs();

	lastError = 0;
	close(fd);
	fd = -1;
}

XAttrReaderPrivate::~XAttrReaderPrivate()
{
	// Just in case fd wasn't closed for some reason...
	if (fd >= 0) {
		close(fd);
	}
}

/**
 * Load Ext2 attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadExt2Attrs(void)
{
	// Attempt to get Ext2 flags.

#if defined(__linux__) && defined(FS_IOC_GETFLAGS)
	// NOTE: The ioctl is defined as using long, but the actual
	// kernel code uses int.
	int ret;
	if (!ioctl(fd, FS_IOC_GETFLAGS, &ext2Attributes)) {
		// ioctl() succeeded. We have Ext2 flags.
		hasExt2Attributes = true;
		ret = 0;
	} else {
		// No Ext2 flags on this file.
		// Assume this file system doesn't support them.
		ret = -errno;
		if (ret == 0) {
			ret = -EIO;
		}

		ext2Attributes = 0;
		hasExt2Attributes = false;
	}
	return ret;
#else /* !__linux__ || !FS_IOC_GETFLAGS */
	// Not supported.
	return -ENOTSUP;
#endif /* __linux__ && FS_IOC_GETFLAGS */
}

/**
 * Load XFS attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadXfsAttrs(void)
{
	// Attempt to get the XFS flags and project ID.

#if defined(__linux__) && defined(FS_IOC_FSGETXATTR)
	// NOTE: If we want to use the fsx_nextents field later,
	// change the ioctl to FS_IOC_FSGETXATTRA.
	int ret;
	struct fsxattr fsx;
	if (!ioctl(fd, FS_IOC_FSGETXATTR, &fsx)) {
		// ioctl() succeeded. We have XFS flags.
		xfsXFlags = fsx.fsx_xflags;
		xfsProjectId = fsx.fsx_projid;
		hasXfsAttributes = true;
		ret = 0;
	} else {
		// No XFS flags on this file.
		// Assume this file system doesn't support them.
		ret = -errno;
		if (ret == 0) {
			ret = -EIO;
		}

		xfsXFlags = 0;
		xfsProjectId = 0;
		hasXfsAttributes = false;
	}
	return ret;
#else /* !__linux__ || !FS_IOC_FSGETXATTR */
	// Not supported.
	return -ENOTSUP;
#endif /* __linux__ && FS_IOC_FSGETXATTR */
}

/**
 * Load MS-DOS attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(void)
{
	// Attempt to get MS-DOS attributes.

#ifdef __linux__
	// ioctl (Linux FAT/exFAT only)
	if (!ioctl(fd, FAT_IOCTL_GET_ATTRIBUTES, &dosAttributes)) {
		// ioctl() succeeded. We have MS-DOS attributes.
		validDosAttributes = VALID_DOS_ATTRIBUTES_FAT;
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
	static const array<DosAttrName, 3> dosAttrNames = {{
		{"system.ntfs_attrib_be", true},
		{"system.ntfs_attrib", false},
		{"system.dos_attrib", false},
	}};
	for (const auto &p : dosAttrNames) {
		ssize_t sz = fgetxattr(fd, p.name, buf.u8, sizeof(buf.u8));
		if (sz == 4) {
			dosAttributes = (p.be32) ? be32_to_cpu(buf.u32) : le32_to_cpu(buf.u32);
			validDosAttributes = VALID_DOS_ATTRIBUTES_NTFS;
			hasDosAttributes = true;
			return 0;
		}
	}

	// No valid attributes found.
	return -ENOENT;
#else /* !__linux__ */
	// Not supported.
	return -ENOTSUP;
#endif /* __linux__ */
}

/**
 * Load the compression algorithm, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadCompressionAlgorithm(void)
{
	// TODO: Check for compression on various file systems.
	hasCompressionAlgorithm = false;
	compressionAlgorithm = XAttrReader::ZAlgorithm::None;
	return -ENOTSUP;
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

#ifdef HAVE_SYS_EXTATTR_H
	// TODO: XAttrReader_freebsd.cpp ran this twice: once for user and once for system.
	// TODO: Read system namespace attributes.
	const int attrnamespace = EXTATTR_NAMESPACE_USER;

	// Namespace prefix
	const char *s_namespace;
	switch (attrnamespace) {
		case EXTATTR_NAMESPACE_SYSTEM:
			s_namespace = "system: ";
			break;
		case EXTATTR_NAMESPACE_USER:
			s_namespace = "user: ";
			break;
		default:
			assert(!"Invalid attribute namespace.");
			s_namespace = "invalid: ";
			break;
	}
#endif /* HAVE_SYS_EXTATTR_H */

	// Partially based on KIO's FileProtocol::copyXattrs().
	// Reference: https://invent.kde.org/frameworks/kio/-/blob/584a81fd453858db432a573c011a1433bc6947e1/src/kioworkers/file/file_unix.cpp#L521
	ssize_t listlen = 0;
	rp::uvector<char> keylist;
	keylist.reserve(256);
	while (true) {
		keylist.resize(listlen);
#if defined(HAVE_SYS_XATTR_H) && !defined(__stub_getxattr) && !defined(__APPLE__)
		listlen = flistxattr(fd, keylist.data(), listlen);
#elif defined(__APPLE__)
		listlen = flistxattr(fd, keylist.data(), listlen, 0);
#elif defined(HAVE_SYS_EXTATTR_H)
		listlen = extattr_list_fd(fd, attrnamespace, listlen == 0 ? nullptr : keylist.data(), listlen);
#endif

		if (listlen > 0 && keylist.size() == 0) {
			continue;
		} else if (listlen > 0 && keylist.size() > 0) {
			break;
		} else if (listlen == -1 && errno == ERANGE) {
			// Not sure when this case would be hit...
			listlen = 0;
			continue;
		} else if (listlen == 0) {
			// No extended attributes.
			hasGenericXAttrs = true;
			return 0;
		} else if (listlen == -1 && errno == ENOTSUP) {
			// Filesystem does not support extended attributes.
			return -ENOTSUP;
		}
	}

	if (listlen == 0 || keylist.empty()) {
		// Shouldn't be empty...
		return -EIO;
	}
	keylist.resize(listlen);

#ifdef HAVE_SYS_XATTR_H
	// Linux, macOS: List should end with a NULL terminator.
	if (keylist[keylist.size()-1] != '\0') {
		// Not NULL-terminated...
		return -EIO;
	}
#endif /* HAVE_SYS_XATTR_H */

	// Value buffer
	rp::uvector<char> value;
	value.reserve(256);

	// Linux, macOS: List contains NULL-terminated strings.
	// FreeBSD: List contains counted (but not NULL-terminated) strings.
	// Process strings until we reach list_buf + list_size.
	const char *const keylist_end = keylist.data() + keylist.size();
	const char *p = keylist.data();
	while (p < keylist_end) {
#if defined(HAVE_SYS_XATTR_H)
		const char *name = p;
		if (name[0] == '\0') {
			// Empty name. Assume we're at the end of the list.
			break;
		}

		// NOTE: If p == keylist_end here, then we're at the last attribute.
		// Only fail if p > keylist_end, because that indicates an overflow.
		p += strlen(name) + 1;
		if (p > keylist_end)
			break;
#elif defined(HAVE_SYS_EXTATTR_H)
		uint8_t len = *p;

		// NOTE: If p + 1 + len == keylist_end here, then we're at the last attribute.
		// Only fail if p + 1 + len > keylist_end, because that indicates an overflow.
			if (p + 1 + len > keylist_end) {
			// Out of bounds.
			break;
		}

		string name(p+1, len);
		p += 1 + len;
#endif

		ssize_t valuelen = 0;
		do {
			// Get the value for this attribute.
			// NOTE: valuelen does *not* include a NULL-terminator.
			value.resize(valuelen);
#if defined(HAVE_SYS_XATTR_H) && !defined(__stub_getxattr) && !defined(__APPLE__)
			valuelen = fgetxattr(fd, name, value.data(), valuelen);
#elif defined(__APPLE__)
			valuelen = fgetxattr(fd, name, value.data(), valuelen, 0, 0);
#elif defined(HAVE_SYS_EXTATTR_H)
			valuelen = extattr_get_fd(fd, attrnamespace, name.c_str(), nullptr, 0);
#endif

			if (valuelen > 0 && value.size() == 0) {
				continue;
			} else if (valuelen > 0 && value.size() > 0) {
				break;
			} else if (valuelen == -1 && errno == ERANGE) {
				valuelen = 0;
				continue;
			} else if (valuelen == 0) {
				// attr value is an empty string
				break;
			}
		} while (true);

		if (valuelen < 0) {
			// Not valid. Go to the next attribute.
			continue;
		}

		// We have the attribute.
		// NOTE: Not checking for duplicates, since there
		// shouldn't be duplicate attribute names.
		string s_value(value.data(), valuelen);
#if HAVE_SYS_EXTATTR_H
		string s_name_prefixed(s_namespace);
		s_name_prefixed.append(name);
		genericXAttrs.emplace(std::move(s_name_prefixed), std::move(s_value));
#else /* !HAVE_SYS_EXTATTR_H */
		genericXAttrs.emplace(name, std::move(s_value));
#endif /* HAVE_SYS_EXTATTR_H */
	}

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}

} // namespace LibRpFile

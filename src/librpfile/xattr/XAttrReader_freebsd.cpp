/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_freebsd.cpp: Extended Attribute reader (FreeBSD version)    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/extattr.h>

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
	// Make sure this is a regular file.
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
#define OPEN_FLAGS (O_RDONLY|O_NONBLOCK)
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
	// FIXME: Is there a similar function on FreeBSD?
	return -ENOTSUP;
}

/**
 * Load MS-DOS attributes, if available.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(void)
{
	// FIXME: Is this possible on FreeBSD?
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

	// FreeBSD has two namespaces for extended attributes:
	// - EXTATTR_NAMESPACE_SYSTEM
	// - EXTATTR_NAMESPACE_USER
	// Handle this by using a lambda function and prefix
	// the attribute names with "system: " or "user: ".

	auto process_attrs = [](XAttrReader::XAttrList &xattrs, int fd, int attrnamespace) -> int {
		// Get the size of the xattr name list.
		ssize_t list_size = extattr_list_fd(fd, attrnamespace, nullptr, 0);
		if (list_size == 0) {
			// No xattrs.
			return 0;
		} else if (list_size < 0) {
			// Xattrs are not supported.
			return -ENOTSUP;
		}

		unique_ptr<char[]> list_buf(new char[list_size]);
		if (extattr_list_fd(fd, attrnamespace, list_buf.get(), list_size) != list_size) {
			// List size doesn't match. Something broke here...
			return -ENOTSUP;
		}

		// Value buffer
		size_t value_len = 256;
		unique_ptr<char[]> value_buf(new char[value_len]);

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

		// List contains counted strings:
		// - One byte indicating length
		// - Attribute name (NOT NULL-terminated)
		// Process strings until we reach list_buf + list_size.
		const char *const list_end = &list_buf[list_size];
		const char *p = list_buf.get();
		while (p < list_end) {
			uint8_t len = *p;

			// NOTE: If p + 1 + len == list_end here, then we're at the last attribute.
			// Only fail if p + 1 + len > list_end, because that indicates an overflow.
			if (p + 1 + len > list_end) {
				// Out of bounds.
				break;
			}

			string name = s_namespace;
			name.append(p+1, len);

			// Get the value for this attribute.
			// NOTE: vlen does *not* include a NULL-terminator.
			ssize_t vlen = extattr_get_fd(fd, attrnamespace, name.c_str(), nullptr, 0);
			if (vlen <= 0) {
				// Error retrieving attribute information.
				continue;
			} else if ((size_t)vlen > value_len) {
				// Need to reallocate the buffer.
				value_len = vlen;
				value_buf.reset(new char[value_len]);
			}
			if (extattr_get_fd(fd, attrnamespace, name.c_str(), value_buf.get(), value_len) != vlen) {
				// Failed to get this attribute. Skip it for now.
				continue;
			}

			// We have the attribute.
			// NOTE: Not checking for duplicates, since there
			// shouldn't be duplicate attribute names.
			string s_value(value_buf.get(), vlen);
			xattrs.emplace(name, std::move(s_value));
		}

		// Extended attributes retrieved successfully.
		return 0;
	};

	int ret1 = process_attrs(genericXAttrs, fd, EXTATTR_NAMESPACE_SYSTEM);
	int ret2 = process_attrs(genericXAttrs, fd, EXTATTR_NAMESPACE_USER);

	if (ret1 != 0 && ret2 != 0) {
		// Error retrieving extended attributes.
		hasGenericXAttrs = false;
		return ret1;
	}

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}

}

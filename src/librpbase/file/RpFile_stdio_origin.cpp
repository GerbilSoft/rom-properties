/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_stdio_origin.cpp: Standard file object. (stdio implementation)   *
 * setOriginInfo() function.                                               *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#ifdef _WIN32
# error RpFile_stdio is not supported on Windows, use RpFile_win32.
#endif /* _WIN32 */

#include "RpFile.hpp"
#include "RpFile_p.hpp"

// Config class (for storeFileOriginInfo)
#include "../config/Config.hpp"

// C includes.
#include <sys/time.h>
#include <sys/types.h>

// xattrs
#if defined(HAVE_FSETXATTR_LINUX)
# include <sys/xattr.h>
#elif defined(HAVE_EXTATTR_SET_FD)
# include <sys/extattr.h>
// Linux-compatible wrapper.
static inline int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags)
{
	RP_UNUSED(flags);
	ssize_t sxret = extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, name, value, size);
	if (sxret != size) {
		errno = EIO;
		return -1;
	}
	return 0;
}
#elif defined(HAVE_FSETXATTR_MAC)
# include <sys/xattr.h>
// TODO: Define a Linux-compatible version.
#endif /* HAVE_FSETXATTR_LINUX || HAVE_FSETXATTR_MAC*/

namespace LibRpBase {

/** File properties (NON-VIRTUAL) **/

/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::setOriginInfo(const std::string &url, time_t mtime)
{
	RP_D(RpFile);
	const int fd = fileno(d->file);

	// TODO: Use the origin website instead of "rom-properties"?
	static const char xdg_publisher[] = "rom-properties";

	// NOTE: Even if one of the xattr functions fails, we'll
	// continue with others and setting mtime. The first error
	// will be returned at the end of the function.
	int err = 0;

	// xattr reference: https://github.com/pkg/xattr

	// NOTE: This will force a configuration timestamp check.
	const Config *const config = Config::instance();
	const bool storeFileOriginInfo = config->storeFileOriginInfo();
	if (storeFileOriginInfo) {
#if defined(HAVE_FSETXATTR_LINUX) || defined(HAVE_EXTATTR_SET_FD)
		// fsetxattr() [Linux version]
		// NOTE: Also used for FreeBSD using a wrapper function.

		// Set the XDG origin attributes.
		errno = 0;
		int sxret = fsetxattr(fd, "user.xdg.origin.url", url.data(), url.size(), 0);
		if (sxret != 0 && err != 0) {
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}

		errno = 0;
		sxret = fsetxattr(fd, "user.xdg.publisher", xdg_publisher, sizeof(xdg_publisher)-1, 0);
		if (sxret != 0 && err != 0) {
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}
#elif defined(HAVE_FSETXATTR_MAC)
		// fsetxattr() [Mac OS X]
		// TODO: Implement this:
		// - com.apple.metadata:kMDItemWhereFroms
		// - com.apple.quarantine
		// References:
		// - https://apple.stackexchange.com/questions/110239/where-is-the-where-from-meta-data-stored-when-downloaded-via-chrome
		// - http://osxdaily.com/2018/05/03/view-remove-extended-attributes-file-mac/
# warning Mac origin info not implemented yet.
#else
# warning No xattr implementation for this system, cannot set origin info.
#endif /* HAVE_FSETXATTR_LINUX */
	}

	// Set the mtime if >= 0.
	if (mtime >= 0) {
		struct timeval tv[2];

		// atime
		// TODO: Nanosecond precision if available?
		int ret = gettimeofday(&tv[0], nullptr);
		if (ret != 0) {
			// gettimeofday() failed for some reason.
			// Fall back to time() with no microseconds.
			tv[0].tv_sec = time(nullptr);
			tv[0].tv_usec = 0;
		}

		// mtime
		tv[1].tv_sec = mtime;
		tv[1].tv_usec = 0;

		// Flush the file before setting the times to ensure
		// that libc doesn't write anything afterwards.
		::fflush(d->file);

		// Set the times.
		errno = 0;
		ret = futimes(fd, tv);
		if (ret != 0 && err != 0) {
			// Error setting the times.
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}
	}

	return -err;
}

}

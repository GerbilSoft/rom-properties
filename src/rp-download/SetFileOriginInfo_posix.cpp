/******************************************************************************
 * ROM Properties Page shell extension. (rp-download)                         *
 * SetFileOriginInfo_posix.cpp: setFileOriginInfo() function. (POSIX version) *
 *                                                                            *
 * Copyright (c) 2016-2023 by David Korth.                                    *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

#include "stdafx.h"
#include "config.rp-download.h"
#include "SetFileOriginInfo.hpp"

#ifdef _WIN32
# error SetFileOriginInfo_posix.cpp is for POSIX systems, not Windows.
#endif /* _WIN32 */

// libunixcommon
#include "libunixcommon/userdirs.hpp"

// INI parser.
#include "ini.h"

// C includes.
#include <sys/time.h>
#include <sys/types.h>

// C++ STL classes.
using std::string;

// xattrs
#if defined(HAVE_SYS_XATTR_H)
// NOTE: Mac fsetxattr() is the same as Linux but with an extra options parameter.
#  include <sys/xattr.h>
#elif defined(HAVE_SYS_EXTATTR_H)
#  include <sys/types.h>
#  include <sys/extattr.h>
#endif

namespace RpDownload {

/**
 * Process a configuration line.
 * @param user Pointer to bool for StoreFileOriginInfo.
 * @param section Section.
 * @param name Key.
 * @param value Value.
 * @return 1 to continue; 0 to stop processing.
 */
static int processConfigLine(void *user, const char *section, const char *name, const char *value)
{
	if (!strcasecmp(section, "Downloads") &&
	    !strcasecmp(name, "StoreFileOriginInfo"))
	{
		// Found the key.
		// Parse the value.
		if (!strcasecmp(value, _T("false")) || !strcmp(value, "0")) {
			// Disabled.
			*(bool*)user = false;
			return 0;
		} else {
			// Enabled.
			*(bool*)user = true;
			return 0;
		}
	}

	// Not StoreFileOriginInfo. Keep going.
	return 1;
}

/**
 * Get the storeFileOriginInfo setting from rom-properties.conf.
 *
 * Default value is true.
 *
 * @return storeFileOriginInfo setting.
 */
static bool getStoreFileOriginInfo(void)
{
	static const bool default_value = true;

	// Get the config filename.
	// NOTE: Not cached, since rp-download downloads one file per run.
	string conf_filename = LibUnixCommon::getConfigDirectory().c_str();
	if (conf_filename.empty()) {
		// Empty filename...
		return default_value;
	}
	// Add a trailing slash if necessary.
	if (conf_filename.at(conf_filename.size()-1) != DIR_SEP_CHR) {
		conf_filename += DIR_SEP_CHR;
	}
	conf_filename += "rom-properties/rom-properties.conf";

	// Parse the INI file.
	// NOTE: We're stopping parsing once we find the config entry,
	// so ignore the return value.
	bool bValue = default_value;
	ini_parse(conf_filename.c_str(), processConfigLine, &bValue);
	return bValue;
}

/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param file Open file. (Must be writable.)
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int setFileOriginInfo(FILE *file, const TCHAR *url, time_t mtime)
{
	const int fd = fileno(file);

	// TODO: Use the origin website instead of "rom-properties"?
	static const char xdg_publisher[] = "rom-properties";

	// NOTE: Even if one of the xattr functions fails, we'll
	// continue with others and setting mtime. The first error
	// will be returned at the end of the function.
	int err = 0;

	// xattr reference: https://github.com/pkg/xattr

	// Check if storeFileOriginInfo is enabled.
	const bool storeFileOriginInfo = getStoreFileOriginInfo();
	if (storeFileOriginInfo) {
#if defined(HAVE_FSETXATTR_LINUX)
		// fsetxattr() [Linux version]
		errno = 0;
		int sxret = fsetxattr(fd, "user.xdg.origin.url", url, _tcslen(url), 0);
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
#elif defined(HAVE_EXTATTR_SET_FD)
		// extattr_set_fd() [FreeBSD version]
		errno = 0;
		ssize_t sxret = extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, "user.xdg.origin.url", url, _tcslen(url));
		if (sxret != 0 && err != 0) {
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}
		errno = 0;
		sxret = extattr_set_fd(fd, EXTATTR_NAMESPACE_USER, "user.xdg.publisher", xdg_publisher, sizeof(xdg_publisher)-1);
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
#  warning Mac origin info not implemented yet.
#else
#  warning No xattr implementation for this system, cannot set origin info.
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
		::fflush(file);

		// Set the times.
		errno = 0;
		ret = futimes(fd, tv);
		if (ret != 0 && err == 0) {
			// Error setting the times.
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}
	}

	return -err;
}

} //namespace RpDownload

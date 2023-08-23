/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs_mac.mm: Find user directories. (Mac OS X)                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.libunixcommon.h"

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// Reference: https://fossies.org/dox/wxWidgets-3.1.1/stdpaths_8mm_source.html
// NOTE: Foundation is needed for NSFileManager and other types.
// CoreFoundation doesn't include it.
#import <Foundation/Foundation.h>

// C++ includes.
#include <string>
using std::string;

namespace LibUnixCommon {

/**
 * Get a path from NSFileManager.
 * This path won't have a trailing slash.
 * @return Path, or empty string on error.
 */
static string getFMDirectory(NSSearchPathDirectory directory,
	NSSearchPathDomainMask domainMask)
{
	// FIXME: Memory management issues?
	NSURL *url = [[NSFileManager defaultManager] URLForDirectory:directory
			inDomain:domainMask
			appropriateForURL:nil
			create:NO
			error:nil];
	if (!url) {
		// Could not obtain the directory.
		return {};
	}

	// Get the path of the URL.
	NSString *path = (NSString*)[url path];
	if (!path) {
		// Could not obtain the directory.
		return {};
	}

	// Convert to a C++ string.
	return string([path UTF8String]);
}

/**
 * Get the user's home directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's home directory (without trailing slash), or empty string on error.
 */
std::string getHomeDirectory(void)
{
	return getFMDirectory(NSUserDirectory, NSUserDomainMask);
}

/**
 * Get the user's cache directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
string getCacheDirectory(void)
{
	return getFMDirectory(NSCachesDirectory, NSUserDomainMask);
}

/**
 * Get the user's configuration directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's configuration directory (without trailing slash), or empty string on error.
 */
string getConfigDirectory(void)
{
	// TODO: Application Support or Preferences?
	return getFMDirectory(NSApplicationSupportDirectory, NSUserDomainMask);
}

}

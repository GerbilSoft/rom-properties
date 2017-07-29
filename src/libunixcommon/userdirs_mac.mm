/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs_mac.mm: Find user directories. (Mac OS X)                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "config.libunixcommon.h"

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// Reference: https://fossies.org/dox/wxWidgets-3.1.0/stdpaths_8mm_source.html
#import <CoreFoundation/CoreFoundation.h>

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
		return string();
	}

	// Get the path of the URL.
	NSString *path = (NSString*)[url path];
	if (!path) {
		// Could not obtain the directory.
		return string();
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

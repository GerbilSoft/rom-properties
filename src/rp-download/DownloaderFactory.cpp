/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * DownloaderFactory.cpp: IDownloader factory class.                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DownloaderFactory.hpp"

#ifdef _WIN32
#  include "libwin32common/rp_versionhelpers.h"
#endif /* _WIN32 */

// IDownloader implementations
#include "CurlDownloader.hpp"
#ifdef _WIN32
#  include "WinInetDownloader.hpp"
#endif /* _WIN32 */

namespace RpDownload { namespace DownloaderFactory {

/**
 * Create an IDownloader object.
 *
 * The implementation is chosen depending on the system
 * environment. The caller doesn't need to know what
 * the underlying implementation is.
 *
 * @return IDownloader object, or nullptr if an error occurred.
 */
IDownloader *create(void)
{
	IDownloader *downloader = nullptr;

#ifndef _WIN32
	// Non-Windows: Use cURL.
	downloader = new CurlDownloader();
#else /* _WIN32 */
	// Windows: Use WinInet for modern Windows; cURL for Windows XP/2003.

	if (!IsWindowsVistaOrGreater()) {
		// Windows XP/2003 or earlier: Try cURL first.
		downloader = new CurlDownloader();
		if (downloader->isUsable()) {
			return downloader;
		}

		// cURL is not usable. Fall back to WinInet, even though it
		// might have issues with modern Internet security protocols
		// on Windows XP/2003.
		delete downloader;
		downloader = nullptr;
	}

	// Windows Vista or later: Use WinInet.
	downloader = new WinInetDownloader();
#endif

	if (!downloader || !downloader->isUsable()) {
		// IDownloader object is not usable...
		delete downloader;
		downloader = nullptr;
	}

	return downloader;
}

/**
 * Create an IDownloader object.
 *
 * The implementation can be selected by the caller.
 * This is usually only used for test suites.
 *
 * @return IDownloader object, or nullptr if the selected implementation isn't supported.
 */
IDownloader *create(Implementation implementation)
{
	IDownloader *downloader = nullptr;

	switch (implementation) {
		default:
			break;

		case Implementation::cURL:
			downloader = new CurlDownloader();
			break;

#ifdef _WIN32
		case Implementation::WinInet:
			downloader = new WinInetDownloader();
			break;
#endif /* _WIN32 */
	}

	if (!downloader || !downloader->isUsable()) {
		// IDownloader object is not usable...
		delete downloader;
		downloader = nullptr;
	}

	return downloader;
}

} }

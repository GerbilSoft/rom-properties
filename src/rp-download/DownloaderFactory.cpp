/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * DownloaderFactory.cpp: IDownloader factory class.                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DownloaderFactory.hpp"

// IDownloader implementations
#ifndef _WIN32
#  include "CurlDownloader.hpp"
#endif /* !_WIN32 */
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
#ifndef _WIN32
	// Non-Windows: Use cURL.
	return new CurlDownloader();
#else /* _WIN32 */
	// Windows: Use WinInet.
	return new WinInetDownloader();
#endif
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

#ifndef _WIN32
		case Implementation::cURL:
			downloader = new CurlDownloader();
			break;
#else /* _WIN32 */
		case Implementation::WinInet:
			downloader = new WinInetDownloader();
			break;
#endif /* _WIN32 */
	}

	return downloader;
}

} }

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * DownloaderFactory.hpp: IDownloader factory class.                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

namespace RpDownload {

class IDownloader;

namespace DownloaderFactory
{

/**
 * Create an IDownloader object.
 *
 * The implementation is chosen depending on the system
 * environment. The caller doesn't need to know what
 * the underlying implementation is.
 *
 * @return IDownloader object, or nullptr if an error occurred.
 */
IDownloader *create(void);

enum class Implementation {
	cURL,
#ifdef _WIN32
	WinInet,
#endif /* _WIN32 */
};

/**
 * Create an IDownloader object.
 *
 * The implementation can be selected by the caller.
 * This is usually only used for test suites.
 *
 * @return IDownloader class, or nullptr if the selected implementation isn't supported.
 */
IDownloader *create(Implementation implementation);

} }

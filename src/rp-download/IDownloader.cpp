/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * IDownloader.cpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "IDownloader.hpp"
#include "config.version.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::tstring;

namespace RpDownload {

IDownloader::IDownloader()
	: m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const TCHAR *url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const tstring &url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::~IDownloader()
{ }

/** Properties. **/

/**
 * Is a download in progress?
 * @return True if a download is in progress.
 */
bool IDownloader::isInProgress(void) const
{
	return m_inProgress;
}

/**
 * Get the current URL.
 * @return URL.
 */
tstring IDownloader::url(void) const
{
	return m_url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const TCHAR *url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const tstring &url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Get the maximum buffer size. (0 == unlimited)
 * @return Maximum buffer size.
 */
size_t IDownloader::maxSize(void) const
{
	return m_maxSize;
}

/**
 * Set the maximum buffer size. (0 == unlimited)
 * @param maxSize Maximum buffer size.
 */
void IDownloader::setMaxSize(size_t maxSize)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_maxSize = maxSize;
}

/** Data accessors. **/

/**
 * Get the size of the data.
 * @return Size of the data.
 */
size_t IDownloader::dataSize(void) const
{
	return m_data.size();
}

/**
* Get a pointer to the start of the data.
* @return Pointer to the start of the data.
*/
const uint8_t *IDownloader::data(void) const
{
	return m_data.data();
}

/**
 * Get the Last-Modified time.
 * @return Last-Modified time, or -1 if none was set by the server.
 */
time_t IDownloader::mtime(void) const
{
	return m_mtime;
}

/**
 * Clear the data.
 */
void IDownloader::clear(void)
{
	assert(!m_inProgress);
	// TODO: Don't clear if m_inProgress?
	m_data.clear();
}

/**
 * Create the User-Agent value.
 */
void IDownloader::createUserAgent(void)
{
	m_userAgent.reserve(256);
	m_userAgent = _T("rom-properties/") _T(RP_VERSION_STRING);

	// CPU
#if defined(_M_ARM64) || defined(__aarch64__)
# define CPU "ARM64"
# define MAC_CPU "ARM64"
#elif defined(_M_ARM) || defined(__arm__)
# define CPU "ARM"
# define MAC_CPU "ARM"
#elif defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__)
# ifdef _WIN32
#  define CPU "x64"
# else /* !_WIN32 */
#  define CPU "x86_64"
# endif
# define MAC_CPU "Intel"
#elif defined(_M_IX86) || defined(__i386__)
# ifdef _WIN32
#  define CPU ""
#  define NO_CPU 1
# else /* !_WIN32 */
#  define CPU "i386"
# endif /* _WIN32 */
# define MAC_CPU "Intel"
#elif defined(__powerpc64__) || defined(__ppc64__)
# define CPU "PPC64"
# define MAC_CPU "PPC"
#elif defined(__powerpc__) || defined(__ppc__)
# define CPU "PPC"
# define MAC_CPU "PPC"
#endif

#ifdef _WIN32
	// TODO: OS version number.
	// For now, assuming "Windows NT".
	m_userAgent += _T(" (Windows NT");
# ifndef NO_CPU
	m_userAgent += _T("; ");
#  ifdef _WIN64
	m_userAgent += _T("Win64; ");
#  endif /* _WIN64 */
	m_userAgent += _T(CPU ")");
# endif /* !NO_CPU */

#elif defined(__linux__)
	// TODO: Kernel version and/or lsb_release?
	m_userAgent += _T(" (Linux " CPU ")");
#elif defined(__FreeBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (FreeBSD " CPU ")");
#elif defined(__NetBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (NetBSD " CPU ")");
#elif defined(__OpenBSD__)
	// TODO: Distribution version?
	m_userAgent += _T(" (OpenBSD " CPU ")");
#elif defined(__bsdi__)
	// TODO: Distribution version?
	m_userAgent += _T(" (BSDi " CPU ")");
#elif defined(__DragonFly__)
	// TODO: Distribution version?
	m_userAgent += _T(" (DragonFlyBSD " CPU ")");
#elif defined(__APPLE__)
	// TODO: OS version?
	m_userAgent += _T(" (Macintosh; " MAC_CPU " Mac OS X)");
#elif defined(__unix__)
	// Generic UNIX fallback.
	m_userAgent += _T(" (Unix " CPU ")");
#else
	// Unknown OS...
	m_userAgent += _T(" (Unknown " CPU ")");
#endif
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
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
using std::string;

namespace LibCacheMgr {

IDownloader::IDownloader()
	: m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const char *url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{
	createUserAgent();
}

IDownloader::IDownloader(const string &url)
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
string IDownloader::url(void) const
{
	return m_url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const char *url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const string &url)
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

/** Proxy server functions. **/
// NOTE: This is only useful for downloaders that
// can't retrieve the system proxy server normally.

/**
 * Get the proxy server.
 * @return Proxy server URL.
 */
string IDownloader::proxyUrl(void) const
{
	return m_proxyUrl;
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
 */
void IDownloader::setProxyUrl(const char *proxyUrl)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	if (proxyUrl) {
		m_proxyUrl = proxyUrl;
	} else {
		m_proxyUrl.clear();
	}
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
 */
void IDownloader::setProxyUrl(const string &proxyUrl)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_proxyUrl = proxyUrl;
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
	m_userAgent = "rom-properties/" RP_VERSION_STRING;

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
	m_userAgent += " (Windows NT";
# ifndef NO_CPU
	m_userAgent += "; ";
#  ifdef _WIN64
	m_userAgent += "Win64; ";
#  endif /* _WIN64 */
	m_userAgent += CPU ")";
# endif /* !NO_CPU */

#elif defined(__linux__)
	// TODO: Kernel version and/or lsb_release?
	m_userAgent += " (Linux " CPU ")";
#elif defined(__FreeBSD__)
	// TODO: Distribution version?
	m_userAgent += " (FreeBSD " CPU ")";
#elif defined(__NetBSD__)
	// TODO: Distribution version?
	m_userAgent += " (NetBSD " CPU ")";
#elif defined(__OpenBSD__)
	// TODO: Distribution version?
	m_userAgent += " (OpenBSD " CPU ")";
#elif defined(__bsdi__)
	// TODO: Distribution version?
	m_userAgent += " (BSDi " CPU ")";
#elif defined(__DragonFly__)
	// TODO: Distribution version?
	m_userAgent += " (DragonFlyBSD " CPU ")";
#elif defined(__APPLE__)
	// TODO: OS version?
	m_userAgent += " (Macintosh; " MAC_CPU " Mac OS X)";
#elif defined(__unix__)
	// Generic UNIX fallback.
	m_userAgent += " (Unix " CPU ")";
#else
	// Unknown OS...
	m_userAgent += " (Unknown " CPU ")";
#endif
}

}

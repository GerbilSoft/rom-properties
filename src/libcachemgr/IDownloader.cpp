/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * IDownloader.cpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "IDownloader.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibCacheMgr {

IDownloader::IDownloader()
	: m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{ }

IDownloader::IDownloader(const rp_char *url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{ }

IDownloader::IDownloader(const LibRpBase::rp_string &url)
	: m_url(url)
	, m_mtime(-1)
	, m_inProgress(false)
	, m_maxSize(0)
{ }

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
LibRpBase::rp_string IDownloader::url(void) const
{
	return m_url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const rp_char *url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void IDownloader::setUrl(const LibRpBase::rp_string &url)
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
LibRpBase::rp_string IDownloader::proxyUrl(void) const
{
	return m_proxyUrl;
}

/**
 * Set the proxy server.
 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
 */
void IDownloader::setProxyUrl(const rp_char *proxyUrl)
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
void IDownloader::setProxyUrl(const LibRpBase::rp_string &proxyUrl)
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

}

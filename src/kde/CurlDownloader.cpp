/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * CurlDownloader.cpp: libcurl-based file downloader.                      *
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

#include "CurlDownloader.hpp"
#include "libromdata/TextFuncs.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// cURL for network access.
#include <curl/curl.h>

CurlDownloader::CurlDownloader()
	: m_inProgress(false)
	, m_maxSize(0)
{ }

CurlDownloader::CurlDownloader(const rp_char *url)
	: m_url(url)
	, m_inProgress(false)
	, m_maxSize(0)
{ }

CurlDownloader::CurlDownloader(const LibRomData::rp_string &url)
	: m_url(url)
	, m_inProgress(false)
	, m_maxSize(0)
{ }

/** Properties. **/

/**
 * Is a download in progress?
 * @return True if a download is in progress.
 */
bool CurlDownloader::isInProgress(void) const
{
	return m_inProgress;
}

/**
 * Get the current URL.
 * @return URL.
 */
LibRomData::rp_string CurlDownloader::url(void) const
{
	return m_url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void CurlDownloader::setUrl(const rp_char *url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Set the URL.
 * @param url New URL.
 */
void CurlDownloader::setUrl(const LibRomData::rp_string &url)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_url = url;
}

/**
 * Get the maximum buffer size. (0 == unlimited)
 * @return Maximum buffer size.
 */
size_t CurlDownloader::maxSize(void) const
{
	return m_maxSize;
}

/**
 * Set the maximum buffer size. (0 == unlimited)
 * @param maxSize Maximum buffer size.
 */
void CurlDownloader::setMaxSize(size_t maxSize)
{
	assert(!m_inProgress);
	// TODO: Don't set if m_inProgress?
	m_maxSize = maxSize;
}

/**
 * Get the size of the data.
 * @return Size of the data.
 */
size_t CurlDownloader::dataSize(void) const
{
	return m_data.size();
}

/**
* Get a pointer to the start of the data.
* @return Pointer to the start of the data.
*/
const uint8_t *CurlDownloader::data(void) const
{
	return m_data.data();
}

/**
 * Clear the data.
 */
void CurlDownloader::clear(void)
{
	assert(!m_inProgress);
	// TODO: Don't clear if m_inProgress?
	m_data.clear();
}

/** Main functions. **/

/**
 * Internal cURL data write function.
 * @param ptr Data to write.
 * @param size Element size.
 * @param nmemb Number of elements.
 * @param userdata m_data pointer.
 * @return Number of bytes written.
 */
size_t CurlDownloader::write_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	// References:
	// - http://stackoverflow.com/questions/1636333/download-file-using-libcurl-in-c-c
	// - http://stackoverflow.com/a/1636415
	// - https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
	CurlDownloader *curlDL = reinterpret_cast<CurlDownloader*>(userdata);
	vector<uint8_t> *vec = &curlDL->m_data;
	size_t len = size * nmemb;

	if (curlDL->m_maxSize > 0) {
		// Maximum buffer size is set.
		// TODO: Check Content-Length header before receiving anything?
		if (vec->size() + len > curlDL->m_maxSize) {
			// Out of memory.
			return 0;
		}
	}

	size_t pos = vec->size();
	vec->resize(pos + len);
	memcpy(vec->data() + pos, ptr, len);
	return len;
}

/**
 * Download the file.
 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
 */
int CurlDownloader::download(void)
{
	// References:
	// - http://stackoverflow.com/questions/1636333/download-file-using-libcurl-in-c-c
	// - http://stackoverflow.com/a/1636415
	// - https://curl.haxx.se/libcurl/c/curl_easy_setopt.html

	// Initialize cURL.
	CURL *curl = curl_easy_init();
	if (!curl) {
		// Could not initialize cURL.
		return -1;	// TODO: Better error?
	}

	// Reserve at least 128 KB.
	m_data.reserve(128*1024);

	// Convert the URL to UTF-8.
	// TODO: Only if not RP_UTF8?
	string url8 = LibRomData::rp_string_to_utf8(m_url);

	// Set options for curl's "easy" mode.
	curl_easy_setopt(curl, CURLOPT_URL, url8.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	// TODO: Set the User-Agent?
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		// Error downloading the file.
		return -2;
	}

	// Check if we have data.
	if (m_data.empty()) {
		// No data.
		return -3;
	}

	// Data retrieved.
	return 0;
}

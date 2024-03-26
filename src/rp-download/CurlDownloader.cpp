/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CurlDownloader.cpp: libcurl-based file downloader.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CurlDownloader.hpp"

// C++ STL classes.
using std::string;

// cURL for network access.
#include <curl/curl.h>

namespace RpDownload {

CurlDownloader::CurlDownloader()
	: super()
{}

CurlDownloader::CurlDownloader(const TCHAR *url)
	: super(url)
{}

CurlDownloader::CurlDownloader(const tstring &url)
	: super(url)
{}

/**
 * Internal cURL data write function.
 * @param ptr Data to write.
 * @param size Element size.
 * @param nmemb Number of elements.
 * @param userdata m_data pointer.
 * @return Number of bytes written.
 */
size_t CurlDownloader::write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	// References:
	// - http://stackoverflow.com/questions/1636333/download-file-using-libcurl-in-c-c
	// - http://stackoverflow.com/a/1636415
	// - https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
	CurlDownloader *curlDL = static_cast<CurlDownloader*>(userdata);
	rp::uvector<uint8_t> *vec = &curlDL->m_data;
	const size_t len = size * nmemb;

	if (curlDL->m_maxSize > 0) {
		// Maximum buffer size is set.
		if (vec->size() + len > curlDL->m_maxSize) {
			// Out of memory.
			return 0;
		}
	}

	if (vec->capacity() == 0) {
		// Capacity wasn't initialized by Content-Length.
		// Reserve at least 64 KB.
		static constexpr size_t min_reserve = 64*1024;
		const size_t reserve = (len > min_reserve ? len : min_reserve);
		vec->reserve(reserve);
	}

	const size_t pos = vec->size();
	vec->resize(pos + len);
	memcpy(vec->data() + pos, ptr, len);
	return len;
}

/**
 * Internal cURL header parsing function.
 * @param ptr Pointer to header data. (NOT necessarily null-terminated!)
 * @param size Element size.
 * @param nitems Number of elements.
 * @param userdata m_data pointer.
 * @return Amount of data processed, or 0 on error.
 */
size_t CurlDownloader::parse_header(char *ptr, size_t size, size_t nitems, void *userdata)
{
	// References:
	// - https://curl.haxx.se/libcurl/c/CURLOPT_HEADERFUNCTION.html

	// TODO: Add support for non-HTTP protocols?
	CurlDownloader *curlDL = static_cast<CurlDownloader*>(userdata);
	rp::uvector<uint8_t> *vec = &curlDL->m_data;
	const size_t len = size * nitems;

	// Supported headers.
	static constexpr char http_content_length[] = "Content-Length: ";
	static constexpr char http_last_modified[] = "Last-Modified: ";

	if (len >= sizeof(http_content_length) &&
	    !strncasecmp(ptr, http_content_length, sizeof(http_content_length)-1))
	{
		// Found the Content-Length.
		// Parse the value.
		char s_val[24];
		size_t val_len = len-sizeof(http_content_length);
		if (val_len >= sizeof(s_val)) {
			// Shouldn't happen...
			val_len = sizeof(s_val)-1;
		}
		memcpy(s_val, ptr+sizeof(http_content_length)-1, val_len);
		s_val[val_len] = 0;

		// Convert the Content-Length to an off64_t.
		char *endptr = nullptr;
		const off64_t fileSize = strtoll(s_val, &endptr, 10);

		// *endptr should be \0 or a whitespace character.
		if (*endptr != '\0' && !ISSPACE(*endptr)) {
			// Content-Length is invalid.
			return 0;
		} else if (fileSize <= 0) {
			// Content-Length is too small.
			return 0;
		} else if (curlDL->m_maxSize > 0 &&
			   fileSize > (off64_t)curlDL->m_maxSize)
		{
			// Content-Length is too big.
			return 0;
		}

		// Reserve enough space for the file being downloaded.
		vec->reserve(fileSize);
	}
	else if (len >= sizeof(http_last_modified) &&
	         !strncasecmp(ptr, http_last_modified, sizeof(http_last_modified)-1))
	{
		// Found the Last-Modified time.
		// Should be in the format: "Wed, 15 Nov 1995 04:58:08 GMT"
		// - "GMT" can be "UTC".
		// - It should NOT be another timezone, but some servers are misconfigured...
		char mtime_str[40];
		size_t val_len = len-sizeof(http_last_modified);
		if (val_len >= sizeof(mtime_str)) {
			// Shouldn't happen...
			val_len = sizeof(mtime_str)-1;
		}
		memcpy(mtime_str, ptr+sizeof(http_last_modified)-1, val_len);
		mtime_str[val_len] = 0;

		// Parse the modification time.
		curlDL->m_mtime = curl_getdate(mtime_str, nullptr);
	}

	// Continue processing.
	return len;
}

/**
 * Download the file.
 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
 */
int CurlDownloader::download(void)
{
	// References:
	// - http://stackoverflow.com/questions/1636333/download-file-using-libcurl-in-c-c
	// - http://stackoverflow.com/a/1636415
	// - https://curl.haxx.se/libcurl/c/curl_easy_setopt.html

	// Clear the previous download.
	m_data.clear();
	m_mtime = -1;

	// Initialize cURL.
	CURL *curl = curl_easy_init();
	if (!curl) {
		// Could not initialize cURL.
		return -ENOMEM;	// TODO: Better error?
	}

	// Proxy settings should be set by the calling application
	// in the http_proxy and https_proxy variables.

	// TODO: Send a HEAD request first?

	// Set options for curl's "easy" mode.
	curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);
	// Fail on HTTP errors. (>= 400)
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	// Redirection is required for https://amiibo.life/nfc/%08X-%08X
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
	// Request file modification time.
	// NOTE: Probably not needed for http...
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

	if (m_if_modified_since >= 0) {
		// Add an "If-Modified-Since" header.
#if LIBCURL_VERSION_NUM >= 0x073B00
		curl_easy_setopt(curl, CURLOPT_TIMEVALUE_LARGE, static_cast<curl_off_t>(m_if_modified_since));
#else /* LIBCURL_VERSION_NUM < 0x073B00 */
		curl_easy_setopt(curl, CURLOPT_TIMEVALUE, static_cast<long>(m_if_modified_since));
#endif /* LIBCURL_VERSION_NUM >= 0x073B00 */
		curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
	}

	// Header and data functions.
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, parse_header);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	// Don't use signals. We're running as a plugin, so using
	// signals might interfere.
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	// Set timeouts to ensure we don't take forever.
	// TODO: User configuration?
	// - Connect timeout: 2 seconds.
	// - Total timeout: 10 seconds.
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

	// Set the User-Agent.
	curl_easy_setopt(curl, CURLOPT_USERAGENT, m_userAgent.c_str());

	// Download the file.
	int ret;
	CURLcode res = curl_easy_perform(curl);
	switch (res) {
		case CURLE_OK:
			// If the file is empty, check for a 304.
			if (m_data.empty() && m_if_modified_since >= 0) {
				long unmet = 0;
				if (!curl_easy_getinfo(curl, CURLINFO_CONDITION_UNMET, &unmet) && unmet) {
					// HTTP 304 Not Modified
					ret = 304;
					break;
				}
			}

			// File downloaded successfull.
			ret = 0;
			break;

		case CURLE_OPERATION_TIMEDOUT:
			// Operation timed out.
			ret = -ETIMEDOUT;
			break;

		default:
			// Some other error downloading the file.
			// Check if we have an HTTP response code.
			// NOTE: GameTDB sometimes returns nothing instead of 404...
			long response_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code <= 0) {
				// No HTTP response code.
				// TODO: Return a cURL error code and/or message...
				return -EIO;
			}
			ret = (int)response_code;
			break;
	}

	curl_easy_cleanup(curl);
	if (ret != 0) {
		return ret;
	}

	// Check if we have data.
	if (m_data.empty()) {
		// No data.
		return -EIO;	// TODO: Better error?
	}

	// Data retrieved.
	return 0;
}

} //namespace RpDownload

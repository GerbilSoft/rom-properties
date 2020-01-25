/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CurlDownloader.cpp: libcurl-based file downloader.                      *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "CurlDownloader.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cstring>

// C++ includes.
#include <string>
using std::string;

// cURL for network access.
#include <curl/curl.h>

namespace RpDownload {

CurlDownloader::CurlDownloader()
	: super()
{ }

CurlDownloader::CurlDownloader(const TCHAR *url)
	: super(url)
{ }

CurlDownloader::CurlDownloader(const tstring &url)
	: super(url)
{ }

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
	ao::uvector<uint8_t> *vec = &curlDL->m_data;
	size_t len = size * nmemb;

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
		static const size_t min_reserve = 64*1024;
		size_t reserve = (len > min_reserve ? len : min_reserve);
		vec->reserve(reserve);
	}

	size_t pos = vec->size();
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
	ao::uvector<uint8_t> *vec = &curlDL->m_data;
	size_t len = size * nitems;

	// Supported headers.
	static const char http_content_length[] = "Content-Length: ";
	static const char http_last_modified[] = "Last-Modified: ";

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

		// Convert the Content-Length to an int64_t.
		char *endptr = nullptr;
		int64_t fileSize = strtoll(s_val, &endptr, 10);

		// *endptr should be \0 or a whitespace character.
		if (*endptr != '\0' && !ISSPACE(*endptr)) {
			// Content-Length is invalid.
			return 0;
		} else if (fileSize <= 0) {
			// Content-Length is too small.
			return 0;
		} else if (curlDL->m_maxSize > 0 &&
			   fileSize > (int64_t)curlDL->m_maxSize)
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
	// TODO: Limit the number of redirects?
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);

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

	// TODO: Set the User-Agent?
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		// Error downloading the file.
		// Check if we have an HTTP response code.
		// NOTE: GameTDB sometimes returns nothing instead of 404...
		long response_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code <= 0) {
			// No HTTP response code.
			return -EIO;
		}
		return (int)response_code;
	}

	// Check if we have data.
	if (m_data.empty()) {
		// No data.
		return -EIO;	// TODO: Better error?
	}

	// Data retrieved.
	return 0;
}

}

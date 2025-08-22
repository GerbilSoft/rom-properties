/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CurlDownloader.cpp: libcurl-based file downloader.                      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CurlDownloader.hpp"

// C++ STL classes
#include <mutex>
using std::string;
using std::unique_ptr;
using std::tstring;

// We're opening libcurl dynamically, so we need to define all
// cURL function prototypes and definitions here.
#include "curl-mini.h"

#ifndef _WIN32
// Unix dlopen()
#  include <dlfcn.h>
typedef void *HMODULE;
// T2U8() is a no-op.
#  define T2U8(str) (str)
#else /* _WIN32 */
// Windows LoadLibrary()
#  include "libwin32common/RpWin32_sdk.h"
#  include "libwin32common/MiniU82T.hpp"
#  include "tcharx.h"
#  define dlsym(handle, symbol)	((void*)GetProcAddress(handle, symbol))
#  define dlclose(handle)	FreeLibrary(handle)
using LibWin32Common::T2U8;
#endif /* _WIN32 */


namespace RpDownload {

class HMODULE_deleter
{
public:
	typedef HMODULE pointer;

	void operator()(HMODULE hModule)
	{
		if (hModule) {
			dlclose(hModule);
		}
	}
};
static unique_ptr<HMODULE, HMODULE_deleter> libcurl_dll;
static std::once_flag curl_once_flag;

// Function pointer macro
#define DEF_FUNCPTR(f) __typeof__(f) * p##f = nullptr
#define LOAD_FUNCPTR(f) p##f = (__typeof__(f)*)dlsym(libcurl_dll.get(), #f)

DEF_FUNCPTR(curl_slist_append);
DEF_FUNCPTR(curl_slist_free_all);
DEF_FUNCPTR(curl_getdate);

DEF_FUNCPTR(curl_easy_init);
DEF_FUNCPTR(curl_easy_setopt);
DEF_FUNCPTR(curl_easy_perform);
DEF_FUNCPTR(curl_easy_cleanup);
DEF_FUNCPTR(curl_easy_getinfo);

/**
 * Initialize libcurl.
 * Called by std::call_once().
 *
 * Check if libcurl_dll is nullptr afterwards.
 */
static void init_curl_once(void)
{
	// Open libcurl.
#ifdef _WIN32
	libcurl_dll.reset(LoadLibraryEx(_T("libcurl.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_APPLICATION_DIR));
#else /* !_WIN32 */
	// TODO: Consistently use either RTLD_NOW or RTLD_LAZY.
	// Maybe make it a CMake option?
	// TODO: Check for ABI version 3 too?
	// Version 4 was introduced with cURL v7.16.0 (October 2006),
	// but Debian arbitrarily kept it at version 3.
	// Reference: https://daniel.haxx.se/blog/2024/10/30/eighteen-years-of-abi-stability/
	libcurl_dll.reset(dlopen("libcurl.so.4", RTLD_LOCAL | RTLD_NOW));
#endif /* _WIN32 */

	if (!libcurl_dll) {
		// Could not open libcurl.
		return;
	}

	// Load all of the function pointers.
	LOAD_FUNCPTR(curl_slist_append);
	LOAD_FUNCPTR(curl_slist_free_all);
	LOAD_FUNCPTR(curl_getdate);

	LOAD_FUNCPTR(curl_easy_init);
	LOAD_FUNCPTR(curl_easy_setopt);
	LOAD_FUNCPTR(curl_easy_perform);
	LOAD_FUNCPTR(curl_easy_cleanup);
	LOAD_FUNCPTR(curl_easy_getinfo);

	if (!pcurl_slist_append ||
	    !pcurl_slist_free_all ||
	    !pcurl_getdate ||
	    !pcurl_easy_init ||
	    !pcurl_easy_setopt ||
	    !pcurl_easy_perform ||
	    !pcurl_easy_cleanup ||
	    !pcurl_easy_getinfo)
	{
		// At least one symbol is missing.
		libcurl_dll.reset();
	}
}

CurlDownloader::CurlDownloader()
{
	std::call_once(curl_once_flag, init_curl_once);
	// FIXME: Set a flag if initialization fails.
}

CurlDownloader::CurlDownloader(const TCHAR *url)
	: super(url)
{
	std::call_once(curl_once_flag, init_curl_once);
	// FIXME: Set a flag if initialization fails.
}

CurlDownloader::CurlDownloader(const tstring &url)
	: super(url)
{
	std::call_once(curl_once_flag, init_curl_once);
	// FIXME: Set a flag if initialization fails.
}

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
			   fileSize > static_cast<off64_t>(curlDL->m_maxSize))
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
		curlDL->m_mtime = pcurl_getdate(mtime_str, nullptr);
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
	CURL *curl = pcurl_easy_init();
	if (!curl) {
		// Could not initialize cURL.
		return -ENOMEM;	// TODO: Better error?
	}

	// Proxy settings should be set by the calling application
	// in the http_proxy and https_proxy variables.

	// TODO: Send a HEAD request first?

	// Set options for curl's "easy" mode.
	pcurl_easy_setopt(curl, CURLOPT_URL, T2U8(m_url).c_str());
	pcurl_easy_setopt(curl, CURLOPT_NOPROGRESS, true);
	// Fail on HTTP errors. (>= 400)
	pcurl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	// Redirection is required for https://amiibo.life/nfc/%08X-%08X
	pcurl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
	pcurl_easy_setopt(curl, CURLOPT_MAXREDIRS, 8L);
	// Request file modification time.
	// NOTE: Probably not needed for http...
	pcurl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

	if (m_if_modified_since >= 0) {
		// Add an "If-Modified-Since" header.
		// TODO: Check cURL version at runtime.
#if LIBCURL_VERSION_NUM >= 0x073B00
		pcurl_easy_setopt(curl, CURLOPT_TIMEVALUE_LARGE, static_cast<curl_off_t>(m_if_modified_since));
#else /* LIBCURL_VERSION_NUM < 0x073B00 */
		pcurl_easy_setopt(curl, CURLOPT_TIMEVALUE, static_cast<long>(m_if_modified_since));
#endif /* LIBCURL_VERSION_NUM >= 0x073B00 */
		pcurl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
	}

	// Custom headers
	struct curl_slist *req_headers = nullptr;
	if (!m_reqMimeType.empty()) {
		// Add the "Accept:" header.
		string accept_header = "Accept: ";
		accept_header += T2U8(m_reqMimeType);

		req_headers = pcurl_slist_append(req_headers, accept_header.c_str());
		
	}

	if (req_headers) {
		pcurl_easy_setopt(curl, CURLOPT_HTTPHEADER, req_headers);
	}

	// Header and data functions.
	pcurl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, parse_header);
	pcurl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
	pcurl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	pcurl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	// Don't use signals. We're running as a plugin, so using
	// signals might interfere.
	pcurl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	// Set timeouts to ensure we don't take forever.
	// TODO: User configuration?
	// - Connect timeout: 2 seconds.
	// - Total timeout: 10 seconds.
	pcurl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
	pcurl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

#ifdef _WIN32
	// FIXME: cURL SSL verification doesn't work on Wine.
	// Need to verify if it's broken on Windows, too.
	pcurl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
#endif /* _WIN32 */

	// Set the User-Agent.
	pcurl_easy_setopt(curl, CURLOPT_USERAGENT, T2U8(m_userAgent).c_str());

	// Download the file.
	int ret;
	CURLcode res = pcurl_easy_perform(curl);
	switch (res) {
		case CURLE_OK:
			// If the file is empty, check for a 304.
			if (m_data.empty() && m_if_modified_since >= 0) {
				long unmet = 0;
				if (!pcurl_easy_getinfo(curl, CURLINFO_CONDITION_UNMET, &unmet) && unmet) {
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
			pcurl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code <= 0) {
				// No HTTP response code.
				// TODO: Return a cURL error code and/or message...
				return -EIO;
			}
			ret = static_cast<int>(response_code);
			break;
	}

	pcurl_slist_free_all(req_headers);
	pcurl_easy_cleanup(curl);
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

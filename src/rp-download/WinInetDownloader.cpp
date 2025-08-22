/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * WinInetDownloader.cpp: WinInet-based file downloader.                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WinInetDownloader.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/rp_versionhelpers.h"
#include "libwin32common/w32err.hpp"
#include "libwin32common/w32time.h"

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::wstring;

// Windows includes
#include <wininet.h>

namespace RpDownload {

class HINTERNET_deleter
{
public:
	typedef HINTERNET pointer;

	void operator()(HINTERNET hInternet)
	{
		if (hInternet) {
			InternetCloseHandle(hInternet);
		}
	}
};

WinInetDownloader::WinInetDownloader(const TCHAR *url)
	: super(url)
{}

WinInetDownloader::WinInetDownloader(const tstring &url)
	: super(url)
{}

/**
 * Is this IDownloader object usable?
 * @return True if it's usable; false if it's not.
 */
bool WinInetDownloader::isUsable(void) const
{
	// WinInetDownloader is always usable on Windows.
	return true;
}

/**
 * Download the file.
 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
 */
int WinInetDownloader::download(void)
{
	// References:
	// - https://docs.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-internetopenw
	// - https://docs.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-internetopenurlw
	// - https://docs.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-httpqueryinfow

	// Clear the previous download.
	m_data.clear();
	m_mtime = -1;

	// Open up an Internet connection.
	// This doesn't actually connect to anything yet.
	unique_ptr<HINTERNET, HINTERNET_deleter> hConnection(
		InternetOpen(
			m_userAgent.c_str(),		// lpszAgent
			INTERNET_OPEN_TYPE_PRECONFIG,	// dwAccessType
			nullptr,			// lpszProxy
			nullptr,			// lpszProxyBypass
			0));				// dwFlags
	if (!hConnection) {
		// Error opening a WinInet instance.
		const int err = w32err_to_posix(GetLastError());
		return (err != 0 ? -err : -EIO);
	}

	// Flags.
	DWORD dwFlags = INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
			INTERNET_FLAG_NO_AUTH |
			INTERNET_FLAG_NO_COOKIES |
			INTERNET_FLAG_NO_UI;
	if (!IsWindowsVistaOrGreater()) {
		// WinInet doesn't support SNI prior to Vista.
		static const TCHAR rpdb_domain[] = _T("https://rpdb.gerbilsoft.com/");
		if (m_url.size() >= _countof(rpdb_domain) &&
		    !_tcsncmp(m_url.c_str(), rpdb_domain, _countof(rpdb_domain)-1))
		{
			// rpdb.gerbilsoft.com requires SNI.
			dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
		}
	}

	// Custom headers
	tstring req_headers;
	if (m_if_modified_since >= 0) {
		// Add an "If-Modified-Since" header.
		// FIXME: +4 is needed to avoid ERROR_INSUFFICIENT_BUFFER.
		TCHAR szTime[INTERNET_RFC1123_BUFSIZE+4];
		SYSTEMTIME st;

		UnixTimeToSystemTime(m_if_modified_since, &st);
		BOOL bRet = InternetTimeFromSystemTime(&st, INTERNET_RFC1123_FORMAT, szTime, sizeof(szTime));
		if (bRet) {
			req_headers = _T("If-Modified-Since: ");
			req_headers += szTime;
		}
	}
	if (!m_reqMimeType.empty()) {
		// Add the "Accept:" header.
		if (!req_headers.empty()) {
			req_headers += _T("\r\n");
		}

		req_headers += _T("Accept: ");
		req_headers += m_reqMimeType;
	}

	// Request the URL.
	unique_ptr<HINTERNET, HINTERNET_deleter> hUrl(
		InternetOpenUrl(
			hConnection.get(),	// hInternet
			m_url.c_str(),		// lpszUrl (Latin-1 characters only!)
			!req_headers.empty() ? req_headers.data() : nullptr,	// lpszHeaders
			static_cast<DWORD>(req_headers.size()),			// dwHeaderLength
			dwFlags,	// dwFlags
			reinterpret_cast<DWORD_PTR>(this)));	// dwContext
	if (!hUrl) {
		// Error opening the URL.
		// TODO: Is InternetGetLastResponseInfo() usable here?
		int err = w32err_to_posix(GetLastError());
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}

	// Check if we got an HTTP response code.
	DWORD dwHttpStatusCode = 0;
	DWORD dwBufferLength = static_cast<DWORD>(sizeof(dwHttpStatusCode));
	if (HttpQueryInfo(hUrl.get(),		// hRequest
		HTTP_QUERY_STATUS_CODE |
		HTTP_QUERY_FLAG_NUMBER,		// dwInfoLevel
		&dwHttpStatusCode,		// lpBuffer
		&dwBufferLength,		// lpdwBufferLength
		0))				// lpdwIndex
	{
		// Received DWORD.
		if (dwBufferLength == static_cast<DWORD>(sizeof(dwHttpStatusCode))) {
			// Length is valid.
			// We're only accepting HTTP 200.
			if (dwHttpStatusCode != 200) {
				// Unexpected status code.
				return static_cast<int>(dwHttpStatusCode);
			}
		}
	}

	// Get mtime if it's available.
	SYSTEMTIME st_mtime;
	dwBufferLength = static_cast<DWORD>(sizeof(st_mtime));
	if (HttpQueryInfo(hUrl.get(),		// hRequest
		HTTP_QUERY_LAST_MODIFIED | 
		HTTP_QUERY_FLAG_SYSTEMTIME,	// dwInfoLevel
		&st_mtime,			// lpBuffer
		&dwBufferLength,		// lpdwBufferLength
		0))				// lpdwIndex
	{
		// Received SYSTEMTIME.
		if (dwBufferLength == static_cast<DWORD>(sizeof(st_mtime))) {
			// Length is valid.
			// FIXME: How to determine if the value is valid?
			m_mtime = SystemTimeToUnixTime(&st_mtime);
		}
	}

	// Get Content-Length.
	DWORD dwContentLength = 0;
	dwBufferLength = static_cast<DWORD>(sizeof(dwContentLength));
	if (HttpQueryInfo(hUrl.get(),		// hRequest
		HTTP_QUERY_CONTENT_LENGTH |
		HTTP_QUERY_FLAG_NUMBER,		// dwInfoLevel
		&dwContentLength,		// lpBuffer
		&dwBufferLength,		// lpdwBufferLength
		0))				// lpdwIndex
	{
		// Received DWORD.
		if (dwBufferLength == static_cast<DWORD>(sizeof(dwContentLength))) {
			// Length is valid.
			// Check if it's 0 or >= max size.
			if (dwContentLength == 0) {
				// Zero-length file. Not valid.
				return -ENOENT;	// handling as if it doesn't exist
			} else if (dwContentLength > m_maxSize) {
				// File is too big.
				return -ENOSPC;	// or -ENOMEM?
			}
		}
	}

	// Read the file.
	static constexpr DWORD BUF_SIZE_INCREMENT = 64U * 1024U;
	DWORD cur_increment;
	m_data.clear();
	if (dwContentLength > 0) {
		// Content-Length is known.
		// Start with the full Content-Length.
		m_data.reserve(dwContentLength);
		cur_increment = dwContentLength;
	} else {
		// Content-Length is unknown.
		// Start with 64 KB.
		m_data.reserve(BUF_SIZE_INCREMENT);
		cur_increment = BUF_SIZE_INCREMENT;
	}

	bool done = false;
	do {
		// Read the current buffer size increment.
		size_t prev_size = m_data.size();
		m_data.resize(prev_size + cur_increment);

		DWORD dwNumberOfBytesRead;
		if (InternetReadFile(hUrl.get(), &m_data[prev_size], cur_increment, &dwNumberOfBytesRead)) {
			// Successful read.
			if (dwNumberOfBytesRead == 0) {
				// EOF.
				m_data.resize(prev_size);
				done = true;
				break;
			} else if (dwNumberOfBytesRead < cur_increment) {
				// Read less than the buffer size increment.
				m_data.resize(prev_size + dwNumberOfBytesRead);
			}

			// Make sure we haven't exceeded the maximum buffer size.
			if (m_maxSize > 0) {
				// Maximum buffer size is set.
				if (m_data.size() > m_maxSize) {
					// Out of memory.
					m_data.clear();
					m_data.shrink_to_fit();
					return -ENOSPC;
				}
			}
		} else {
			// Read failed.
			int err = w32err_to_posix(GetLastError());
			if (err == 0) {
				err = EIO;
			}
			m_data.clear();
			m_data.shrink_to_fit();
			return -err;
		}

		// Continue reading in BUF_SIZE_INCREMENT chunks.
		cur_increment = BUF_SIZE_INCREMENT;
	} while (!done);

	// Finished downloading the file.
	// Return an error if no data was received.
	// TODO: Check `done`?
	return (m_data.empty() ? -ENOENT : 0);
}

} //namespace RpDownload

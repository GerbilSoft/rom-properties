/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * WinInetDownloader.cpp: WinInet-based file downloader.                   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WinInetDownloader.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32err.h"
#include "libwin32common/w32time.h"

// librpbase
#include "librpbase/common.h"

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

// Windows includes.
#include <wininet.h>

namespace RpDownload {

WinInetDownloader::WinInetDownloader()
	: super()
{ }

WinInetDownloader::WinInetDownloader(const TCHAR *url)
	: super(url)
{ }

WinInetDownloader::WinInetDownloader(const tstring &url)
	: super(url)
{ }

/**
 * Download the file.
 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
 */
int WinInetDownloader::download(void)
{
	// Reference: https://msdn.microsoft.com/en-us/library/ms775122(v=vs.85).aspx
	// TODO: IBindStatusCallback to enforce data size?
	// TODO: Check Content-Length to prevent large files in the first place?

	// Clear the previous download.
	m_data.clear();
	m_mtime = -1;

	HINTERNET hConnection = InternetOpen(
		m_userAgent.c_str(),		// lpszAgent
		INTERNET_OPEN_TYPE_PRECONFIG,	// dwAccessType
		nullptr,			// lpszProxy
		nullptr,			// lpszProxyBypass
		0);				// dwFlags
	if (!hConnection) {
		// Error opening a WinInet instance.
		int err = w32err_to_posix(GetLastError());
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}

	// Request the URL.
	HINTERNET hURL = InternetOpenUrl(
		hConnection,		// hInternet
		m_url.c_str(),		// lpszUrl (Latin-1 characters only!)
		nullptr,		// lpszHeaders
		0,			// dwHeaderLength
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
		INTERNET_FLAG_NO_AUTH |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_NO_UI,	// dwFlags
		reinterpret_cast<DWORD_PTR>(this));	// dwContext
	if (!hURL) {
		// Error opening the URL.
		// TODO: InternetGetLastResponseInfo()
		int err = w32err_to_posix(GetLastError());
		if (err == 0) {
			err = EIO;
		}
		InternetCloseHandle(hConnection);
		return -err;
	}

	// Get mtime if it's available.
	SYSTEMTIME st_mtime;
	DWORD dwBufferLength = static_cast<DWORD>(sizeof(st_mtime));
	if (HttpQueryInfo(hURL,			// hRequest
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

	// Read the file.
	// TODO: Reserve based on content length.
	static const DWORD BUF_SIZE_INCREMENT = 64*1024;
	m_data.clear();
	m_data.reserve(1048576);
	bool done = false;
	do {
		// Read up to 16 KB.
		size_t prev_size = m_data.size();
		m_data.resize(prev_size + BUF_SIZE_INCREMENT);

		DWORD dwNumberOfBytesRead;
		if (InternetReadFile(hURL, &m_data[prev_size], BUF_SIZE_INCREMENT, &dwNumberOfBytesRead)) {
			// Successful read.
			if (dwNumberOfBytesRead == 0) {
				// EOF.
				m_data.resize(prev_size);
				done = true;
				break;
			} else if (dwNumberOfBytesRead < BUF_SIZE_INCREMENT) {
				// Read less than the buffer size increment.
				m_data.resize(prev_size + dwNumberOfBytesRead);
			}
		} else {
			// Read failed.
			// TODO: Get the error.
			InternetCloseHandle(hURL);
			InternetCloseHandle(hConnection);
			m_data.clear();
			m_data.shrink_to_fit();
			return 1;
		}
	} while (!done);

	// Finished downloading the file.
	InternetCloseHandle(hURL);
	InternetCloseHandle(hConnection);
	return 0;
}

}

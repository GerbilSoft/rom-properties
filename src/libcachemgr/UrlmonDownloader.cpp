/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * UrlmonDownloader.cpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UrlmonDownloader.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
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
#include <urlmon.h>
#include <wininet.h>

// TODO: tcharx.h?
#ifndef _WIN32
# define _tfopen fopen
#endif /* !_WIN32 */

namespace RpDownload {

UrlmonDownloader::UrlmonDownloader()
	: super()
{ }

UrlmonDownloader::UrlmonDownloader(const TCHAR *url)
	: super(url)
{ }

UrlmonDownloader::UrlmonDownloader(const tstring &url)
	: super(url)
{ }

/**
 * Download the file.
 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
 */
int UrlmonDownloader::download(void)
{
	// Reference: https://msdn.microsoft.com/en-us/library/ms775122(v=vs.85).aspx
	// TODO: IBindStatusCallback to enforce data size?
	// TODO: Check Content-Length to prevent large files in the first place?
	// TODO: Replace with WinInet?
	// TODO: Set the User-Agent.

	// Buffer for cache filename.
	TCHAR szFileName[MAX_PATH];

	HRESULT hr = URLDownloadToCacheFile(nullptr, m_url.c_str(),
		szFileName, ARRAY_SIZE(szFileName),
		0, nullptr /* TODO */);

	if (FAILED(hr)) {
		// Failed to download the file.
		return hr;
	}

	// Open the cached file.
	FILE *f_cached = _tfopen(szFileName, _T("rb"));
	if (!f_cached) {
		// Unable to open the file.
		return -1;
	}

	// Get the cache information.
	// NOTE: GetUrlCacheEntryInfo() might fail with lastError == ERROR_INSUFFICIENT_BUFFER.
	// FIXME: amiibo.life downloads aren't found here. (CDN redirection issues?)
	DWORD cbCacheEntryInfo = 0;
	BOOL bRet = GetUrlCacheEntryInfo(m_url.c_str(), nullptr, &cbCacheEntryInfo);
	DWORD dwLastError = GetLastError();
	if (bRet || dwLastError == ERROR_INSUFFICIENT_BUFFER) {
		uint8_t *pCacheEntryInfoBuf =
			static_cast<uint8_t*>(malloc(cbCacheEntryInfo));
		if (!pCacheEntryInfoBuf) {
			// ENOMEM
			fclose(f_cached);
			return -ENOMEM;
		}
		INTERNET_CACHE_ENTRY_INFO *pCacheEntryInfo =
			reinterpret_cast<INTERNET_CACHE_ENTRY_INFO*>(pCacheEntryInfoBuf);
		bRet = GetUrlCacheEntryInfo(m_url.c_str(), pCacheEntryInfo, &cbCacheEntryInfo);
		if (bRet) {
			// Convert from Win32 FILETIME to Unix time.
			m_mtime = FileTimeToUnixTime(&pCacheEntryInfo->LastModifiedTime);
		}
		free(pCacheEntryInfoBuf);
	}

	// Get the file size.
	int ret = fseeko(f_cached, 0, SEEK_END);
	if (ret != 0) {
		// Seek error.
		fclose(f_cached);
		return -2;
	}
	const int64_t fileSize = ftello(f_cached);
	rewind(f_cached);

	// Read the file into the data buffer.
	// TODO: Max size limitation?
	m_data.resize(static_cast<size_t>(fileSize));
	size_t size = fread(m_data.data(), 1, static_cast<size_t>(fileSize), f_cached);
	fclose(f_cached);
	if (size != fileSize) {
		// Error reading the file.
		m_data.clear();
		m_data.shrink_to_fit();
		return -3;
	}

	// Data loaded.
	// TODO: Delete the cached file?
	return 0;
}

}

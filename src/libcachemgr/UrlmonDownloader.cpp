/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.cpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "UrlmonDownloader.hpp"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/w32time.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/TextFuncs_wchar.hpp"
#include "librpbase/file/RpFile.hpp"
using LibRpBase::IRpFile;
using LibRpBase::RpFile;

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

// Windows includes.
#include <urlmon.h>
#include <wininet.h>

namespace LibCacheMgr {

UrlmonDownloader::UrlmonDownloader()
	: super()
{ }

UrlmonDownloader::UrlmonDownloader(const char *url)
	: super(url)
{ }

UrlmonDownloader::UrlmonDownloader(const string &url)
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

	// Buffer for cache filename.
	TCHAR szFileName[MAX_PATH];

	const tstring t_url = U82T_s(m_url);
	HRESULT hr = URLDownloadToCacheFile(nullptr, t_url.c_str(),
		szFileName, ARRAY_SIZE(szFileName),
		0, nullptr /* TODO */);

	if (FAILED(hr)) {
		// Failed to download the file.
		return hr;
	}

	// Open the cached file.
	RpFile *const file = new RpFile(T2U8(szFileName), RpFile::FM_OPEN_READ);
	if (!file->isOpen()) {
		// Unable to open the file.
		file->unref();
		return -1;
	}

	// Get the cache information.
	// NOTE: GetUrlCacheEntryInfo() might fail with lastError == ERROR_INSUFFICIENT_BUFFER.
	// FIXME: amiibo.life downloads aren't found here. (CDN redirection issues?)
	DWORD cbCacheEntryInfo = 0;
	BOOL bRet = GetUrlCacheEntryInfo(t_url.c_str(), nullptr, &cbCacheEntryInfo);
	DWORD dwLastError = GetLastError();
	if (bRet || dwLastError == ERROR_INSUFFICIENT_BUFFER) {
		uint8_t *pCacheEntryInfoBuf =
			static_cast<uint8_t*>(malloc(cbCacheEntryInfo));
		if (!pCacheEntryInfoBuf) {
			// ENOMEM
			file->unref();
			return -ENOMEM;
		}
		INTERNET_CACHE_ENTRY_INFO *pCacheEntryInfo =
			reinterpret_cast<INTERNET_CACHE_ENTRY_INFO*>(pCacheEntryInfoBuf);
		bRet = GetUrlCacheEntryInfo(t_url.c_str(), pCacheEntryInfo, &cbCacheEntryInfo);
		if (bRet) {
			// Convert from Win32 FILETIME to Unix time.
			m_mtime = FileTimeToUnixTime(&pCacheEntryInfo->LastModifiedTime);
		}
		free(pCacheEntryInfoBuf);
	}

	// Read the file into the data buffer.
	const int64_t fileSize = file->size();
	m_data.resize(static_cast<size_t>(fileSize));
	size_t ret = file->read(m_data.data(), static_cast<size_t>(fileSize));
	file->unref();
	if (ret != fileSize) {
		// Error reading the file.
		m_data.clear();
		m_data.shrink_to_fit();
		return -2;
	}

	// Data loaded.
	// TODO: Delete the cached file?
	return 0;
}

}

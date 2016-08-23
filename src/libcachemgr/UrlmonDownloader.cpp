/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.cpp: urlmon-based file downloader.                     *
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

#include "stdafx.h"
#include "UrlmonDownloader.hpp"

#include "libromdata/TextFuncs.hpp"
#include "libromdata/RpFile.hpp"
using LibRomData::IRpFile;
using LibRomData::RpFile;
using LibRomData::rp_string;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// Windows includes.
#include <urlmon.h>

namespace LibCacheMgr {

UrlmonDownloader::UrlmonDownloader()
	: IDownloader()
{ }

UrlmonDownloader::UrlmonDownloader(const rp_char *url)
	: IDownloader(url)
{ }

UrlmonDownloader::UrlmonDownloader(const rp_string &url)
	: IDownloader(url)
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
	wchar_t szFileName[MAX_PATH];

#ifndef RP_UTF16
	// FIXME: RP_UTF8 support?
	#error UrlmonDownloader only supports RP_UTF16.
#endif

	HRESULT hr = URLDownloadToCacheFile(nullptr,
		reinterpret_cast<const wchar_t*>(m_url.c_str()),
		szFileName, sizeof(szFileName)/sizeof(szFileName[0]),
		0, nullptr /* TODO */);

	if (FAILED(hr)) {
		// Failed to download the file.
		return hr;
	}

	// Open the cached file.
	// TODO: UTF-8 support.
	IRpFile *file = new RpFile(reinterpret_cast<const rp_char*>(szFileName), RpFile::FM_OPEN_READ);
	if (!file || !file->isOpen()) {
		// Unable to open the file.
		delete file;
		return -1;
	}
	// TODO: Remove this after Gdiplus testing.
	m_cacheFile = szFileName;

	// Read the file into the data buffer.
	// TODO: malloc()'d buffer to prevent initialization?
	int64_t fileSize = file->fileSize();
	m_data.resize((size_t)fileSize);
	size_t ret = file->read(m_data.data(), (size_t)fileSize);
	if (ret != fileSize) {
		// Error reading the file.
		m_data.clear();
		return -2;
	}

	// Data loaded.
	// TODO: Delete the cached file?
	return 0;
}

}

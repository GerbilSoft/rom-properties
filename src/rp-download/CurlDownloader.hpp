/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CurlDownloader.hpp: libcurl-based file downloader.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IDownloader.hpp"

namespace RpDownload {

class CurlDownloader final : public IDownloader
{
public:
	CurlDownloader() = default;
	explicit CurlDownloader(const TCHAR *url);
	explicit CurlDownloader(const std::tstring &url);

private:
	typedef IDownloader super;
	RP_DISABLE_COPY(CurlDownloader)

protected:
	/**
	 * Internal cURL data write function.
	 * @param ptr Data to write.
	 * @param size Element size.
	 * @param nmemb Number of elements.
	 * @param userdata m_data pointer.
	 * @return Number of bytes written.
	 */
	static size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata);

	/**
	 * Internal cURL header parsing function.
	 * @param ptr Pointer to header data. (NOT necessarily null-terminated!)
	 * @param size Element size.
	 * @param nitems Number of elements.
	 * @param userdata m_data pointer.
	 * @return Amount of data processed, or 0 on error.
	 */
	static size_t parse_header(char *ptr, size_t size, size_t nitems, void *userdata);

public:
	/**
	 * Download the file.
	 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
	 */
	int download(void) final;
};

} //namespace RpDownload

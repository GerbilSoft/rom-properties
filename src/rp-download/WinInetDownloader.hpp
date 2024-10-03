/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * WinInetDownloader.hpp: WinInet-based file downloader.                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IDownloader.hpp"

namespace RpDownload {

class WinInetDownloader final : public IDownloader
{
public:
	WinInetDownloader() = default;
	explicit WinInetDownloader(const TCHAR *url);
	explicit WinInetDownloader(const std::tstring &url);

private:
	typedef IDownloader super;
	RP_DISABLE_COPY(WinInetDownloader)

public:
	/**
	 * Download the file.
	 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
	 */
	int download(void) final;
};

} //namespace RpDownload

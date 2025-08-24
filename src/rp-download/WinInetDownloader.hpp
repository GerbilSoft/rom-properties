/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * WinInetDownloader.hpp: WinInet-based file downloader.                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
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
	 * Get the name of the IDownloader implementation.
	 * @return Name
	 */
	const TCHAR *name(void) const final;

	/**
	 * Is this IDownloader object usable?
	 * @return True if it's usable; false if it's not.
	 */
	bool isUsable(void) const final;

	/**
	 * Download the file.
	 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
	 */
	int download(void) final;
};

} //namespace RpDownload

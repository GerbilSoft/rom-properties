/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * WinInetDownloader.hpp: WinInet-based file downloader.                   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_WININETDOWNLOADER_HPP__
#define __ROMPROPERTIES_RP_DOWNLOAD_WININETDOWNLOADER_HPP__

#include "IDownloader.hpp"

namespace RpDownload {

class WinInetDownloader : public IDownloader
{
	public:
		WinInetDownloader();
		explicit WinInetDownloader(const TCHAR *url);
		explicit WinInetDownloader(const std::tstring &url);

	private:
		typedef IDownloader super;
		RP_DISABLE_COPY(WinInetDownloader)

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		int download(void) final;
};

}

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_WININETDOWNLOADER_HPP__ */

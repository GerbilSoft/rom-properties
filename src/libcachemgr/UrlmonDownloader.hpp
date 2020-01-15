/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * UrlmonDownloader.hpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_URLMONDOWNLOADER_HPP__
#define __ROMPROPERTIES_RP_DOWNLOAD_URLMONDOWNLOADER_HPP__

#include "IDownloader.hpp"

namespace RpDownload {

class UrlmonDownloader : public IDownloader
{
	public:
		UrlmonDownloader();
		explicit UrlmonDownloader(const TCHAR *url);
		explicit UrlmonDownloader(const std::tstring &url);

	private:
		typedef IDownloader super;
		RP_DISABLE_COPY(UrlmonDownloader)

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		int download(void) final;
};

}

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_URLMONDOWNLOADER_HPP__ */

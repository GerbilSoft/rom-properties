/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.hpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__

#include "IDownloader.hpp"

namespace LibCacheMgr {

class UrlmonDownloader : public IDownloader
{
	public:
		UrlmonDownloader();
		explicit UrlmonDownloader(const char *url);
		explicit UrlmonDownloader(const std::string &url);

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

#endif /* __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__ */

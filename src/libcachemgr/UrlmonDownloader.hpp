/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.hpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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

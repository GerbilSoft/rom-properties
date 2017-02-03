/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.hpp: urlmon-based file downloader.                     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__

#include "IDownloader.hpp"

namespace LibCacheMgr {

class UrlmonDownloader : public IDownloader
{
	public:
		UrlmonDownloader();
		explicit UrlmonDownloader(const rp_char *url);
		explicit UrlmonDownloader(const LibRomData::rp_string &url);

	private:
		typedef IDownloader super;
		RP_DISABLE_COPY(UrlmonDownloader)

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		virtual int download(void) override final;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__ */

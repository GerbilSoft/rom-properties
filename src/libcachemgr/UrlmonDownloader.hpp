/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * UrlmonDownloader.hpp: urlmon-based file downloader.                     *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__

#include "libromdata/config.libromdata.h"

// C includes.
#include <stdint.h>

// C++ includes.
#include <vector>

namespace LibCacheMgr {

class UrlmonDownloader
{
	public:
		UrlmonDownloader();
		explicit UrlmonDownloader(const rp_char *url);
		explicit UrlmonDownloader(const LibRomData::rp_string &url);

	private:
		UrlmonDownloader(const UrlmonDownloader &);
		UrlmonDownloader &operator=(const UrlmonDownloader &);

	public:
		/** Properties. **/

		/**
		 * Is a download in progress?
		 * @return True if a download is in progress.
		 */
		bool isInProgress(void) const;

		/**
		 * Get the current URL.
		 * @return URL.
		 */
		LibRomData::rp_string url(void) const;

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const rp_char *url);

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const LibRomData::rp_string &url);

		/**
		 * Get the maximum buffer size. (0 == unlimited)
		 * @return Maximum buffer size.
		 */
		size_t maxSize(void) const;

		/**
		 * Set the maximum buffer size. (0 == unlimited)
		 * @param maxSize Maximum buffer size.
		 */
		void setMaxSize(size_t maxSize);

		/**
		 * Get the size of the data.
		 * @return Size of the data.
		 */
		size_t dataSize(void) const;

		/**
		 * Get a pointer to the start of the data.
		 * @return Pointer to the start of the data.
		 */
		const uint8_t *data(void) const;

		/**
		 * Clear the data.
		 */
		void clear(void);

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		int download(void);

	protected:
		LibRomData::rp_string m_url;

		// TODO: Use C malloc()/realloc()?
		// std::vector::resize() forces initialization.
		std::vector<uint8_t> m_data;

		bool m_inProgress;	// Set when downloading.
		size_t m_maxSize;	// Maximum buffer size. (0 == unlimited)

	public:
		// FIXME: Remove this.
		// Only for Gdiplus testing.
		std::wstring m_cacheFile;
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_URLMONDOWNLOADER_HPP__ */

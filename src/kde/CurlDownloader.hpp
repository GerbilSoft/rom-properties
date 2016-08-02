/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * CurlDownloader.hpp: libcurl-based file downloader.                      *
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

#ifndef __ROMPROPERTIES_KDE_CURLDOWNLOADER_HPP__
#define __ROMPROPERTIES_KDE_CURLDOWNLOADER_HPP__

// TODO: Split out into a library shared by
// both KDE and GTK+ later.

#include "libromdata/config.libromdata.h"

// C includes.
#include <stdint.h>

// C++ includes.
#include <vector>

class CurlDownloader
{
	public:
		CurlDownloader();
		explicit CurlDownloader(const rp_char *url);
		explicit CurlDownloader(const LibRomData::rp_string &url);

	private:
		CurlDownloader(const CurlDownloader &);
		CurlDownloader &operator=(const CurlDownloader &);

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
		 * Get the proxy server.
		 * @return Proxy server URL.
		 */
		LibRomData::rp_string proxyUrl(void) const;

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
		 */
		void setProxyUrl(const rp_char *proxyUrl);

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
		 */
		void setProxyUrl(const LibRomData::rp_string &proxyUrl);

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

	protected:
		/**
		 * Internal cURL data write function.
		 * @param ptr Data to write.
		 * @param size Element size.
		 * @param nmemb Number of elements.
		 * @param userdata m_data pointer.
		 * @return Number of bytes written.
		 */
		static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata);

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		int download(void);

	protected:
		LibRomData::rp_string m_url;
		LibRomData::rp_string m_proxyUrl;

		// TODO: Use C malloc()/realloc()?
		// std::vector::resize() forces initialization.
		std::vector<uint8_t> m_data;

		bool m_inProgress;	// Set when downloading.
		size_t m_maxSize;	// Maximum buffer size. (0 == unlimited)
};

#endif /* __ROMPROPERTIES_KDE_CURLDOWNLOADER_HPP__ */

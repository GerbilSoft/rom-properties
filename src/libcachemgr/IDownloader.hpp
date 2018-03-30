/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * IDownloader.hpp: Downloader interface.                                  *
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

#ifndef __ROMPROPERTIES_LIBCACHEMGR_IDOWNLOADER_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_IDOWNLOADER_HPP__

// librpbase
#include "librpbase/common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>

// C++ includes.
#include <string>

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibCacheMgr {

class IDownloader
{
	public:
		IDownloader();
		explicit IDownloader(const char *url);
		explicit IDownloader(const std::string &url);
		virtual ~IDownloader();

	private:
		RP_DISABLE_COPY(IDownloader)

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
		std::string url(void) const;

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const char *url);

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const std::string &url);

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

	public:
		/** Proxy server functions. **/
		// NOTE: This is only useful for downloaders that
		// can't retrieve the system proxy server normally.

		/**
		 * Get the proxy server.
		 * @return Proxy server URL.
		 */
		std::string proxyUrl(void) const;

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use nullptr or blank string for default settings.)
		 */
		void setProxyUrl(const char *proxyUrl);

		/**
		 * Set the proxy server.
		 * @param proxyUrl Proxy server URL. (Use blank string for default settings.)
		 */
		void setProxyUrl(const std::string &proxyUrl);

	public:
		/** Data accessors. **/

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
		 * Get the Last-Modified time.
		 * @return Last-Modified time, or -1 if none was set by the server.
		 */
		time_t mtime(void) const;

		/**
		 * Clear the data.
		 */
		void clear(void);

	public:
		/**
		 * Download the file.
		 * @return 0 on success; non-zero on error. [TODO: HTTP error codes?]
		 */
		virtual int download(void) = 0;

	protected:
		std::string m_url;
		std::string m_proxyUrl;

		// Uninitialized vector class.
		// Reference: http://andreoffringa.org/?q=uvector
		ao::uvector<uint8_t> m_data;

		// Last-Modified time.
		time_t m_mtime;

		bool m_inProgress;	// Set when downloading.
		size_t m_maxSize;	// Maximum buffer size. (0 == unlimited)
};

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CURLDOWNLOADER_HPP__ */

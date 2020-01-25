/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * IDownloader.hpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_RP_DOWNLOAD_IDOWNLOADER_HPP__
#define __ROMPROPERTIES_RP_DOWNLOAD_IDOWNLOADER_HPP__

// librpbase
#include "librpbase/common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>
#include <ctime>

// C++ includes.
#include <string>

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

// tcharx
#include "tcharx.h"

namespace RpDownload {

class IDownloader
{
	public:
		IDownloader();
		explicit IDownloader(const TCHAR *url);
		explicit IDownloader(const std::tstring &url);
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
		std::tstring url(void) const;

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const TCHAR *url);

		/**
		 * Set the URL.
		 * @param url New URL.
		 */
		void setUrl(const std::tstring &url);

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
		 * @return 0 on success; negative POSIX error code, positive HTTP status code on error.
		 */
		virtual int download(void) = 0;

	private:
		/**
		 * Create the User-Agent value.
		 */
		void createUserAgent(void);

	protected:
		std::tstring m_url;

		// Uninitialized vector class.
		// Reference: http://andreoffringa.org/?q=uvector
		ao::uvector<uint8_t> m_data;

		// Last-Modified time.
		time_t m_mtime;

		bool m_inProgress;	// Set when downloading.
		size_t m_maxSize;	// Maximum buffer size. (0 == unlimited)

		// User-Agent.
		std::tstring m_userAgent;
};

}

#endif /* __ROMPROPERTIES_RP_DOWNLOAD_IDOWNLOADER_HPP__ */

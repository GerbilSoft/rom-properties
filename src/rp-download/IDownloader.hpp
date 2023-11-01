/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * IDownloader.hpp: Downloader interface.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Common definitions, including function attributes.
#include "common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>
#include <ctime>

// Uninitialized vector class
#include "uvector.h"

// tcharx
#include "tcharx.h"

namespace RpDownload {

class NOVTABLE IDownloader
{
public:
	explicit IDownloader();
	explicit IDownloader(const TCHAR *url);
	explicit IDownloader(const std::tstring &url);
	virtual ~IDownloader() = default;

private:
	RP_DISABLE_COPY(IDownloader)

public:
	/** Properties **/

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

	/**
	 * Get the If-Modified-Since request timestamp.
	 * @return If-Modified-Since timestamp (-1 for none)
	 */
	time_t ifModifiedSince(void) const;

	/**
	 * Set the If-Modified-Since request timestamp.
	 * @param timestamp If-Modified-Since timestamp (-1 for none)
	 */
	void setIfModifiedSince(time_t timestamp);

public:
	/** Data accessors **/

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

	/**
	 * Get the OS release information.
	 * @return OS release information, or empty string if not available.
	 */
	std::tstring getOSRelease(void);

protected:
	std::tstring m_url;

	// Uninitialized vector class.
	// Reference: http://andreoffringa.org/?q=uvector
	rp::uvector<uint8_t> m_data;

	time_t m_mtime;			// Last-Modified response
	time_t m_if_modified_since;	// If-Modified-Since request

	size_t m_maxSize;		// Maximum buffer size. (0 == unlimited)
	std::tstring m_userAgent;	// User-Agent

	bool m_inProgress;		// Set when downloading
};

} //namespace RpDownload

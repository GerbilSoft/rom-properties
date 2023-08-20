/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RpFile_kio.hpp: IRpFile implementation using KIO.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// FIXME: Dolphin ends up hanging for some reason...
#if 0

// NOTE: Only available for KDE Frameworks 5.
// KDE 4.x's KIO doesn't have KIO::open().
#include <QtCore/qglobal.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#define HAVE_RPFILE_KIO 1

// librpfile
#include "librpfile/IRpFile.hpp"

// Qt includes.
#include <QtCore/QObject>
#include <QtCore/QUrl>

class RpFileKioPrivate;
class RpFileKio : public QObject, public LibRpFile::IRpFile
{
	Q_OBJECT

	public:
		/**
		 * Open a file.
		 * NOTE: Files are always opened as read-only in binary mode.
		 * @param uri KIO URI.
		 */
		RpFileKio(const char *uri);
		RpFileKio(const std::string &uri);
		RpFileKio(const QUrl &uri);
	private:
		void init(void);
	public:
		~RpFileKio() override;

	private:
		typedef LibRpFile::IRpFile super;
		RP_DISABLE_COPY(RpFileKio)
	protected:
		friend class RpFileKioPrivate;
		RpFileKioPrivate *const d_ptr;

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final;

		/**
		 * Close the file.
		 */
		void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpFileKio; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		off64_t tell(void) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		off64_t size(void) final;

		/**
		 * Get the filename.
		 * NOTE: For RpFileKio, this returns a KIO URI.
		 * @return Filename. (May be nullptr if the filename is not available.)
		 */
		const char *filename(void) const final;

	signals:
		// Event loop handling.
		// Reference: https://github.com/KDE/kio/blob/master/autotests/jobremotetest.cpp
		void exitLoop(void);
};

#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

#endif // FIXME: Dolphin ends up hanging for some reason...

/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RpFile_kio.hpp: IRpFile implementation using KIO.                       *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_RPFILE_KIO_HPP__
#define __ROMPROPERTIES_KDE_RPFILE_KIO_HPP__

// librpbase
#include "librpbase/file/IRpFile.hpp"

// Qt includes.
// TODO: KUrl for Qt4.
#include <QtCore/QObject>
#include <QtCore/QUrl>

class RpFileKioPrivate;
class RpFileKio : public QObject, public LibRpBase::IRpFile
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
	protected:
		virtual ~RpFileKio();	// call unref() instead

	private:
		typedef LibRpBase::IRpFile super;
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
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpFileKio; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Truncate the file.
		 * (NOTE: Not valid for RpFileKio; this will always return -1.)
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(int64_t size = 0) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		int64_t size(void) final;

		/**
		 * Get the filename.
		 * NOTE: For RpFileKio, this returns a KIO URI.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;

	signals:
		// Event loop handling.
		// Reference: https://github.com/KDE/kio/blob/master/autotests/jobremotetest.cpp
		void exitLoop(void);
};

#endif /* __ROMPROPERTIES_KDE_RPFILE_KIO_HPP__ */

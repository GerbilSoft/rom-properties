/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpQByteArrayFile.hpp: IRpFile implementation using a QByteArray.        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpfile
#include "librpfile/IRpFile.hpp"

// Qt includes.
#include <QtCore/QByteArray>

class RpQByteArrayFile : public LibRpFile::IRpFile
{
public:
	/**
	 * Open an IRpFile backed by a QByteArray.
	 * The resulting IRpFile is writable.
	 */
	RpQByteArrayFile();

private:
	typedef LibRpFile::IRpFile super;
	RP_DISABLE_COPY(RpQByteArrayFile)

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

	/**
	 * Truncate the file.
	 * @param size New size. (default is 0)
	 * @return 0 on success; -1 on error.
	 */
	int truncate(off64_t size = 0) final;

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	off64_t size(void) final;

public:
	/** RpQByteArrayFile-specific functions **/

	/**
	 * Get the underlying QByteArray.
	 * @return QByteArray.
	 */
	QByteArray qByteArray(void) const
	{
		return m_byteArray;
	}

protected:
	QByteArray m_byteArray;
	size_t m_pos;		// Current position.
};

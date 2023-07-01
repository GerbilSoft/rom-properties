/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpQByteArrayFile.cpp: IRpFile implementation using a QByteArray.        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpQByteArrayFile.hpp"

// C++ STL classes.
using std::string;

/**
 * Open an IRpFile backed by a QByteArray.
 * The resulting IRpFile is writable.
 */
RpQByteArrayFile::RpQByteArrayFile()
	: super()
	, m_pos(0)
{
	// Reserve at least 16 KB.
	m_byteArray.reserve(16*1024);
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpQByteArrayFile::isOpen(void) const
{
	// RpQByteArrayFile is always open.
	return true;
}

/**
 * Close the file.
 */
void RpQByteArrayFile::close(void)
{
	// Not really useful...
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpQByteArrayFile::read(void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// Check if size is in bounds.
	// NOTE: Need to use a signed comparison here.
	if (static_cast<off64_t>(m_pos) > (static_cast<off64_t>(m_byteArray.size()) - static_cast<off64_t>(size))) {
		// Not enough data.
		// Copy whatever's left in the buffer.
		size = m_byteArray.size() - m_pos;
	}

	// Copy the data.
	const char *const buf = m_byteArray.constData();
	memcpy(ptr, &buf[m_pos], size);
	m_pos += size;
	return size;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpQByteArrayFile::write(const void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not writing anything...
		return 0;
	}

	// Do we need to expand the QByteArray?
	const off64_t req_size = static_cast<off64_t>(m_byteArray.size()) + size;
	if (req_size < 0) {
		// Overflow...
		return 0;
	} else if (req_size > m_byteArray.size()) {
		// Need to expand the QByteArray.
		m_byteArray.resize(req_size);
	}

	// Copy the data to the buffer.
	char *const buf = m_byteArray.data();
	memcpy(&buf[m_pos], ptr, size);
	m_pos += size;
	return size;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpQByteArrayFile::seek(off64_t pos)
{
	// NOTE: m_pos is size_t, since it's referring to
	// a position within a memory buffer.
	if (pos <= 0) {
		m_pos = 0;
	} else if (pos >= m_byteArray.size()) {
		m_pos = m_byteArray.size();
	} else {
		m_pos = static_cast<size_t>(pos);
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpQByteArrayFile::tell(void)
{
	return (off64_t)m_pos;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpQByteArrayFile::truncate(off64_t size)
{
	m_byteArray.resize(size);
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpQByteArrayFile::size(void)
{
	return m_byteArray.size();
}

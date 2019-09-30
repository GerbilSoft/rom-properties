/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpMemFile.cpp: IRpFile implementation using a memory buffer.            *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpMemFile.hpp"

// C++ STL classes.
using std::string;

namespace LibRpBase {

/**
 * Open an IRpFile backed by memory.
 * The resulting IRpFile is read-only.
 *
 * NOTE: The memory buffer is NOT copied; it must remain
 * valid as long as this object is still open.
 *
 * @param buf Memory buffer.
 * @param size Size of memory buffer.
 */
RpMemFile::RpMemFile(const void *buf, size_t size)
	: super()
	, m_buf(buf)
	, m_size(static_cast<int64_t>(size))
	, m_pos(0)
{
	if (!buf) {
		// No buffer specified.
		m_lastError = EBADF;
	}
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpMemFile::isOpen(void) const
{
	return (m_buf != nullptr);
}

/**
 * Close the file.
 */
void RpMemFile::close(void)
{
	m_buf = nullptr;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpMemFile::read(void *ptr, size_t size)
{
	if (!m_buf) {
		m_lastError = EBADF;
		return 0;
	}

	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// Check if size is in bounds.
	// NOTE: Need to use a signed comparison here.
	if (static_cast<int64_t>(m_pos) > (static_cast<int64_t>(m_size) - static_cast<int64_t>(size))) {
		// Not enough data.
		// Copy whatever's left in the buffer.
		size = m_size - m_pos;
	}

	// Copy the data.
	const uint8_t *const buf = static_cast<const uint8_t*>(m_buf);
	memcpy(ptr, &buf[m_pos], size);
	m_pos += size;
	return size;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for RpMemFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpMemFile::write(const void *ptr, size_t size)
{
	// Not a valid operation for RpMemFile.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpMemFile::seek(int64_t pos)
{
	if (!m_buf) {
		m_lastError = EBADF;
		return -1;
	}

	// NOTE: m_pos is size_t, since it's referring to
	// a position within a memory buffer.
	if (pos <= 0) {
		m_pos = 0;
	} else if (static_cast<size_t>(pos) >= m_size) {
		m_pos = m_size;
	} else {
		m_pos = static_cast<size_t>(pos);
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
int64_t RpMemFile::tell(void)
{
	if (!m_buf) {
		m_lastError = EBADF;
		return 0;
	}

	return (int64_t)m_pos;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpMemFile::truncate(int64_t size)
{
	// Not supported.
	// TODO: Writable RpMemFile?
	RP_UNUSED(size);
	m_lastError = ENOTSUP;
	return -1;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
int64_t RpMemFile::size(void)
{
	if (!m_buf) {
		m_lastError = EBADF;
		return -1;
	}

	return m_size;
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
string RpMemFile::filename(void) const
{
	// TODO: Implement this?
	return string();
}

}

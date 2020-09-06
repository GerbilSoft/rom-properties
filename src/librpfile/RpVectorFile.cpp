/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpVectorFile.cpp: IRpFile implementation using an std::vector.          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpVectorFile.hpp"

// C++ STL classes.
using std::string;

namespace LibRpFile {

/**
 * Open an IRpFile backed by an std::vector.
 * The resulting IRpFile is writable.
 */
RpVectorFile::RpVectorFile()
	: super()
	, m_pos(0)
{
	// Reserve at least 16 KB.
	m_vector.reserve(16*1024);

	// RpVectorFile is writable.
	m_isWritable = true;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpVectorFile::isOpen(void) const
{
	// RpVectorFile is always open.
	return true;
}

/**
 * Close the file.
 */
void RpVectorFile::close(void)
{
	// Not really useful...
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpVectorFile::read(void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// Check if size is in bounds.
	// NOTE: Need to use a signed comparison here.
	if (static_cast<off64_t>(m_pos) > (static_cast<off64_t>(m_vector.size()) - static_cast<off64_t>(size))) {
		// Not enough data.
		// Copy whatever's left in the buffer.
		size = m_vector.size() - m_pos;
	}

	// Copy the data.
	const uint8_t *const buf = m_vector.data();
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
size_t RpVectorFile::write(const void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not writing anything...
		return 0;
	}

	// Do we need to expand the std::vector?
	off64_t req_size = static_cast<off64_t>(m_vector.size()) + size;
	if (req_size < 0) {
		// Overflow...
		return 0;
	} else if (req_size > static_cast<off64_t>(m_vector.size())) {
		// Need to expand the std::vector.
		m_vector.resize(req_size);
	}

	// Copy the data to the buffer.
	uint8_t *const buf = m_vector.data();
	memcpy(&buf[m_pos], ptr, size);
	m_pos += size;
	return size;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int RpVectorFile::seek(off64_t pos)
{
	// NOTE: m_pos is size_t, since it's referring to
	// a position within a memory buffer.
	if (pos <= 0) {
		m_pos = 0;
	} else if (pos >= static_cast<off64_t>(m_vector.size())) {
		m_pos = m_vector.size();
	} else {
		m_pos = pos;
	}

	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpVectorFile::tell(void)
{
	return (off64_t)m_pos;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int RpVectorFile::truncate(off64_t size)
{
	m_vector.resize(size);
	return 0;
}

/**
 * Flush buffers.
 * This operation only makes sense on writable files.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpVectorFile::flush(void)
{
	// Ignore flush operations, since RpVectorFile is entirely in memory.
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpVectorFile::size(void)
{
	return m_vector.size();
}

/**
 * Get the filename.
 * @return Filename. (May be empty if the filename is not available.)
 */
string RpVectorFile::filename(void) const
{
	// TODO: Implement this?
	return string();
}

}

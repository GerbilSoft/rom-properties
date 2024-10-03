/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * MemFile.cpp: IRpFile implementation using a memory buffer.              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MemFile.hpp"

// C++ STL classes.
using std::string;

namespace LibRpFile {

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
MemFile::MemFile(const void *buf, size_t size)
	: m_buf(buf)
	, m_size(static_cast<off64_t>(size))
	, m_pos(0)
	, m_filename(nullptr)
{
	assert(buf != nullptr);
	assert(size != 0);
	if (!buf || size == 0) {
		// No buffer specified.
		m_lastError = EBADF;
	}
}

/**
 * Internal constructor for use by subclasses.
 * This initializes everything to nullptr.
 */
MemFile::MemFile()
	: m_buf(nullptr)
	, m_size(0)
	, m_pos(0)
	, m_filename(nullptr)
{ }

MemFile::~MemFile()
{
	free(m_filename);
}

/**
 * Close the file.
 */
void MemFile::close(void)
{
	m_buf = nullptr;
	m_size = 0;
	m_pos = 0;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t MemFile::read(void *ptr, size_t size)
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
	if (static_cast<off64_t>(m_pos) > (static_cast<off64_t>(m_size) - static_cast<off64_t>(size))) {
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
 * (NOTE: Not valid for MemFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t MemFile::write(const void *ptr, size_t size)
{
	// Not a valid operation for MemFile.
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
int MemFile::seek(off64_t pos)
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
off64_t MemFile::tell(void)
{
	if (!m_buf) {
		m_lastError = EBADF;
		return 0;
	}

	return static_cast<off64_t>(m_pos);
}

/** MemFile functions **/

/**
 * Set the filename.
 * @param filename Filename
 */
void MemFile::setFilename(const char *filename)
{
	free(m_filename);
	m_filename = (filename && filename[0] != '\0') ? strdup(filename) : nullptr;
}

/**
 * Set the filename.
 * @param filename Filename
 */
void MemFile::setFilename(const std::string &filename)
{
	free(m_filename);
	m_filename = !filename.empty() ? strdup(filename.c_str()) : nullptr;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * DualFile.cpp: Special wrapper for handling a split file as one.         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DualFile.hpp"

// C++ STL classes
using std::string;

namespace LibRpFile {

/**
 * Open two files and handle them as if they're a single file.
 * The resulting IRpFile is read-only.
 *
 * @param file0 First file.
 * @param file1 Second file.
 */
DualFile::DualFile(const IRpFilePtr &file0, const IRpFilePtr &file1)
	: m_file{file0, file1}
	, m_pos(0)
{
	assert((bool)file0);
	assert((bool)file1);
	if (!file0 || !file1) {
		// At least one file is missing.
		m_lastError = EBADF;
		m_fullSize = 0;
		m_file[0].reset();
		m_file[1].reset();
		m_size.fill(0);
		return;
	}

	m_size[0] = file0->size();
	m_size[1] = file1->size();

	m_fullSize = m_size[0] + m_size[1];
}

/**
 * Internal constructor for use by subclasses.
 * This initializes everything to nullptr.
 */
DualFile::DualFile()
	: m_size{0, 0}
	, m_fullSize(0)
	, m_pos(0)
{}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool DualFile::isOpen(void) const
{
	return ((bool)m_file[0] && (bool)m_file[1]);
}

/**
 * Close the file.
 */
void DualFile::close(void)
{
	m_file[0].reset();
	m_file[1].reset();
	m_size.fill(0);

	m_fullSize = 0;
	m_pos = 0;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t DualFile::read(void *ptr, size_t size)
{
	if (!m_file[0] || !m_file[1]) {
		m_lastError = EBADF;
		return 0;
	}

	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// uint8_t pointer access.
	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);

	// Check if the read is fully within file 0.
	if (m_pos < m_size[0] && ((m_pos + static_cast<off64_t>(size)) < m_size[0])) {
		// Read is fully within file 0.
		const size_t sz_read = m_file[0]->seekAndRead(m_pos, ptr8, size);
		m_lastError = m_file[0]->lastError();
		m_pos += sz_read;
		return sz_read;
	}

	// Check if the read is fully within file 1.
	if (m_pos >= m_size[0]) {
		// Fully within file 1.
		// NOTE: If the size is past the bounds, the read will be truncated.
		const size_t sz_read = m_file[1]->seekAndRead(m_pos - m_size[0], ptr8, size);
		m_lastError = m_file[1]->lastError();
		m_pos += sz_read;
		return sz_read;
	}

	// Read crosses the boundary between file 0 and file 1.

	// File 0 portion.
	const size_t file0_sz = static_cast<size_t>(m_size[0] - m_pos);
	size_t sz0_read = m_file[0]->seekAndRead(m_pos, ptr8, file0_sz);
	m_lastError = m_file[0]->lastError();
	m_pos += sz0_read;
	if (sz0_read != file0_sz) {
		// Short read.
		return sz0_read;
	}
	size -= sz0_read;
	ptr8 += sz0_read;

	// File 1 portion.
	size_t sz1_read = m_file[1]->seekAndRead(0, ptr8, size);
	m_lastError = m_file[1]->lastError();
	m_pos += sz1_read;

	return (sz0_read + sz1_read);
}

/**
 * Write data to the file.
 * (NOTE: Not valid for DualFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t DualFile::write(const void *ptr, size_t size)
{
	// Not a valid operation for DualFile.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int DualFile::seek(off64_t pos, SeekWhence whence)
{
	if (!m_file[0] || !m_file[1]) {
		m_lastError = EBADF;
		return -1;
	}

	pos = adjust_file_pos_for_whence(pos, whence, m_pos, m_fullSize);
	m_pos = constrain_file_pos(pos, m_fullSize);
	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t DualFile::tell(void)
{
	if (!m_file[0] || !m_file[1]) {
		m_lastError = EBADF;
		return 0;
	}

	return m_pos;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t DualFile::size(void)
{
	if (!m_file[0] || !m_file[1]) {
		m_lastError = EBADF;
		return -1;
	}

	return m_fullSize;
}

}

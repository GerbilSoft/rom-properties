/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * VectorFile.cpp: IRpFile implementation using an std::vector.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "VectorFile.hpp"

// C++ STL classes.
using std::string;

namespace LibRpFile {

// 128 MB *should* be a reasonable maximum...
static constexpr off64_t MAX_SIZE = 128U*1024U*1024U;

/**
 * Open an IRpFile backed by an std::vector.
 * The resulting IRpFile is writable.
 * @param size Initial size
 */
VectorFile::VectorFile(size_t size)
	: m_pVector(new std::vector<uint8_t>(size))
	, m_pos(0)
{
	// Reserve at least 16 KiB.
	static constexpr size_t INITIAL_RESERVE = 16U * 1024;
	if (size < INITIAL_RESERVE) {
		m_pVector->reserve(INITIAL_RESERVE);
	}

	// VectorFile is writable.
	m_isWritable = true;
}

VectorFile::~VectorFile()
{
	delete m_pVector;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t VectorFile::read(void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not reading anything...
		return 0;
	}

	// Check if size is in bounds.
	// NOTE: Need to use a signed comparison here.
	const size_t vec_size = m_pVector->size();
	if (static_cast<off64_t>(m_pos) > (static_cast<off64_t>(vec_size) - static_cast<off64_t>(size))) {
		// Not enough data.
		// Copy whatever's left in the buffer.
		size = vec_size - m_pos;
	}

	// Copy the data.
	const uint8_t *const buf = m_pVector->data();
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
size_t VectorFile::write(const void *ptr, size_t size)
{
	if (unlikely(size == 0)) {
		// Not writing anything...
		return 0;
	}

	// Do we need to expand the std::vector?
	const size_t vec_size = m_pVector->size();
	const off64_t req_size = static_cast<off64_t>(vec_size) + size;
	if (req_size < 0) {
		// Overflow...
		return 0;
	} else if (req_size > MAX_SIZE) {
		// Too much...
		m_lastError = -ENOMEM;
		return 0;
	} else if (req_size > static_cast<off64_t>(vec_size)) {
		// Need to expand the std::vector.
		m_pVector->resize(static_cast<size_t>(req_size));
	}

	// Copy the data to the buffer.
	uint8_t *const buf = m_pVector->data();
	memcpy(&buf[m_pos], ptr, size);
	m_pos += size;
	return size;
}

/**
 * Set the file position.
 * @param pos File position.
 * @return 0 on success; -1 on error.
 */
int VectorFile::seek(off64_t pos)
{
	// NOTE: m_pos is size_t, since it's referring to
	// a position within a memory buffer.
	if (pos <= 0) {
		m_pos = 0;
	} else if (pos >= static_cast<off64_t>(m_pVector->size())) {
		m_pos = m_pVector->size();
	} else {
		m_pos = static_cast<size_t>(pos);
	}

	return 0;
}

/**
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; -1 on error.
 */
int VectorFile::truncate(off64_t size)
{
	if (unlikely(size < 0)) {
		m_lastError = -EINVAL;
		return -1;
	} else if (size > MAX_SIZE) {
		m_lastError = -ENOMEM;
		return -1;
	}

	m_pVector->resize(static_cast<size_t>(size));
	return 0;
}

}

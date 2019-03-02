/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * DiscReader.cpp: Basic disc reader interface.                            *
 * This class is a "null" interface that simply passes calls down to       *
 * libc's stdio functions.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "DiscReader.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

namespace LibRpBase {

/**
 * Construct a DiscReader with the specified file.
 * The file is ref()'d, so the original file can be
 * unref()'d afterwards.
 * @param file File to read from.
 */
DiscReader::DiscReader(IRpFile *file)
	: super(file)
	, m_offset(0)
	, m_length(0)
{
	if (!m_file) {
		m_lastError = EBADF;
		return;
	}

	// TODO: Propagate errors.
	m_length = file->size();
	if (m_length < 0) {
		m_length = 0;
	}
}

/**
 * Construct a DiscReader with the specified file.
 * The file is ref()'d, so the original file can be
 * unref()'d afterwards.
 * @param file File to read from.
 * @param offset Starting offset.
 * @param length Disc length. (-1 for "until end of file")
 */
DiscReader::DiscReader(IRpFile *file, int64_t offset, int64_t length)
	: super(file)
	, m_offset(0)
	, m_length(0)
{
	if (!m_file) {
		m_lastError = EBADF;
		return;
	}

	// TODO: Propagate errors.
	// Validate offset and filesize.
	const int64_t filesize = m_file->size();
	if (offset > filesize) {
		offset = filesize;
	}
	if (length < 0 || offset + length > filesize) {
		length = filesize - offset;
	}

	m_offset = offset;
	m_length = length;
}

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int DiscReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	// DiscReader supports everything.
	RP_UNUSED(pHeader);
	RP_UNUSED(szHeader);
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DiscReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	// DiscReader supports everything.
	RP_UNUSED(pHeader);
	RP_UNUSED(szHeader);
	return 0;
}

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t DiscReader::read(void *ptr, size_t size)
{
	assert(m_file != nullptr);
	if (!m_file) {
		m_lastError = EBADF;
		return 0;
	}

	// Constrain size based on offset and length.
	int64_t pos = m_file->tell();
	if (pos + static_cast<int64_t>(size) > m_offset + m_length) {
		size = static_cast<size_t>(m_offset + m_length - pos);
	}

	size_t ret = m_file->read(ptr, size);
	m_lastError = m_file->lastError();
	return ret;
}

/**
 * Set the disc image position.
 * @param pos Disc image position.
 * @return 0 on success; -1 on error.
 */
int DiscReader::seek(int64_t pos)
{
	assert(m_file != nullptr);
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	int ret = m_file->seek(pos + m_offset);
	if (ret != 0) {
		m_lastError = m_file->lastError();
	}
	return ret;
}

/**
 * Get the disc image position.
 * @return Partition position on success; -1 on error.
 */
int64_t DiscReader::tell(void)
{
	assert(m_file != nullptr);
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	int64_t ret = m_file->tell();
	if (ret < 0) {
		m_lastError = m_file->lastError();
	}
	return ret;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
int64_t DiscReader::size(void)
{
	assert(m_file != nullptr);
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}
	// TODO: Propagate errors.
	return m_length;
}

}

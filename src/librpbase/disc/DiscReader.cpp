/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * DiscReader.cpp: Basic disc reader interface.                            *
 * This class is a "null" interface that simply passes calls down to       *
 * libc's stdio functions.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DiscReader.hpp"

// Other rom-properties libraries
using namespace LibRpFile;

namespace LibRpBase {

/**
 * Construct a DiscReader with the specified file.
 * The file is ref()'d, so the original file can be
 * unref()'d afterwards.
 * @param file File to read from.
 */
DiscReader::DiscReader(const IRpFilePtr &file)
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
DiscReader::DiscReader(const IRpFilePtr &file, off64_t offset, off64_t length)
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
	const off64_t filesize = m_file->size();
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
	// TODO: What if pos is negative?
	off64_t pos = m_file->tell() - m_offset;
	if (pos + static_cast<off64_t>(size) > m_offset + m_length) {
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
int DiscReader::seek(off64_t pos)
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
off64_t DiscReader::tell(void)
{
	assert(m_file != nullptr);
	if (!m_file) {
		m_lastError = EBADF;
		return -1;
	}

	off64_t ret = m_file->tell();
	if (ret < 0) {
		m_lastError = m_file->lastError();
	}
	return ret;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
off64_t DiscReader::size(void)
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

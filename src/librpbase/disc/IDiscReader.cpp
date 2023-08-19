/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.cpp: Disc reader interface.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "IDiscReader.hpp"

// Other rom-properties libraries
using namespace LibRpFile;

namespace LibRpBase {

IDiscReader::IDiscReader(const IRpFilePtr &file)
	: m_lastError(0)
	, m_hasDiscReader(false)
	, m_file(file)
{}

IDiscReader::IDiscReader(IDiscReader *discReader)
	: m_lastError(0)
	, m_hasDiscReader(true)
{
	if (discReader) {
		m_discReader = discReader->ref();
	} else {
		m_discReader = nullptr;
	}
}

IDiscReader::~IDiscReader()
{
	if (m_hasDiscReader) {
		UNREF(m_discReader);
	}
}

/**
 * Is the disc image open?
 * This usually only returns false if an error occurred.
 * @return True if the disc image is open; false if it isn't.
 */
bool IDiscReader::isOpen(void) const
{
	if (!m_hasDiscReader) {
		return (m_file && m_file->isOpen());
	} else {
		return (m_discReader && m_discReader->isOpen());
	}
}

/**
 * Seek to the specified address, then read data.
 * @param pos	[in] Requested seek address.
 * @param ptr	[out] Output data buffer.
 * @param size	[in] Amount of data to read, in bytes.
 * @return Number of bytes read on success; 0 on seek or read error.
 */
size_t IDiscReader::seekAndRead(off64_t pos, void *ptr, size_t size)
{
	int ret = this->seek(pos);
	if (ret != 0) {
		// Seek error.
		return 0;
	}
	return this->read(ptr, size);
}

/**
 * Seek to a relative offset. (SEEK_CUR)
 * @param pos Relative offset
 * @return 0 on success; -1 on error
 */
int IDiscReader::seek_cur(off64_t offset)
{
	off64_t pos = this->tell();
	if (pos < 0) {
		return -1;
	}
	return this->seek(pos + offset);
}

/** Device file functions **/

/**
 * Is the underlying file a device file?
 * @return True if the underlying file is a device file; false if not.
 */
bool IDiscReader::isDevice(void) const
{
	if (!m_hasDiscReader) {
		return (m_file && m_file->isDevice());
	} else {
		return (m_discReader && m_discReader->isDevice());
	}
}

}

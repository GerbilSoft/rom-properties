/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.cpp: Disc reader interface.                                 *
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

#include "IDiscReader.hpp"
#include "../file/IRpFile.hpp"

namespace LibRpBase {

IDiscReader::IDiscReader(IRpFile *file)
	: m_hasDiscReader(false)
	, m_lastError(0)
{
	if (file) {
		m_file = file->ref();
	} else {
		m_file = nullptr;
	}
}

IDiscReader::IDiscReader(IDiscReader *discReader)
	: m_hasDiscReader(true)
	, m_lastError(0)
{
	// TODO: Reference counting?
	m_discReader = discReader;
}

IDiscReader::~IDiscReader()
{
	if (!m_hasDiscReader) {
		if (m_file) {
			m_file->unref();
		}
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
size_t IDiscReader::seekAndRead(int64_t pos, void *ptr, size_t size)
{
	int ret = this->seek(pos);
	if (ret != 0) {
		// Seek error.
		return 0;
	}
	return this->read(ptr, size);
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

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.cpp: Disc reader interface.                                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "IDiscReader.hpp"

namespace LibRpBase {

IDiscReader::IDiscReader()
	: m_lastError(0)
{ }

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int IDiscReader::lastError(void) const
{
	return m_lastError;
}

/**
 * Clear the last error.
 */
void IDiscReader::clearError(void)
{
	m_lastError = 0;
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

}

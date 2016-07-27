/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DiscReader.cpp: Basic disc reader interface.                            *
 * This class is a "null" interface that simply passes calls down to       *
 * libc's stdio functions.                                                 *
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

#include "DiscReader.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

DiscReader::DiscReader(FILE *file)
	: IDiscReader(file)
{
	if (!m_file)
		return;

	// Get the file size.
	fseek(m_file, 0, SEEK_END);
	m_fileSize = ftell(m_file);
	::rewind(m_file);
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t DiscReader::read(void *ptr, size_t size)
{
	assert(m_file != nullptr);
	return fread(ptr, 1, size, m_file);
}

/**
 * Set the file position.
 * @param pos File position.
 */
void DiscReader::seek(int64_t pos)
{
	assert(m_file != nullptr);
	fseek(m_file, pos, SEEK_SET);
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
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

 // dup()
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {
 
/**
 * Construct an IDiscReader with the specified file.
 * The file is dup()'d, so the original file can be
 * closed afterwards.
 *
 * NOTE: Subclasses must initialize m_fileSize.
 *
 * @param file File to read from.
 */
IDiscReader::IDiscReader(FILE *file)
	: m_file(nullptr)
	, m_fileSize(0)
{
	if (!file)
		return;

	// dup() the file.
	int fd_old = fileno(file);
	int fd_new = dup(fd_old);
	if (fd_new >= 0) {
		m_file = fdopen(fd_new, "rb");
		if (m_file) {
			// Make sure we're at the beginning of the file.
			::rewind(m_file);
			::fflush(m_file);
		}
	}
}

IDiscReader::~IDiscReader()
{
	if (m_file) {
		fclose(m_file);
	}
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool IDiscReader::isOpen(void) const
{
	return (m_file != nullptr);
}

/**
 * Seek to the beginning of the file.
 */
void IDiscReader::rewind(void)
{
	this->seek(0);
}

/**
 * Get the file size.
 * @return File size.
 */
int64_t IDiscReader::fileSize(void) const
{
	assert(m_file != nullptr);
	return m_fileSize;
}

}

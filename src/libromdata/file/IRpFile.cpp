/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IRpFile.cpp: File wrapper interface.                                    *
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

#include "IRpFile.hpp"

// C includes. (C++ namespace)
#include <cstdio>

namespace LibRomData {

IRpFile::IRpFile()
	: m_lastError(0)
{ }

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int IRpFile::lastError(void) const
{
	return m_lastError;
}

/**
 * Clear the last error.
 */
void IRpFile::clearError(void)
{
	m_lastError = 0;
}

/**
 * Get a single character (byte) from the file
 * @return Character from file, or EOF on end of file or error.
 */
int IRpFile::getc(void)
{
	uint8_t buf;
	size_t sz = this->read(&buf, 1);
	return (sz == 1 ? buf : EOF);
}

/**
 * Un-get a single character (byte) from the file.
 *
 * Note that this implementation doesn't actually
 * use a character buffer; it merely decrements the
 * seek pointer by 1.
 *
 * @param c Character. (ignored!)
 * @return 0 on success; non-zero on error.
 */
int IRpFile::ungetc(int c)
{
	RP_UNUSED(c);	// TODO: Don't ignore this?

	// TODO: seek() overload that supports SEEK_CUR?
	int64_t pos = tell();
	if (pos <= 0) {
		// Cannot ungetc().
		return -1;
	}

	return this->seek(pos-1);
}

}

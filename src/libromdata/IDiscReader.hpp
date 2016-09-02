/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IDiscReader.hpp: Disc reader interface.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IDISCREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IDISCREADER_HPP__

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>

namespace LibRomData {

class IRpFile;

class IDiscReader
{
	protected:
		/**
		 * Construct an IDiscReader with the specified file.
		 * The file is dup()'d, so the original file can be
		 * closed afterwards.
		 *
		 * NOTE: Subclasses must initialize m_fileSize.
		 *
		 * @param file File to read from.
		 */
		IDiscReader(IRpFile *file);
	public:
		virtual ~IDiscReader();

	private:
		IDiscReader(const IDiscReader &);
		IDiscReader &operator=(const IDiscReader&);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) = 0;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) = 0;

		/**
		 * Seek to the beginning of the file.
		 */
		void rewind(void);

		/**
		 * Get the file size.
		 * @return File size.
		 */
		int64_t fileSize(void) const;

	protected:
		IRpFile *m_file;
		int64_t m_fileSize;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IDISCREADER_HPP__ */

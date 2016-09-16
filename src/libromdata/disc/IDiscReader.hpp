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
		IDiscReader() { }
	public:
		virtual ~IDiscReader() = 0;

	private:
		IDiscReader(const IDiscReader &);
		IDiscReader &operator=(const IDiscReader&);

	public:
		/**
		 * Is the disc image open?
		 * This usually only returns false if an error occurred.
		 * @return True if the disc image is open; false if it isn't.
		 */
		virtual bool isOpen(void) const = 0;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		virtual int lastError(void) const = 0;

		/**
		 * Clear the last error.
		 */
		virtual void clearError(void) = 0;

		/**
		 * Read data from the disc image.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) = 0;

		/**
		 * Set the disc image position.
		 * @param pos disc image position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) = 0;

		/**
		 * Seek to the beginning of the disc image.
		 */
		virtual void rewind(void) = 0;

		/**
		 * Get the disc image size.
		 * @return Disc image size, or -1 on error.
		 */
		virtual int64_t size(void) const = 0;
};

/**
 * Both gcc and MSVC fail to compile unless we provide
 * an empty implementation, even though the function is
 * declared as pure-virtual.
 */
inline IDiscReader::~IDiscReader() { }

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IDISCREADER_HPP__ */

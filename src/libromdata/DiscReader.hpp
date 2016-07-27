/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DiscReader.hpp: Basic disc reader interface.                            *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISCREADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISCREADER_HPP__

#include "IDiscReader.hpp"

namespace LibRomData {

class DiscReader : public IDiscReader
{
	public:
		/**
		 * Construct a DiscReader with the specified file.
		 * The file is dup()'d, so the original file can be
		 * closed afterwards.
		 * @param file File to read from.
		 */
		DiscReader(FILE *file);

	private:
		DiscReader(const DiscReader &);
		DiscReader &operator=(const DiscReader&);

	public:
		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override;

		/**
		 * Set the file position.
		 * @param pos File position.
		 */
		virtual void seek(int64_t pos) override;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISCREADER_HPP__ */

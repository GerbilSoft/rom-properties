/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * IRpFile.hpp: File wrapper interface.                                    *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IRPFILE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IRPFILE_HPP__

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>	/* for size_t */

namespace LibRomData {

class IRpFile
{
	protected:
		IRpFile() { }
	public:
		virtual ~IRpFile() { }

	private:
		IRpFile(const IRpFile &);
		IRpFile &operator=(const IRpFile&);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
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
		 * dup() the file handle.
		 * Needed because IRpFile* objects are typically
		 * pointers, not actual instances of the object.
		 * @return dup()'d file, or nullptr on error.
		 */
		virtual IRpFile *dup(void) = 0;

		/**
		 * Close the file.
		 */
		virtual void close(void) = 0;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) = 0;

		/**
		 * Write data to the file.
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		virtual size_t write(const void *ptr, size_t size) = 0;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) = 0;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		virtual int64_t tell(void) = 0;

		/**
		 * Seek to the beginning of the file.
		 */
		virtual void rewind(void) = 0;

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		virtual int64_t fileSize(void) = 0;

	public:
		/** Convenience functions implemented for all IRpFile classes. **/

		/**
		 * Get a single character (byte) from the file
		 * @return Character from file, or EOF on end of file or error.
		 */
		int getc(void);

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
		int ungetc(int c);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IRPFILE_HPP__ */

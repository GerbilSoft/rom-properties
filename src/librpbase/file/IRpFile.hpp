/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IRpFile.hpp: File wrapper interface.                                    *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__

#include "librpbase/common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>	/* for size_t */

// C++ includes.
#include <string>

namespace LibRpBase {

class IRpFile
{
	protected:
		IRpFile();
	public:
		virtual ~IRpFile() { }

	private:
		RP_DISABLE_COPY(IRpFile)

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
		int lastError(void) const;

		/**
		 * Clear the last error.
		 */
		void clearError(void);

		/**
		 * dup() the file handle.
		 *
		 * Needed because IRpFile* objects are typically
		 * pointers, not actual instances of the object.
		 *
		 * NOTE: The dup()'d IRpFile* does NOT have a separate
		 * file pointer. This is due to how dup() works.
		 *
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
		 * Seek to the beginning of the file.
		 */
		inline void rewind(void)
		{
			this->seek(0);
		}

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		virtual int64_t tell(void) = 0;

		/**
		 * Truncate the file.
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		virtual int truncate(int64_t size = 0) = 0;

	public:
		/** File properties. **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		virtual int64_t size(void) = 0;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		virtual std::string filename(void) const = 0;

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

		/**
		 * Seek to the specified address, then read data.
		 * @param pos	[in] Requested seek address.
		 * @param ptr	[out] Output data buffer.
		 * @param size	[in] Amount of data to read, in bytes.
		 * @return Number of bytes read on success; 0 on seek or read error.
		 */
		size_t seekAndRead(int64_t pos, void *ptr, size_t size);

	protected:
		int m_lastError;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__ */

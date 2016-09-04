/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpMemFile.hpp: IRpFile implementation using a memory block.             *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_RPMEMFILE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_RPMEMFILE_HPP__

#include "IRpFile.hpp"
#include "libromdata/config.libromdata.h"

namespace LibRomData {

class RpMemFile : public IRpFile
{
	public:
		/**
		 * Open an IRpFile backed by memory.
		 * The resulting IRpFile is read-only.
		 *
		 * NOTE: The memory buffer is NOT copied; it must remain
		 * valid as long as this object is still open.
		 *
		 * @param buf Memory buffer.
		 * @param size Size of memory buffer.
		 */
		RpMemFile(const void *buf, size_t size);

	private:
		typedef IRpFile super;
	public:
		RpMemFile(const RpMemFile &);
		RpMemFile &operator=(const RpMemFile&);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		virtual bool isOpen(void) const override;

		/**
		 * dup() the file handle.
		 * Needed because IRpFile* objects are typically
		 * pointers, not actual instances of the object.
		 * @return dup()'d file, or nullptr on error.
		 */
		virtual IRpFile *dup(void) override;

		/**
		 * Close the file.
		 */
		virtual void close(void) override;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpMemFile; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		virtual size_t write(const void *ptr, size_t size) override;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		virtual int64_t tell(void) override;

		/**
		 * Seek to the beginning of the file.
		 */
		virtual void rewind(void) override;

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		virtual int64_t fileSize(void) override;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		virtual int lastError(void) const override;

	protected:
		const void *m_buf;	// Memory buffer.
		size_t m_size;		// Size of memory buffer.
		size_t m_pos;		// Current position.
		int m_lastError;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IRPFILE_HPP__ */

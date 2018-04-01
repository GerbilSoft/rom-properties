/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpMemFile.hpp: IRpFile implementation using a memory buffer.            *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_RPMEMFILE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_RPMEMFILE_HPP__

#include "IRpFile.hpp"

namespace LibRpBase {

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
		RpMemFile(const RpMemFile &other);
		RpMemFile &operator=(const RpMemFile &other);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final;

		/**
		 * dup() the file handle.
		 *
		 * Needed because IRpFile* objects are typically
		 * pointers, not actual instances of the object.
		 *
		 * NOTE: For RpMemFile, this will simply copy the
		 * memory buffer pointer and size values.
		 *
		 * @return dup()'d file, or nullptr on error.
		 */
		IRpFile *dup(void) final;

		/**
		 * Close the file.
		 */
		void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for RpMemFile; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Truncate the file.
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(int64_t size = 0) final;

	public:
		/** File properties. **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		int64_t size(void) final;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;

	protected:
		const void *m_buf;	// Memory buffer.
		size_t m_size;		// Size of memory buffer.
		size_t m_pos;		// Current position.
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__ */

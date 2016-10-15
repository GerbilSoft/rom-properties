/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PartitionFile.hpp: IRpFile implementation for IPartition.               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_PARTITIONFILE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_PARTITIONFILE_HPP__

#include "../file/IRpFile.hpp"
#include "libromdata/config.libromdata.h"
#include <cstdio>

namespace LibRomData {

class IPartition;

class PartitionFile : public IRpFile
{
	public:
		/**
		 * Open a file from an IPartition.
		 * NOTE: These files are read-only.
		 * @param partition IPartition object.
		 * @param offset File starting offset.
		 * @param size File size.
		 */
		PartitionFile(IPartition *partition, int64_t offset, int64_t size);
	public:
		virtual ~PartitionFile();

	private:
		typedef IRpFile super;
	public:
		PartitionFile(const PartitionFile &other);
		PartitionFile &operator=(const PartitionFile &other);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		virtual bool isOpen(void) const final;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		virtual int lastError(void) const final;

		/**
		 * Clear the last error.
		 */
		virtual void clearError(void) final;

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
		virtual IRpFile *dup(void) final;

		/**
		 * Close the file.
		 */
		virtual void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for PartitionFile; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		virtual size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		virtual int64_t tell(void) final;

		/**
		 * Seek to the beginning of the file.
		 */
		virtual void rewind(void) final;

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		virtual int64_t fileSize(void) final;

	protected:
		int m_lastError;

		IPartition *m_partition;
		int64_t m_offset;	// File starting offset.
		int64_t m_size;		// File size.
		int64_t m_pos;		// Current position.
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_PARTITIONFILE_HPP__ */

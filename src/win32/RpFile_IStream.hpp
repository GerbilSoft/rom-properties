/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_IStream.hpp: IRpFile using an IStream*.                          *
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

#ifndef __ROMPROPERTIES_WIN32_RPFILE_ISTREAM_HPP__
#define __ROMPROPERTIES_WIN32_RPFILE_ISTREAM_HPP__

#include "libromdata/file/IRpFile.hpp"
#include "libromdata/RpWin32.hpp"
#include <objidl.h>

class RpFile_IStream : public LibRomData::IRpFile
{
	public:
		/**
		 * Create an IRpFile using IStream* as the underlying storage mechanism.
		 * @param pStream IStream*.
		 */
		RpFile_IStream(IStream *pStream);
		virtual ~RpFile_IStream();

	private:
		typedef LibRomData::IRpFile super;
		RpFile_IStream(const RpFile_IStream &other);
		RpFile_IStream &operator=(const RpFile_IStream &other);

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
		IStream *m_pStream;
		int m_lastError;
};

#endif /* __ROMPROPERTIES_WIN32_RPFILE_ISTREAM_HPP__ */

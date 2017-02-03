/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpFile_IStream.hpp: IRpFile using an IStream*.                          *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
		explicit RpFile_IStream(IStream *pStream);
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
		virtual bool isOpen(void) const override final;

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
		virtual IRpFile *dup(void) override final;

		/**
		 * Close the file.
		 */
		virtual void close(void) override final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override final;

		/**
		 * Write data to the file.
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		virtual size_t write(const void *ptr, size_t size) override final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		virtual int64_t tell(void) override final;

		/**
		 * Truncate the file.
		 * @param size New size. (default is 0)
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int truncate(int64_t size = 0) override final;

	public:
		/** File properties. **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		virtual int64_t size(void) override final;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		virtual LibRomData::rp_string filename(void) const override final;

	protected:
		IStream *m_pStream;
		LibRomData::rp_string m_filename;
};

#endif /* __ROMPROPERTIES_WIN32_RPFILE_ISTREAM_HPP__ */

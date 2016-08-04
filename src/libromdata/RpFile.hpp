/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile.hpp: Standard file object.                                       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_RPFILE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_RPFILE_HPP__

#include "IRpFile.hpp"
#include "libromdata/config.libromdata.h"

// C includes. (C++ namespace)
#include <cstdio>

namespace LibRomData {

class RpFile : public IRpFile
{
	public:
		enum FileMode {
			FM_READ = 0,		// Read-only.
			FM_WRITE = 1,		// Read/write.
			FM_OPEN = 0,		// Open the file. (Must exist!)
			FM_CREATE = 2,		// Create the file. (Will overwrite!)

			// Combinations.
			FM_OPEN_READ = 0,	// Open for reading. (Must exist!)
			FM_OPEN_WRITE = 1,	// Open for reading/writing. (Must exist!)
			//FM_CREATE_READ = 2,	// Not valid; handled as FM_CREATE_WRITE.
			FM_CREATE_WRITE = 3,	// Create for reading/writing. (Will overwrite!)
		};

		/**
		 * Open a file.
		 * NOTE: Files are always opened in binary mode.
		 * @param filename Filename.
		 * @param mode File mode.
		 */
		RpFile(const rp_char *filename, FileMode mode);
		RpFile(const rp_string &filename, FileMode mode);
	public:
		virtual ~RpFile();

	public:
		RpFile(const RpFile &);
		RpFile &operator=(const RpFile&);

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		virtual bool isOpen(void) const;

		/**
		 * dup() the file handle.
		 * Needed because IRpFile* objects are typically
		 * pointers, not actual instances of the object.
		 * @return dup()'d file, or nullptr on error.
		 */
		virtual IRpFile *dup(void);

		/**
		 * Close the file.
		 */
		virtual void close(void);

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size);

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos);

		/**
		 * Seek to the beginning of the file.
		 */
		virtual void rewind(void);

		/**
		 * Get the file size.
		 * @return File size.
		 */
		virtual int64_t fileSize(void);

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		virtual int lastError(void) const;

	protected:
		FILE *m_file;
		FileMode m_mode;
		int m_lastError;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IRPFILE_HPP__ */

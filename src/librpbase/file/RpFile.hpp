/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile.hpp: Standard file object.                                       *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_RPFILE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_RPFILE_HPP__

#include "IRpFile.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <memory>

namespace LibRpBase {

class RpFilePrivate;
class RpFile : public IRpFile
{
	public:
		enum FileMode : uint8_t {
			FM_READ = 0,		// Read-only.
			FM_WRITE = 1,		// Read/write.
			FM_OPEN = 0,		// Open the file. (Must exist!)
			FM_CREATE = 2,		// Create the file. (Will overwrite!)

			// Combinations.
			FM_OPEN_READ = 0,	// Open for reading. (Must exist!)
			FM_OPEN_WRITE = 1,	// Open for reading/writing. (Must exist!)
			//FM_CREATE_READ = 2,	// Not valid; handled as FM_CREATE_WRITE.
			FM_CREATE_WRITE = 3,	// Create for reading/writing. (Will overwrite!)

			// Mask.
			FM_MODE_MASK = 3,	// Mode mask.

			// Extras.
			FM_GZIP_DECOMPRESS = 4,	// Transparent gzip decompression. (read-only!)
			FM_OPEN_READ_GZ = FM_READ | FM_GZIP_DECOMPRESS,
		};

		/**
		 * Open a file.
		 * NOTE: Files are always opened in binary mode.
		 * @param filename Filename.
		 * @param mode File mode.
		 */
		RpFile(const char *filename, FileMode mode);
		RpFile(const std::string &filename, FileMode mode);
	private:
		void init(void);
	protected:
		virtual ~RpFile();	// call unref() instead

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(RpFile)
	protected:
		friend class RpFilePrivate;
		RpFilePrivate *const d_ptr;

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final;

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
		/** File properties **/

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

	public:
		/** Device file functions **/

		/**
		 * Is this a device file?
		 * @return True if this is a device file; false if not.
		 */
		bool isDevice(void) const final;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__ */

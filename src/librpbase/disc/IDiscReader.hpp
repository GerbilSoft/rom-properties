/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.hpp: Disc reader interface.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IDISCREADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IDISCREADER_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstddef>

namespace LibRpBase {

class IRpFile;
class IDiscReader
{
	protected:
		IDiscReader(IRpFile *file);
		IDiscReader(IDiscReader *discReader);
	public:
		virtual ~IDiscReader();

	private:
		RP_DISABLE_COPY(IDiscReader)

	public:
		/** Disc image detection functions. **/

		// TODO: Move RomData::DetectInfo somewhere else and use it here?
		/**
		 * Is a disc image supported by this object?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		virtual int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const = 0;

	public:
		/**
		 * Is the disc image open?
		 * This usually only returns false if an error occurred.
		 * @return True if the disc image is open; false if it isn't.
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
		 * Read data from the disc image.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) = 0;

		/**
		 * Set the disc image position.
		 * @param pos disc image position.
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
		 * Get the disc image position.
		 * @return Disc image position on success; -1 on error.
		 */
		virtual int64_t tell(void) = 0;

		/**
		 * Get the disc image size.
		 * @return Disc image size, or -1 on error.
		 */
		virtual int64_t size(void) = 0;

	public:
		/** Convenience functions implemented for all IRpFile classes. **/

		/**
		 * Seek to the specified address, then read data.
		 * @param pos	[in] Requested seek address.
		 * @param ptr	[out] Output data buffer.
		 * @param size	[in] Amount of data to read, in bytes.
		 * @return Number of bytes read on success; 0 on seek or read error.
		 */
		size_t seekAndRead(int64_t pos, void *ptr, size_t size);

	public:
		/** Device file functions **/

		/**
		 * Is the underlying file a device file?
		 * @return True if the underlying file is a device file; false if not.
		 */
		bool isDevice(void) const;

	protected:
		// Subclasses may have an underlying file, or may
		// stack another IDiscReader object.
		union {
			IRpFile *m_file;
			IDiscReader *m_discReader;
		};
		bool m_hasDiscReader;

		int m_lastError;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IDISCREADER_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.hpp: Disc reader interface.                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes
#include <stdint.h>

// C includes (C++ namespace)
#include <cstddef>

// common macros
#include "common.h"

// librpfile
#include "librpfile/IRpFile.hpp"

namespace LibRpBase {

class NOVTABLE IDiscReader
{
	protected:
		explicit IDiscReader(const LibRpFile::IRpFilePtr &file);
		explicit IDiscReader(const std::shared_ptr<IDiscReader> &discReader);
	public:
		virtual ~IDiscReader() = default;

	private:
		RP_DISABLE_COPY(IDiscReader)

	public:
		/** Disc image detection functions **/

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
		bool isOpen(void) const;

		/**
		 * Get the last error.
		 * @return Last POSIX error, or 0 if no error.
		 */
		inline int lastError(void) const
		{
			return m_lastError;
		}

		/**
		 * Clear the last error.
		 */
		inline void clearError(void)
		{
			m_lastError = 0;
		}

		/**
		 * Read data from the disc image.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		virtual size_t read(void *ptr, size_t size) = 0;

		/**
		 * Set the disc image position.
		 * @param pos Disc image position
		 * @return 0 on success; -1 on error
		 */
		virtual int seek(off64_t pos) = 0;

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
		virtual off64_t tell(void) = 0;

		/**
		 * Get the disc image size.
		 * @return Disc image size, or -1 on error.
		 */
		virtual off64_t size(void) = 0;

	public:
		/** Convenience functions implemented for all IDiscReader subclasses. **/

		/**
		 * Seek to the specified address, then read data.
		 * @param pos	[in] Requested seek address.
		 * @param ptr	[out] Output data buffer.
		 * @param size	[in] Amount of data to read, in bytes.
		 * @return Number of bytes read on success; 0 on seek or read error.
		 */
		ATTR_ACCESS_SIZE(write_only, 3, 4)
		size_t seekAndRead(off64_t pos, void *ptr, size_t size);

		/**
		 * Seek to a relative offset. (SEEK_CUR)
		 * @param pos Relative offset
		 * @return 0 on success; -1 on error
		 */
		int seek_cur(off64_t offset);

	public:
		/** Device file functions **/

		/**
		 * Is the underlying file a device file?
		 * @return True if the underlying file is a device file; false if not.
		 */
		bool isDevice(void) const;

	protected:
		int m_lastError;
		bool m_hasDiscReader;

		// Subclasses may have an underlying file, or may
		// stack another IDiscReader object.
		// NOTE: This used to be a union{} prior to the std::shared_ptr<> conversion.
		// TODO: Convert to std::variant<>?
		LibRpFile::IRpFilePtr m_file;
		std::shared_ptr<IDiscReader> m_discReader;
};

typedef std::shared_ptr<IDiscReader> IDiscReaderPtr;

}

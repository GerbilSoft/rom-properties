/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * IDiscReader.hpp: Disc reader interface.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
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

class NOVTABLE IDiscReader : public LibRpFile::IRpFile
{
	protected:
		explicit IDiscReader(const LibRpFile::IRpFilePtr &file);
	public:
		~IDiscReader() override = default;

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
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final
		{
			return (m_file && m_file->isOpen());
		}

	public:
		/**
		 * Close the underlying file.
		 */
		void close(void) final
		{
			if (m_file) {
				m_file->close();
			}
		}

		/**
		 * Write data to the file.
		 * @param ptr	[in] Input data buffer.
		 * @param size	[in] Amount of data to write, in bytes.
		 * @return Number of bytes written.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		size_t write(const void *ptr, size_t size) final
		{
			// Not implemented for IDiscReader.
			RP_UNUSED(ptr);
			RP_UNUSED(size);
			return 0;
		}

	protected:
		// The underlying file for this IDiscReader.
		// May also be another IDiscReader for layering.
		LibRpFile::IRpFilePtr m_file;
};

typedef std::shared_ptr<IDiscReader> IDiscReaderPtr;

}

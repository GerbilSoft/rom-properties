/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * DiscReader.hpp: Basic disc reader interface.                            *
 * This class is a "null" interface that simply passes calls down to       *
 * libc's stdio functions.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IDiscReader.hpp"

namespace LibRpBase {

class DiscReader final : public IDiscReader
{
	public:
		/**
		 * Construct a DiscReader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 */
		explicit DiscReader(const LibRpFile::IRpFilePtr &file);

		/**
		 * Construct a DiscReader with the specified file.
		 * The file is ref()'d, so the original file can be
		 * unref()'d by the caller afterwards.
		 * @param file File to read from.
		 * @param offset Starting offset.
		 * @param length Disc length. (-1 for "until end of file")
		 */
		explicit DiscReader(const LibRpFile::IRpFilePtr &file, off64_t offset, off64_t length);
	public:
		~DiscReader() final = default;

	private:
		typedef IDiscReader super;
		RP_DISABLE_COPY(DiscReader)

	public:
		/** Disc image detection functions. **/

		/**
		 * Is a disc image supported by this class?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		static int isDiscSupported_static(const uint8_t *pHeader, size_t szHeader);

		/**
		 * Is a disc image supported by this object?
		 * @param pHeader Disc image header.
		 * @param szHeader Size of header.
		 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
		 */
		int isDiscSupported(const uint8_t *pHeader, size_t szHeader) const override;

	public:
		/**
		 * Read data from the disc image.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) override;

		/**
		 * Set the disc image position.
		 * @param pos Disc image position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) override;

		/**
		 * Get the disc image position.
		 * @return Disc image position on success; -1 on error.
		 */
		off64_t tell(void) final;

		/**
		 * Get the disc image size.
		 * @return Disc image size, or -1 on error.
		 */
		off64_t size(void) override;

	protected:
		// Offset/length. Useful for e.g. GameCube TGC.
		off64_t m_offset;
		off64_t m_length;
};

}

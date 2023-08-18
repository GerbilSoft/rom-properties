/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * SubFile.hpp: SubFile sub-file implementation, essentially the           *
 * equivalent of DiscReader+PartitionFile but with less overhead.          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpfile/IRpFile.hpp"

// C++ includes
#include <memory>

namespace LibRpFile {

class SubFile final : public IRpFile
{
	public:
		/**
		 * Open a portion of an IRpFile.
		 */
		SubFile(const std::shared_ptr<IRpFile> &file, off64_t offset, off64_t length)
			: m_file(file)
			, m_offset(offset)
			, m_length(length)
		{
			this->rewind();
		}

	private:
		typedef SubFile super;
		RP_DISABLE_COPY(SubFile)

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final
		{
			return (bool)m_file;
		}

		/**
		 * Close the file.
		 */
		void close(void) final
		{}

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return 0;
			}

			// NOTE: Not enforcing length bounds.
			return m_file->read(ptr, size);
		}

		/**
		 * Write data to the file.
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		size_t write(const void *ptr, size_t size) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return 0;
			}

			// NOTE: Not enforcing length bounds.
			return m_file->write(ptr, size);
		}

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return -1;
			}

			if (pos <= 0) {
				pos = 0;
			} else if (pos >= m_length) {
				pos = m_length;
			}

			return m_file->seek(pos + m_offset);
		}

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		off64_t tell(void) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return -1;
			}

			return m_file->tell() - m_offset;
		}

		/**
		 * Flush buffers.
		 * This operation only makes sense on writable files.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int flush(void) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return -1;
			}

			return m_file->flush();
		}

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		off64_t size(void) final
		{
			if (!m_file) {
				m_lastError = EBADF;
				return 0;
			}

			return m_length;
		}

	protected:
		std::shared_ptr<IRpFile> m_file;
		off64_t m_offset;
		off64_t m_length;
};

}

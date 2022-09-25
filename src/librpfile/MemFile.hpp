/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * MemFile.hpp: IRpFile implementation using a memory buffer.              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_MEMFILE_HPP__
#define __ROMPROPERTIES_LIBRPFILE_MEMFILE_HPP__

#include "IRpFile.hpp"

// C includes (C++ namespace)
#include <cerrno>

// C++ includes
#include <string>

namespace LibRpFile {

class RP_LIBROMDATA_PUBLIC MemFile : public IRpFile
{
	public:
		/**
		 * Open an IRpFile backed by memory.
		 * The resulting IRpFile is read-only.
		 *
		 * NOTE: The memory buffer is NOT copied; it must remain
		 * valid as long as this object is still open.
		 *
		 * @param buf Memory buffer.
		 * @param size Size of memory buffer.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		MemFile(const void *buf, size_t size);
	protected:
		/**
		 * Internal constructor for use by subclasses.
		 * This initializes everything to nullptr.
		 */
		MemFile();
	protected:
		virtual ~MemFile() { }	// call unref() instead

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(MemFile)

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		bool isOpen(void) const final
		{
			return (m_buf != nullptr);
		}

		/**
		 * Close the file.
		 */
		void close(void) override;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * (NOTE: Not valid for MemFile; this will always return 0.)
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		off64_t tell(void) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		off64_t size(void) final
		{
			if (!m_buf) {
				m_lastError = EBADF;
				return -1;
			}

			return static_cast<off64_t>(m_size);
		}

		/**
		 * Get the filename.
		 * @return Filename. (May be nullptr if the filename is not available.)
		 */
		inline const char *filename(void) const final
		{
			return (!m_filename.empty() ? m_filename.c_str() : nullptr);
		}

	public:
		/** MemFile functions **/

		/**
		 * Set the filename.
		 * @param filename Filename
		 */
		inline void setFilename(const char *filename)
		{
			m_filename = filename;
		}

		/**
		 * Set the filename.
		 * @param filename Filename
		 */
		inline void setFilename(const std::string &filename)
		{
			m_filename = filename;
		}

	protected:
		const void *m_buf;	// Memory buffer.
		size_t m_size;		// Size of memory buffer.
		size_t m_pos;		// Current position.

		std::string m_filename;	// Dummy filename.
};

}

#endif /* __ROMPROPERTIES_LIBRPFILE_MEMFILE_HPP__ */

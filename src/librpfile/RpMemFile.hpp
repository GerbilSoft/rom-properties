/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpMemFile.hpp: IRpFile implementation using a memory buffer.            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_RPMEMFILE_HPP__
#define __ROMPROPERTIES_LIBRPFILE_RPMEMFILE_HPP__

#include "IRpFile.hpp"

namespace LibRpFile {

class RpMemFile : public IRpFile
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
		RpMemFile(const void *buf, size_t size);
	protected:
		/**
		 * Internal constructor for use by subclasses.
		 * This initializes everything to nullptr.
		 */
		RpMemFile();
	protected:
		virtual ~RpMemFile() { }	// call unref() instead

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(RpMemFile)

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
		 * (NOTE: Not valid for RpMemFile; this will always return 0.)
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

		/**
		 * Truncate the file.
		 * (NOTE: Not valid for RpMemFile; this will always return -1.)
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(off64_t size = 0) final;

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
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final
		{
			// TODO: Implement this?
			return std::string();
		}

	protected:
		const void *m_buf;	// Memory buffer.
		size_t m_size;		// Size of memory buffer.
		size_t m_pos;		// Current position.
};

}

#endif /* __ROMPROPERTIES_LIBRPFILE_RPMEMFILE_HPP__ */

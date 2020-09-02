/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpVectorFile.hpp: IRpFile implementation using an std::vector.          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPFILE_RPVECTORFILE_HPP__
#define __ROMPROPERTIES_LIBRPFILE_RPVECTORFILE_HPP__

#include "librpfile/IRpFile.hpp"

// C++ includes.
#include <vector>

namespace LibRpFile {

class RpVectorFile : public IRpFile
{
	public:
		/**
		 * Open an IRpFile backed by an std::vector.
		 * The resulting IRpFile is writable.
		 */
		RpVectorFile();
	protected:
		virtual ~RpVectorFile() { }	// call unref() instead

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(RpVectorFile)

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
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
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
		off64_t size(void) final;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;

	public:
		/** Extra functions **/

		/**
		 * Make the file writable.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int makeWritable(void) final { return 0; }

	public:
		/** RpVectorFile-specific functions **/

		/**
		 * Get the underlying std::vector.
		 * @return std::vector.
		 */
		const std::vector<uint8_t> &vector(void) const
		{
			return m_vector;
		}

	protected:
		std::vector<uint8_t> m_vector;
		size_t m_pos;		// Current position.
};

}

#endif /* __ROMPROPERTIES_LIBRPFILE_RPVECTORFILE_HPP__ */

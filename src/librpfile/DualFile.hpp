/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * DualFile.hpp: Special wrapper for handling a split file as one.         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IRpFile.hpp"

// C++ includes
#include <array>

namespace LibRpFile {

class DualFile final : public IRpFile
{
	public:
		/**
		 * Open two files and handle them as if they're a single file.
		 * The resulting IRpFile is read-only.
		 *
		 * @param file0 First file.
		 * @param file1 Second file.
		 */
		DualFile(const IRpFilePtr &file0, const IRpFilePtr &file1);
	protected:
		/**
		 * Internal constructor for use by subclasses.
		 * This initializes everything to nullptr.
		 */
		DualFile();
	public:
		~DualFile() final = default;

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(DualFile)

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
		 * (NOTE: Not valid for DualFile; this will always return 0.)
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
		off64_t size(void) final;

	protected:
		std::array<IRpFilePtr, 2> m_file;
		std::array<off64_t, 2> m_size;
		off64_t m_fullSize;	// Combined sizes.
		off64_t m_pos;		// Current position.
};

typedef std::shared_ptr<DualFile> DualFilePtr;

}

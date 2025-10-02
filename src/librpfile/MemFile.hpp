/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * MemFile.hpp: IRpFile implementation using a memory buffer.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IRpFile.hpp"

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
	RP_LIBROMDATA_LOCAL MemFile();
public:
	RP_LIBROMDATA_LOCAL ~MemFile() override;

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
	 * @param pos		[in] File position
	 * @param whence	[in] Where to seek from
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos, SeekWhence whence) final;

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
		return m_filename;
	}

public:
	/** MemFile functions **/

	/**
	 * Set the filename.
	 * @param filename Filename
	 */
	void setFilename(const char *filename);

	/**
	 * Set the filename.
	 * @param filename Filename
	 */
	void setFilename(const std::string &filename);

	/**
	 * Get a direct pointer to the memory buffer.
	 * @return Pointer to memory buffer
	 */
	const void *buffer(void) const
	{
		return m_buf;
	}

protected:
	const void *m_buf;	// Memory buffer
	size_t m_size;		// Size of memory buffer
	size_t m_pos;		// Current position

	char *m_filename;	// Dummy filename
};

typedef std::shared_ptr<MemFile> MemFilePtr;

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * GzipFile.hpp: Transparent Gzip decompression file wrapper.              *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IRpFile.hpp"

// C++ STL classes
#include <string>
#include <vector>

namespace LibRpFile {

class GzipFilePrivate;
class GzipFile final : public IRpFile
{
public:
	/**
	 * Open a file for transparent Gzip decompression. (read-only)
	 * @param file Other IRpFile that was already opened
	 */
	GzipFile(const IRpFilePtr &file);

	~GzipFile() final;

private:
	typedef IRpFile super;
protected:
	friend class GzipFilePrivate;
	GzipFilePrivate *const d_ptr;
public:
	RP_DISABLE_COPY(GzipFile)

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
	 * (NOTE: Not valid for GzipFile; this will always return 0.)
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
	off64_t size(void) final;

	/**
	 * Get the filename.
	 * @return Filename (UTF-8) (May be nullptr if the filename is not available.)
	 */
	const char *filename(void) const final;

	/**
	 * Get the file modification time.
	 * @return File modification time, or -1 if not available.
	 */
	rp_time_t mtime(void) final;
};

typedef std::shared_ptr<GzipFile> GzipFilePtr;

} // namespace LibRpFile

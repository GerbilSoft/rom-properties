/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_ZArchiveReader.hpp: IRpFile wrapper for ZArchiveReader.          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "compiler-attrs.h"

#include "librpfile/IRpFile.hpp"

#include "zarchive/zarchivereader.h"

// C++ STL classes
#include <memory>
#include <string>
#include "rp-string_view.hpp"

namespace LibRomData {

// IRpFile wrapper around ZArchiveReadere
// TODO: Move to a separate file.
class RpFile_ZArchiveReader final : public LibRpFile::IRpFile
{
public:
	/**
	 * Create an IRpFile using ZArchiveReader as the underlying storage mechanism.
	 * NOTE: RpFile_ZArchiveReader does NOT take ownership of the ZArchiveReader.
	 * @param zarchive	[in] ZArchiveReader*
	 * @param package_name	[in] Package name within the ZArchive (usually title ID + version)
	 * @param filename	[in] File to open within the ZArchive
	 */
	explicit RpFile_ZArchiveReader(const std::shared_ptr<ZArchiveReader> &zarchive, std::string_view package_name, std::string_view filename);

private:
	typedef LibRpFile::IRpFile super;
public:
	RP_DISABLE_COPY(RpFile_ZArchiveReader)

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

	/**
	 * Flush buffers.
	 * This operation only makes sense on writable files.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int flush(void) final;

public:
	/** File properties **/

	/**
	 * Get the file size.
	 * @return File size, or negative on error.
	 */
	off64_t size(void) final;

	/**
	 * Get the filename.
	 * @return Filename. (May be nullptr if the filename is not available.)
	 */
	const char *filename(void) const final;

protected:
	std::shared_ptr<ZArchiveReader> m_zarchive;
	std::string m_filename;

	off64_t m_pos;
	off64_t m_size;
	ZArchiveNodeHandle m_handle;
};

} // namespace LibRomData

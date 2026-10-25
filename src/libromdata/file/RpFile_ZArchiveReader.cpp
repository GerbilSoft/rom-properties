/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpFile_ZArchiveReader.cpp: IRpFile wrapper for ZArchiveReader.          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpFile_ZArchiveReader.hpp"

// C++ STL classes
using std::shared_ptr;

// libfmt
#include "rp-libfmt.h"

namespace LibRomData {

/**
 * Create an IRpFile using ZArchiveReader as the underlying storage mechanism.
 * NOTE: RpFile_ZArchiveReader does NOT take ownership of the ZArchiveReader.
 * @param zarchive	[in] ZArchiveReader*
 * @param package_name	[in] Package name within the ZArchive (usually title ID + version)
 * @param filename	[in] File to open within the ZArchive
 */
RpFile_ZArchiveReader::RpFile_ZArchiveReader(const shared_ptr<ZArchiveReader> &zarchive, std::string_view package_name, std::string_view filename)
	: m_zarchive(zarchive)
	, m_pos(0)
	, m_size(0)
	, m_handle(ZARCHIVE_INVALID_NODE)
{
	if (package_name.empty() || filename.empty()) {
		return;
	}

	// Construct the full filename.
	m_filename = "/";
	m_filename += package_name;
	if (filename[0] != '/') {
		m_filename += '/';
	}
	m_filename += filename;

	// NOTE: ZArchive handles do not need to be closed.
	m_handle = zarchive->LookUp(m_filename.c_str(), true, false);
	if (m_handle == ZARCHIVE_INVALID_NODE || !m_zarchive->IsFile(m_handle)) {
		m_filename.clear();
		m_handle = ZARCHIVE_INVALID_NODE;
		return;
	}

	m_size = zarchive->GetFileSize(m_handle);
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool RpFile_ZArchiveReader::isOpen(void) const
{
	return (m_handle != ZARCHIVE_INVALID_NODE);
}

/**
 * Close the file.
 */
void RpFile_ZArchiveReader::close(void)
{
	m_zarchive = nullptr;
	m_filename.clear();
	m_pos = 0;
	m_size = 0;
	m_handle = ZARCHIVE_INVALID_NODE;
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFile_ZArchiveReader::read(void *ptr, size_t size)
{
	if (m_handle == ZARCHIVE_INVALID_NODE) {
		return 0;
	}

	size_t sz_ret = m_zarchive->ReadFromFile(m_handle, m_pos, size, ptr);
	m_pos += sz_ret;
	return sz_ret;
}

/**
 * Write data to the file.
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t RpFile_ZArchiveReader::write(const void *ptr, size_t size)
{
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	return 0;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int RpFile_ZArchiveReader::seek(off64_t pos, SeekWhence whence)
{
	pos = adjust_file_pos_for_whence(pos, whence, m_pos, m_size);
	m_pos = static_cast<size_t>(constrain_file_pos(pos, m_size));
	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t RpFile_ZArchiveReader::tell(void)
{
	return m_pos;
}

/**
 * Flush buffers.
 * This operation only makes sense on writable files.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile_ZArchiveReader::flush(void)
{
	return 0;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t RpFile_ZArchiveReader::size(void)
{
	return m_size;
}

/**
 * Get the filename.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *RpFile_ZArchiveReader::filename(void) const
{
	return m_filename.c_str();
}

} // namespace LibRomData

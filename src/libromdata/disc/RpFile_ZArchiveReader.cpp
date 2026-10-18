#include "RpFile_ZArchiveReader.hpp"

namespace LibRomData {

/**
 * Create an IRpFile using ZArchiveReader as the underlying storage mechanism.
 * NOTE: RpFile_ZArchiveReader does NOT take ownership of the ZArchiveReader.
 * @param zarchive	[in] ZArchiveReader*
 * @param filename	[in] File to open within the ZArchive
 */
RpFile_ZArchiveReader::RpFile_ZArchiveReader(ZArchiveReader *zarchive, const char *filename)
	: m_zarchive(zarchive)
	, m_pos(0)
	, m_size(0)
	, m_handle(ZARCHIVE_INVALID_NODE)
{
	if (!filename || filename[0] == '\0') {
		m_filename.clear();
		return;
	}

	// FIXME: Need to enumerate root directories and get the main title root.
	// For now, hard-coding it.
	m_filename = "/000500001017d800_v0";
	m_filename += filename;

	// NOTE: ZArchive handles do not need to be closed.
	m_handle = zarchive->LookUp(m_filename.c_str(), true, false);
	if (!m_zarchive->IsFile(m_handle)) {
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
 * Truncate the file.
 * @param size New size. (default is 0)
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile_ZArchiveReader::truncate(off64_t size)
{
	return -EROFS;
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

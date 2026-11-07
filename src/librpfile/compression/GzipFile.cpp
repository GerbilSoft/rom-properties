/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * GzipFile.cpp: Transparent Gzip decompression file wrapper.              *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpfile.h"
#include "GzipFile.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
#include <array>
using std::array;
using std::string;
using std::unique_ptr;

// zlib for transparent gzip decompression.
#include <zlib.h>
// gzclose_r() and gzclose_w() were introduced in zlib-1.2.4.
#if (ZLIB_VER_MAJOR > 1) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR > 2) || \
    (ZLIB_VER_MAJOR == 1 && ZLIB_VER_MINOR == 2 && ZLIB_VER_REVISION >= 4)
// zlib-1.2.4 or later
#else
#  define gzclose_r(file) gzclose(file)
#  define gzclose_w(file) gzclose(file)
#endif

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRpFile {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class GzipFilePrivate
{
public:
	GzipFilePrivate(const IRpFilePtr &file);
	~GzipFilePrivate();

public:
	RP_DISABLE_COPY(GzipFilePrivate)

public:
	IRpFilePtr file;	// File pointer

#define ZLIB_BUFFER_SIZE 16384U

	// zlib/gzip variables
	off64_t gzsz;				// Uncompressed file size
	z_stream zstream;			// zlib stream

	off64_t z_realpos;			// position in real file
	unsigned int z_filepos;			// position in compressed file

	unsigned int zbufLen;			// Current used length of z_buf
	unsigned int zcurPos;			// Current position in z_buf
	array<uint8_t, ZLIB_BUFFER_SIZE> z_buf;	// zlib decompression buffer
};

/** GzipFilePrivate **/

GzipFilePrivate::GzipFilePrivate(const IRpFilePtr &file)
	: file(file)
	, gzsz(-1)
	, z_realpos(0)
	, z_filepos(0)
	, zbufLen(0)
	, zcurPos(0)
{
	assert((bool)file);
	assert(file->isOpen());

	// NOTE: zstream *must* be zero-initialized.
	// Otherwise, inflateInit() will crash.
	memset(&zstream, 0, sizeof(zstream));
}

GzipFilePrivate::~GzipFilePrivate()
{
	if (gzsz >= 0) {
		// Close zlib.
		inflateEnd(&zstream);
	}
}

/** GzipFile **/

/**
 * Open a file for transparent Gzip decompression. (read-only)
 * @param file Other IRpFile that was already opened
 */
GzipFile::GzipFile(const IRpFilePtr &file)
	: d_ptr(new GzipFilePrivate(file))
{
	RP_D(GzipFile);
	m_isWritable = false;

	// Verify that this is a gzipped file.
	// If it isn't, then we'll just pass through the read() calls.
	// NOTE: File will be at position 0 after initialization.
	// (Physical position 0 if not gzip; logical position 0 if gzip)
	uint16_t gzmagic;
	d->file->rewind();
	size_t size = d->file->read(&gzmagic, sizeof(gzmagic));
	if (unlikely(size != sizeof(gzmagic)) || gzmagic != be16_to_cpu(0x1F8B)) {
		// Not a gzipped file.
		d->file->rewind();
		return;
	}

	// This is a gzipped file.
	// Get the uncompressed size at the end of the file.
	if (d->file->seek(-4, IRpFile::SeekWhence::End) != 0) {
		// Unable to seek to the end of the file?
		d->file->rewind();
		return;
	}

	uint32_t uncomp_sz;
	size = d->file->read(&uncomp_sz, sizeof(uncomp_sz));
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	uncomp_sz = le32_to_cpu(uncomp_sz);
#endif /* BYTEORDER != SYS_LIL_ENDIAN */
	if (size != sizeof(uncomp_sz) /*|| uncomp_sz < real_sz-(10+8)*/) {
		// Read error.
		d->file->rewind();
		return;
	}

	// Rewind back to the beginning of the stream.
	d->file->rewind();

	// NOTE: Uncompressed size might be smaller than the real filesize
	// in cases where gzip doesn't help much.
	// TODO: Add better verification heuristics?
	d->gzsz = static_cast<off64_t>(uncomp_sz);

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Don't do any gzip checking.
		return;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	int err = inflateInit2(&d->zstream, 16 + MAX_WBITS);
	if (err != Z_OK) {
		// zlib stream initialization failed.
		d->gzsz = -1;
		return;
	}

	// File is compressed.
	m_isCompressed = true;
}

GzipFile::~GzipFile()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool GzipFile::isOpen(void) const
{
	RP_D(const GzipFile);
	return (d->file != nullptr && d->file->isOpen());
}

/**
 * Close the file.
 */
void GzipFile::close(void)
{
	RP_D(GzipFile);

	if (d->gzsz >= 0) {
		// Close zlib.
		inflateEnd(&d->zstream);
		d->gzsz = -1;
	}
	if (d->file) {
		d->file->close();
		d->file.reset();
	}
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t GzipFile::read(void *ptr, size_t size)
{
	RP_D(GzipFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return 0;
	}

	if (unlikely(d->gzsz < 0)) {
		// Not a gzipped file. Read directly.
		size_t sz_ret = d->file->read(ptr, size);
		m_lastError = d->file->lastError();
		return sz_ret;
	}

	// Read and decompress.
	// Reference: https://www.codeproject.com/Articles/3602/Zlib-compression-decompression-wrapper-as-ISequent
	d->zstream.next_out = static_cast<Bytef*>(ptr);
	d->zstream.avail_out = static_cast<uInt>(size);

	// Only seek if we need to read data.
	bool didSeek = false;

	if (d->zcurPos == d->zbufLen) {
		// Need to read more data from the gzipped file.
		if (!didSeek) {
			// Seek to the last real position.
			int ret = d->file->seek(d->z_realpos);
			if (ret != 0) {
				// Seek error.
				m_lastError = d->file->lastError();
				return 0;
			}
			didSeek = true;
		}

		// Read compressed data into the buffer.
		d->zcurPos = 0;
		d->zbufLen = 0;
		size_t sz_read = d->file->read(d->z_buf.data(), d->z_buf.size());
		if (sz_read == 0 || sz_read > d->z_buf.size()) {
			// Read error.
			m_lastError = d->file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
		d->z_realpos += static_cast<unsigned int>(sz_read);
		d->zbufLen = static_cast<unsigned int>(sz_read);
	}

	int err = Z_OK;
	do {
		d->zstream.next_in = &d->z_buf[d->zcurPos];
		d->zstream.avail_in = d->zbufLen - d->zcurPos;

		if (d->zstream.avail_in == 0) {
			// Out of data.
			break;
		}

		err = inflate(&d->zstream, Z_SYNC_FLUSH);
		if (err == Z_OK || err == Z_STREAM_END) {
			// Data decompressed successfully.
			// NOTE: Z_STREAM_END occurs if we reached
			// the end of the stream in this read.
		} else {
			// Error decompressing data.
			m_lastError = EIO;
			break;
		}

		if (d->zstream.avail_out == 0) {
			d->zcurPos = static_cast<unsigned int>(d->zstream.next_in - d->z_buf.data());
			break;
		}

		// Read more data from the gzipped file.
		if (!didSeek) {
			// Seek to the last real position.
			int ret = d->file->seek(d->z_realpos);
			if (ret != 0) {
				// Seek error.
				m_lastError = d->file->lastError();
				break;
			}
			didSeek = true;
		}

		// Read compressed data into the buffer.
		d->zcurPos = 0;
		d->zbufLen = 0;
		size_t sz_read = d->file->read(d->z_buf.data(), d->z_buf.size());
		if (sz_read == 0 || sz_read > d->z_buf.size()) {
			// Read error.
			m_lastError = d->file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			break;
		}
		d->z_realpos += static_cast<unsigned int>(sz_read);
		d->zbufLen = static_cast<unsigned int>(sz_read);
	} while (d->zstream.avail_out > 0);

	// Adjust the current seek pointer based on how much data was read.
	const unsigned int sz_read = static_cast<unsigned int>(size - d->zstream.avail_out);
	d->z_filepos += sz_read;

	// We're done here.
	return sz_read;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for GzipFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t GzipFile::write(const void *ptr, size_t size)
{
	// Not a valid operation for GzipFile.
	RP_UNUSED(ptr);
	RP_UNUSED(size);
	m_lastError = EBADF;
	return 0;
}

/**
 * Set the file position.
 * @param pos		[in] File position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int GzipFile::seek(off64_t pos, SeekWhence whence)
{
	RP_D(GzipFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->gzsz < 0)) {
		// Not a gzipped file. Seek directly.
		int ret = d->file->seek(pos, whence);
		m_lastError = d->file->lastError();
		return ret;
	}

	// zlib stream: Special seek handling.
	pos = adjust_file_pos_for_whence(pos, whence, static_cast<off64_t>(d->z_filepos), static_cast<off64_t>(d->gzsz));
	if (pos == d->z_filepos) {
		// No seek necessary.
		return 0;
	}

	size_t skip_bytes;
	if (pos < d->z_filepos) {
		// Seeking backwards. Reset the stream first.
		inflateEnd(&d->zstream);

		d->z_filepos = 0;
		d->zbufLen = 0;
		d->zcurPos = 0;
		memset(&d->zstream, 0, sizeof(d->zstream));
		int err = inflateInit2(&d->zstream, 16+MAX_WBITS);

		// Seek to the beginning of the real file.
		// NOTE: Assuming rewind() always succeeds.
		d->file->rewind();
		d->z_realpos = 0;

		if (err != Z_OK) {
			// Error re-initializing the zlib stream.
			// Cannot continue with this stream.
			memset(&d->zstream, 0, sizeof(d->zstream));
			d->gzsz = 0;
			return -1;
		}

		skip_bytes = static_cast<size_t>(pos);
	} else if (pos > d->z_filepos) {
		// Seeking forwards. Skip over the bytes.
		skip_bytes = static_cast<size_t>(pos - d->z_filepos);
	} else {
		// Should not happen...
		assert(!"Something happened...");
		return -1;
	}

	// Seek to the requested position by skipping over the required number of bytes.
	unique_ptr<uint8_t[]> skip_buf(new uint8_t[ZLIB_BUFFER_SIZE]);
	while (skip_bytes > 0) {
		size_t sz_to_read = std::min(ZLIB_BUFFER_SIZE, skip_bytes);
		size_t sz_read = this->read(skip_buf.get(), sz_to_read);
		if (sz_read != sz_to_read) {
			// Read error...
			m_lastError = d->file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return -1;
		}

		skip_bytes -= sz_to_read;
	}

	// Seek was successful.
	return 0;
}

/**
 * Get the file position.
 * @return File position, or -1 on error.
 */
off64_t GzipFile::tell(void)
{
	RP_D(GzipFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->gzsz < 0)) {
		// Not a gzipped file. Call tell() directly.
		return d->file->tell();
	}

	// Return the compressed file position.
	return d->z_filepos;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t GzipFile::size(void)
{
	RP_D(GzipFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->gzsz < 0)) {
		// Not a gzipped file. Call size() directly.
		return d->file->size();
	}

	// gzipped files have the uncompressed size stored
	// at the end of the stream.
	return d->gzsz;
}

/**
 * Get the filename.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *GzipFile::filename(void) const
{
	RP_D(const GzipFile);
	return d->file->filename();
}

/**
 * Get the file modification time.
 * @return File modification time, or -1 if not available.
 */
rp_time_t GzipFile::mtime(void)
{
	RP_D(GzipFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}
	return d->file->mtime();
}

} // namespace LibRpFile

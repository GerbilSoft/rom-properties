/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * ZstdFile.cpp: Transparent zstd decompression file wrapper.              *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpfile.h"
#include "ZstdFile.hpp"

// librpbyteswap
#include "librpbyteswap/byteswap_rp.h"

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
#include <array>
using std::array;
using std::string;
using std::unique_ptr;

// zstd for transparent decompression
#include <zstd.h>

#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRpFile {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL1(ZSTD_freeDCtx, nullptr);
#endif /* _MSC_VER */

// zstd streaming context
// Copied from microtar_zstd.c.
struct zstd_ctx {
	ZSTD_DCtx *dctx;

	// Data buffer sizes
	size_t buf_in_size;
	size_t buf_out_size;

	// Buffer status
	// input.src and output.dst are allocated by us.
	// NOTE: output.pos is how much data was read into the buffer,
	// not our current output read position. For that, use output_ptr.
	ZSTD_inBuffer input;
	ZSTD_outBuffer output;
	size_t output_ptr;

	// Current decompressed position for arbitrary seek.
	off64_t unz_pos;
};

class ZstdFilePrivate
{
public:
	ZstdFilePrivate(const IRpFilePtr &file);
	~ZstdFilePrivate();

public:
	RP_DISABLE_COPY(ZstdFilePrivate)

public:
	IRpFilePtr file;	// File pointer
	off64_t unz_sz;		// Uncompressed size (from zstd header)
	zstd_ctx zctx;		// zstd context
};

/** ZstdFilePrivate **/

ZstdFilePrivate::ZstdFilePrivate(const IRpFilePtr &file)
	: file(file)
	, unz_sz(-1)
{
	assert((bool)file);
	assert(file->isOpen());

	// Zero-initialize the zstd context.
	// Actual zstd initialization will be done in the
	// public class constructor.
	// Otherwise, inflateInit() will crash.
	memset(&zctx, 0, sizeof(zctx));
}

ZstdFilePrivate::~ZstdFilePrivate()
{
	free(const_cast<void*>(zctx.input.src));
	free(zctx.output.dst);

	if (zctx.dctx) {
		ZSTD_freeDCtx(zctx.dctx);
	}
}

/** ZstdFile **/

/**
 * Open a file for transparent zstd decompression. (read-only)
 * @param file Other IRpFile that was already opened
 */
ZstdFile::ZstdFile(const IRpFilePtr &file)
	: d_ptr(new ZstdFilePrivate(file))
{
	RP_D(ZstdFile);
	m_isWritable = false;

#if defined(_MSC_VER) && defined(ZSTD_IS_DLL)
	// Delay load verification.
	if (DelayLoad_test_ZSTD_freeDCtx() != 0) {
		// Delay load failed. Cannot read zstd.
		return;
	}
#endif /* _MSC_VER && ZSTD_IS_DLL */

	// Verify that this is a zstd file.
	// If it isn't, then we'll just pass through the read() calls.
	// NOTE: File will be at position 0 after initialization.
	// (Physical position 0 if not zstd; logical position 0 if zstd)
	union {
		uint8_t u8[18];
		uint32_t u32;
	} zstd_header;
	d->file->rewind();
	size_t size = d->file->read(&zstd_header, sizeof(zstd_header));
	d->file->rewind();	// Rewind after reading the header.
	if (unlikely(size != sizeof(zstd_header)) || zstd_header.u32 != be32_to_cpu(0x28B52FFD)) {
		// Not a zstd file.
		return;
	}

	// This is a zstd file.

	// Check if the uncompressed size is present in the header.
	// TODO: Handle files that don't have it?

	// Determine where the uncompressed size field is, and how big it is.
	// Reference: https://github.com/meta/zstd/blob/dev/doc/zstd_compression_format.md
	// - [4 bytes]   magic number
	// - [1 byte]    Frame header descriptor
	// - [0-1 byte]  Window descriptor
	// - [0-4 bytes] Dictionary ID
	// - [0-8 bytes] Frame content size <-- this one!
	const uint8_t frame_header_descriptor = zstd_header.u8[4];
	uint8_t pos = 5;

	// Bits 1-0: Dictionary_ID_flag
	static const array<uint8_t, 4> dictionary_id_size = {{
		0, 1, 2, 4
	}};
	pos += dictionary_id_size[frame_header_descriptor & 3];

	// Bit 2: Content_Checksum_flag (not useful here)
	// Bit 3: Reserved_bit (not useful here)
	// Bit 4: Unused_bit (not useful here)

	// Bit 5: Single_Segment_flag
	// If set, the window descriptor byte is *not* present.
	// Add 1 to pos if this bit is *not* set.
	if (!(frame_header_descriptor & (1U << 5))) {
		pos++;
	}

	// Bits 7-6: Frame_Content_Size_flag
	// NOTE: Stored in little-endian format.
	uint64_t uncomp_sz = 0;
	switch ((frame_header_descriptor & 0xC0U) >> 6) {
		default:
			// Should not happen...
			assert(!"Invalid zstd Frame Content Size flag?");
			return;

		case 0:
			// If Single_Segment_flag is *not* set, no content size is available.
			if (!(frame_header_descriptor & (1U << 5))) {
				// No content size. Cannot handle this file.
				// TODO: Handle it somehow?
				return;
			}

			// 1-byte content size
			uncomp_sz = static_cast<uint64_t>(zstd_header.u8[pos]);
			break;

		case 1: {
			// 2-byte content size
			// NOTE: Add a 256-byte offset!
			uint16_t uncomp_sz16;
			memcpy(&uncomp_sz16, &zstd_header.u8[pos], sizeof(uncomp_sz16));
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			uncomp_sz32 = le36_to_cpu(uncomp_sz32);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
			uncomp_sz = static_cast<uint64_t>(uncomp_sz16) + 256;
			break;
		}

		case 2: {
			// 4-byte content size
			uint32_t uncomp_sz32;
			memcpy(&uncomp_sz32, &zstd_header.u8[pos], sizeof(uncomp_sz32));
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			uncomp_sz32 = le32_to_cpu(uncomp_sz32);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
			uncomp_sz = static_cast<uint64_t>(uncomp_sz32);
			break;
		}

		case 3:
			// 8-byte content size
			memcpy(&uncomp_sz, &zstd_header.u8[pos], sizeof(uncomp_sz));
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			uncomp_sz = le64_to_cpu(uncomp_sz);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
			break;
	}

	// Create the zstd context.
	zstd_ctx &zctx = d->zctx;
	zctx.dctx = ZSTD_createDCtx();
	if (!zctx.dctx) {
		// Could not create the decompression context.
		return;
	}

	zctx.buf_in_size = ZSTD_DStreamInSize();
	zctx.buf_out_size = ZSTD_DStreamOutSize();

	zctx.input.src = static_cast<uint8_t*>(malloc(zctx.buf_in_size));
	zctx.output.dst = static_cast<uint8_t*>(malloc(zctx.buf_out_size));
	if (!zctx.input.src || !zctx.output.dst) {
		// Could not allocate buffers...
		free(const_cast<void*>(zctx.input.src));
		free(zctx.output.dst);
		ZSTD_freeDCtx(zctx.dctx);
		memset(&zctx, 0, sizeof(zctx));
		return;
	}

	// File is compressed.
	d->unz_sz = static_cast<off64_t>(uncomp_sz);
	m_isCompressed = true;
}

ZstdFile::~ZstdFile()
{
	delete d_ptr;
}

/**
 * Is the file open?
 * This usually only returns false if an error occurred.
 * @return True if the file is open; false if it isn't.
 */
bool ZstdFile::isOpen(void) const
{
	RP_D(const ZstdFile);
	return (d->file != nullptr && d->file->isOpen());
}

/**
 * Close the file.
 */
void ZstdFile::close(void)
{
	RP_D(ZstdFile);

	zstd_ctx &zctx = d->zctx;
	free(const_cast<void*>(zctx.input.src));
	free(zctx.output.dst);

	if (zctx.dctx) {
		ZSTD_freeDCtx(zctx.dctx);
	}

	memset(&d->zctx, 0, sizeof(d->zctx));
}

static inline unsigned my_ZSTD_isError(size_t code)
{
	// FIXME: On 32-bit, ZSTD_isError() doesn't properly detect
	// error code 0xFFFFFFC0 -> 64 -> ZSTD_error_memory_allocation.
	// We'll assume that any decompressed size >= 0xFFFFFF00 is
	// an error, since we're not dealing with huge
	// zstd-compressed blocks here.
	return (code >= 0xFFFFFF00U);
}

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t ZstdFile::read(void *ptr, size_t size)
{
	RP_D(ZstdFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return 0;
	}

	if (unlikely(d->unz_sz < 0)) {
		// Not a zstd file. Read directly.
		size_t sz_ret = d->file->read(ptr, size);
		m_lastError = d->file->lastError();
		return sz_ret;
	}

	size_t sz_total = 0;
	zstd_ctx &zctx = d->zctx;
	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	while (size > 0) {
		// Check if we have any data remaining in the output buffer.
		// NOTE: zctx.output.pos indicates how much is left in the buffer.
		if (zctx.output_ptr < zctx.output.pos) {
			// We have data remaining. Determine how much to use.
			size_t toCopy = std::min(zctx.output.pos - zctx.output_ptr, size);
			memcpy(ptr8, static_cast<uint8_t*>(zctx.output.dst) + zctx.output_ptr, toCopy);

			ptr8 += toCopy;
			size -= toCopy;
			sz_total += toCopy;
			zctx.output_ptr += toCopy;
			zctx.unz_pos += toCopy;
			if (size == 0) {
				break;
			}
		}

		// Check if we need to read more data from the file.
		if (zctx.input.pos >= zctx.input.size) {
			size_t sz_read = d->file->read(const_cast<void*>(zctx.input.src), zctx.buf_in_size);
			if (sz_read == 0) {
				// No more data...
				break;
			}

			zctx.input.size = sz_read;
			zctx.input.pos = 0;
		}

		// Decompress the input buffer.
		zctx.output.size = zctx.buf_out_size;
		zctx.output.pos = 0;
		zctx.output_ptr = 0;
		size_t ret = ZSTD_decompressStream(zctx.dctx, &zctx.output, &zctx.input);
		if (my_ZSTD_isError(ret)) {
			// Decompression error!
			zctx.output.size = 0;
			zctx.output.pos = 0;
			zctx.output_ptr = 0;
			break;
		}

		// Output buffer copying will be done in the next loop iteration.
	}

	return sz_total;
}

/**
 * Write data to the file.
 * (NOTE: Not valid for ZstdFile; this will always return 0.)
 * @param ptr Input data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes written.
 */
size_t ZstdFile::write(const void *ptr, size_t size)
{
	// Not a valid operation for ZstdFile.
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
int ZstdFile::seek(off64_t pos, SeekWhence whence)
{
	RP_D(ZstdFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->unz_sz < 0)) {
		// Not a gzipped file. Seek directly.
		int ret = d->file->seek(pos, whence);
		m_lastError = d->file->lastError();
		return ret;
	}

	zstd_ctx &zctx = d->zctx;
	pos = adjust_file_pos_for_whence(pos, whence, static_cast<off64_t>(zctx.unz_pos), static_cast<off64_t>(d->unz_sz));
	if (pos == zctx.unz_pos) {
		// Useless seek to the same position...
		return 0;;
	}

	if (pos < zctx.unz_pos) {
		// NOTE: MicroTAR seeks backwards after reading the .tar header.
		// If we can seek backwards in the input buffer, do that so we
		// don't have to re-decompress everything.
		// NOTE 2: There are definitely some edge cases that this won't handle,
		// but it fixes *most* of them, resulting in the .tar.zst tests running
		// approximately 2.8 times slower than plain .tar files.

		// Quick test in debug builds: (MegaDrive only)
		// - .tar: 1,093 ms
		// - .tar.zst: 2,805 ms
		if (pos > 0) {
			size_t bytes_to_reverse = zctx.unz_pos - pos;
			if (zctx.output.pos != 0 && zctx.output_ptr <= zctx.output.pos) {
				if (bytes_to_reverse <= zctx.output_ptr) {
					zctx.output_ptr -= bytes_to_reverse;
					zctx.unz_pos -= bytes_to_reverse;
					return 0;
				}
			}
		}

		// Rewind the file and reset ZSTD decompression.
		d->file->rewind();
		zctx.input.size = 0;
		zctx.input.pos = 0;
		zctx.output.size = 0;
		zctx.output.pos = 0;
		zctx.output_ptr = 0;
		zctx.unz_pos = 0;
		ZSTD_DCtx_reset(zctx.dctx, ZSTD_reset_session_and_parameters);
	}

	if (pos == 0) {
		// We're at the start of the file. Nothing else to do here.
		return 0;
	}

	// Seek to the requested position by skipping over the required number of bytes.
	// TODO: Optimal buffer size?
	static constexpr size_t SKIP_BUF_SIZE = 16384U;
	unique_ptr<uint8_t[]> skip_buf(new uint8_t[SKIP_BUF_SIZE]);
	size_t skip_bytes = pos - zctx.unz_pos;
	while (skip_bytes > 0) {
		// Read up to seek_buf bytes.
		size_t sz_to_read = std::min(SKIP_BUF_SIZE, skip_bytes);
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
off64_t ZstdFile::tell(void)
{
	RP_D(ZstdFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->unz_sz < 0)) {
		// Not a gzipped file. Call tell() directly.
		return d->file->tell();
	}

	// Return the file position within the compressed stream.
	return d->zctx.unz_pos;
}

/** File properties **/

/**
 * Get the file size.
 * @return File size, or negative on error.
 */
off64_t ZstdFile::size(void)
{
	RP_D(ZstdFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}

	if (unlikely(d->unz_sz < 0)) {
		// Not a zstd file. Call size() directly.
		return d->file->size();
	}

	// zstd files have the uncompressed size stored in the header.
	return d->unz_sz;
}

/**
 * Get the filename.
 * @return Filename. (May be nullptr if the filename is not available.)
 */
const char *ZstdFile::filename(void) const
{
	RP_D(const ZstdFile);
	return d->file->filename();
}

/**
 * Get the file modification time.
 * @return File modification time, or -1 if not available.
 */
rp_time_t ZstdFile::mtime(void)
{
	RP_D(ZstdFile);
	if (unlikely(!d->file || !d->file->isOpen())) {
		m_lastError = EBADF;
		return -1;
	}
	return d->file->mtime();
}

} // namespace LibRpFile

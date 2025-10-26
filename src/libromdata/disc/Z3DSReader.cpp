/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Z3DSReader.cpp: Nintendo 3DS Z3DS reader.                               *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "Z3DSReader.hpp"
#include "z3ds_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// zstd seekable format
#include <zstd.h>
#include "zstd_seekable.h"
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#if defined(_MSC_VER) && defined(ZSTD_IS_DLL)
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL1(ZSTD_freeDCtx, nullptr);
#endif /* _MSC_VER && ZSTD_IS_DLL */

class Z3DSReaderPrivate
{
public:
	Z3DSReaderPrivate(Z3DSReader *q);
	~Z3DSReaderPrivate();

private:
	RP_DISABLE_COPY(Z3DSReaderPrivate)
protected:
	Z3DSReader *const q_ptr;

public:
	// Z3DS header
	Z3DS_Header z3ds_header;

	// ZSTD seekable context
	ZSTD_seekable *seekable;
	off64_t seekable_start;
	off64_t uncompressed_pos;
};

/** Z3DSReaderPrivate **/

Z3DSReaderPrivate::Z3DSReaderPrivate(Z3DSReader *q)
	: q_ptr(q)
	, seekable(nullptr)
	, seekable_start(0)
	, uncompressed_pos(0)
{
#if defined(_MSC_VER) && defined(ZSTD_IS_DLL)
	// Delay load verification.
	int err = DelayLoad_test_ZSTD_freeDCtx();
	if (err != 0) {
		// Delay load failed.
		// Z3DS format is not supported without zstd.
		q->m_lastError = -err;
		q->m_file.reset();
		return;
	}
#endif /* _MSC_VER && ZSTD_IS_DLL */

	// Read the Z3DS header.
	size_t size = q->m_file->seekAndRead(0, &z3ds_header, sizeof(z3ds_header));
	if (size != sizeof(z3ds_header)) {
		// Seek and/or read error.
		q->m_lastError = q->m_file->lastError();
		q->m_file.reset();
		return;
	}

	// Check the magic number and version number.
	if (z3ds_header.magic != cpu_to_be32(Z3DS_MAGIC) ||
	    z3ds_header.version != Z3DS_VERSION)
	{
		// Incorrect magic number and/or version number.
		q->m_lastError = EIO;
		q->m_file.reset();
		return;
	}

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswap the header, excluding magic numbers.
	z3ds_header.header_size		= le16_to_cpu(z3ds_header.header_size);
	z3ds_header.metadata_size	= le32_to_cpu(z3ds_header.metadata_size);
	z3ds_header.compressed_size	= le64_to_cpu(z3ds_header.compressed_size);
	z3ds_header.uncompressed_size	= le64_to_cpu(z3ds_header.uncompressed_size);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	// Determine the start of the ZSTD seekable stream.
	// NOTE: Not checking other header or metadata fields right now.
	seekable_start = z3ds_header.header_size + z3ds_header.metadata_size;

	// Open the ZSTD seekable stream.
	seekable = ZSTD_seekable_create();

	// Custom file handler.
	// Essentially calls the IRpFile functions with
	// seekable_start as an offset.
	ZSTD_seekable_customFile custom_file;
	custom_file.opaque = this;
	custom_file.read = [](void* opaque, void* buffer, size_t n) -> int {
		Z3DSReaderPrivate *const d = static_cast<Z3DSReaderPrivate*>(opaque);
		IRpFile *const file = d->q_ptr->m_file.get();
		size_t size = file->read(buffer, n);
		if (size != n) {
			return -1;
		}
		return 0;
	};
	custom_file.seek = [](void* opaque, long long offset, int origin) -> int {
		Z3DSReaderPrivate *const d = static_cast<Z3DSReaderPrivate*>(opaque);

		// Adjust the offset for SEEK_SET.
		if (origin == SEEK_SET) {
			offset += d->seekable_start;
		}

		IRpFile *const file = d->q_ptr->m_file.get();
		return file->seek(offset, static_cast<IRpFile::SeekWhence>(origin));
        };

	size_t init_result = ZSTD_seekable_initAdvanced(seekable, custom_file);
	if (ZSTD_isError(init_result)) {
		// Failed to initialize zstd_seekable...
		ZSTD_seekable_free(seekable);
		seekable = nullptr;
		q->m_lastError = EIO;
		q->m_file.reset();
	}

	// zstd_seekable is set up.
}

Z3DSReaderPrivate::~Z3DSReaderPrivate()
{
	if (seekable) {
		ZSTD_seekable_free(seekable);
	}
}

/** Z3DSReader **/

/**
 * Construct an Z3DSReader with the specified IRpFile.
 * @param file IRpFile
 */
Z3DSReader::Z3DSReader(const IRpFilePtr &file)
	: super(file)
	, d_ptr(new Z3DSReaderPrivate(this))
{}

Z3DSReader::~Z3DSReader()
{
	delete d_ptr;
}

/** Disc image detection functions **/

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int Z3DSReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader <= sizeof(Z3DS_Header)) {
		// Not supported.
		return -1;
	}

	const Z3DS_Header *const z3ds_header =
		reinterpret_cast<const Z3DS_Header*>(pHeader);
	if (z3ds_header->magic == cpu_to_be32(Z3DS_MAGIC) &&
	    z3ds_header->version == Z3DS_VERSION)
	{
		// File is supported.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int Z3DSReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	return isDiscSupported_static(pHeader, szHeader);
}

/** IDiscReader **/

/**
 * Read data from the file.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t Z3DSReader::read(void *ptr, size_t size)
{
	RP_D(Z3DSReader);
	assert(ptr != nullptr);
	assert(isOpen());
	if (!ptr) {
		m_lastError = EINVAL;
		return 0;
	} else if (!isOpen()) {
		m_lastError = EBADF;
		return 0;
	} else if (size == 0) {
		// Nothing to do...
		return 0;
	}

	// Are we already at the end of the file?
	if (d->uncompressed_pos >= static_cast<off64_t>(d->z3ds_header.uncompressed_size)) {
		return 0;
	}

	size_t result = ZSTD_seekable_decompress(d->seekable, ptr, size, d->uncompressed_pos);
	if (ZSTD_isError(result)) {
		// Read error...
		m_lastError = EIO;
		return 0;
	}
        d->uncompressed_pos += result;
	return result;
}

/**
 * Set the partition position.
 * @param pos		[in] Partition position
 * @param whence	[in] Where to seek from
 * @return 0 on success; -1 on error.
 */
int Z3DSReader::seek(off64_t pos, SeekWhence whence)
{
	RP_D(Z3DSReader);
	assert(isOpen());
	if (!isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	pos = adjust_file_pos_for_whence(pos, whence, static_cast<off64_t>(d->uncompressed_pos), static_cast<off64_t>(d->z3ds_header.uncompressed_size));
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	}
	d->uncompressed_pos = constrain_file_pos(pos, d->z3ds_header.uncompressed_size);
	return 0;
}

/**
 * Get the partition position.
 * @return Partition position on success; -1 on error.
 */
off64_t Z3DSReader::tell(void)
{
	RP_D(const Z3DSReader);
	assert(isOpen());
	if (!isOpen()) {
		m_lastError = EBADF;
		return -1;
	}

	return d->uncompressed_pos;
}

/**
 * Get the data size.
 * This size does not include the NCCH header,
 * and it's adjusted to exclude hashes.
 * @return Data size, or -1 on error.
 */
off64_t Z3DSReader::size(void)
{
	RP_D(const Z3DSReader);
	if (!d->seekable) {
		return -1;
	}
	return d->z3ds_header.uncompressed_size;
}

}

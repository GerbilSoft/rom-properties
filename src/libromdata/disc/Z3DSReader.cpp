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

// C++ STL classes
using std::pair;
using std::string;
using std::vector;

// Uninitialized vector class
#include "uvector.h"

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
	IRpFile *const file = q->m_file.get();
	size_t size = file->seekAndRead(0, &z3ds_header, sizeof(z3ds_header));
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
	if (seekable_start >= file->size()) {
		// Out of bounds.
		q->m_lastError = EIO;
		q->m_file.reset();
		return;
	}

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
{
	getZ3DSMetaData();
}

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

	/** Z3DSReader-specific functions **/

/**
 * Get the metadata.
 * @return Metadata, or empty vector if not present or an error occurred.
 */
vector<pair<string, std::vector<uint8_t>>> Z3DSReader::getZ3DSMetaData(void)
{
	static constexpr unsigned int Z3DS_MAX_METADATA_SIZE = 128U * 1024U;;
	RP_D(Z3DSReader);
	if (!d->seekable || d->z3ds_header.metadata_size < 2 || d->z3ds_header.metadata_size > Z3DS_MAX_METADATA_SIZE) {
		return {};
	}

	// Load the metadata.
	rp::uvector<uint8_t> metaData;
	metaData.resize(d->z3ds_header.metadata_size);
	size_t size = m_file->seekAndRead(sizeof(d->z3ds_header), metaData.data(), d->z3ds_header.metadata_size);
	if (size != d->z3ds_header.metadata_size) {
		// Seek and/or read error.
		return {};
	}

	const uint8_t *p = metaData.data();
	const uint8_t *const pEnd = p + metaData.size();

	// Check the metadata version.
	if (*p++ != Z3DS_METADATA_VERSION) {
		// Incorrect metadata version.
		return {};
	}

	// Process metadata items until we hit one of type TYPE_END.
	// NOTE: Reserving 5 elements, since that's what Azahar usually writes.
	vector<pair<string, vector<uint8_t>>> items;
	items.reserve(5);
	while ((p + sizeof(Z3DS_Metadata_Item_Header)) < pEnd) {
		const Z3DS_Metadata_Item_Header *const header =
			reinterpret_cast<const Z3DS_Metadata_Item_Header*>(p);
		if (header->type == Z3DS_METADATA_ITEM_TYPE_END) {
			// End of metadata.
			// NOTE: Metadata block should be 16-byte aligned.
			break;
		}
		p += sizeof(Z3DS_Metadata_Item_Header);

		// Check key and value length.
		const unsigned int key_len = header->key_len;
		const unsigned int value_len = le16_to_cpu(header->value_len);
		if ((p + key_len + value_len) > pEnd) {
			// Out of bounds.
			// TODO: Return an error?
			break;
		}

		assert(header->type == Z3DS_METADATA_ITEM_TYPE_BINARY);
		if (header->type != Z3DS_METADATA_ITEM_TYPE_BINARY) {
			// Only Z3DS_METADATA_ITEM_TYPE_BINARY is defined...
			// Skip this item.
			p += key_len;
			p += value_len;
			continue;
		}

		// Get the item key.
		string key(reinterpret_cast<const char*>(p), key_len);
		p += key_len;

		// Get the item value.
		// NOTE: Adding a NUL terminator byte for strings.
		vector<uint8_t> value(p, p + value_len);
		value.push_back(0);

		// NOTE: Using an std::vector, not an std::unordered_map,
		// in order to preserve ordering.

		// Emplace the value.
		items.emplace_back(std::make_pair(std::move(key), std::move(value)));
		p += value_len;
	}

	return items;
}

}

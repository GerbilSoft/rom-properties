/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GczReader.cpp: GameCube/Wii GCZ disc image reader.                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CompressedBlob.cpp
// - https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/DiscIO/CompressedBlob.h

#include "stdafx.h"
#include "config.librpbase.h"

#include "GczReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "gcz_structs.h"

// zlib
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::unique_ptr;

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class GczReaderPrivate : public SparseDiscReaderPrivate {
	public:
		explicit GczReaderPrivate(GczReader *q);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(GczReaderPrivate)

	public:
		// GCZ header
		GczHeader gczHeader;

		// Block pointers and hashes (NOTE: Byteswapped on demand)
		// If bit 63 of the block pointer is set, it's not compressed.
		// Hashes are Adler32.
		ao::uvector<uint64_t> blockPointers;
		ao::uvector<uint32_t> hashes;

		// Decompression buffer
		// (Same size as blockCache)
		ao::uvector<uint8_t> z_buffer;

		// Block cache
		ao::uvector<uint8_t> blockCache;
		uint32_t blockCacheIdx;

		// Starting offset of the data area
		// This offset must be added to the blockPointers value
		uint32_t dataOffset;

		/**
		 * Get the compressed size of a block.
		 * @param blockNum Block number
		 * @return Block's compressed size, or 0 on error
		 */
		uint32_t getBlockCompressedSize(uint32_t blockNum) const;
};

/** GczReaderPrivate **/

GczReaderPrivate::GczReaderPrivate(GczReader *q)
	: super(q)
	, blockCacheIdx(~0U)
	, dataOffset(0)
{
	// Clear the GCZ header struct.
	memset(&gczHeader, 0, sizeof(gczHeader));
}

/**
 * Get the compressed size of a block.
 * @param blockNum Block number
 * @return Block's compressed size, or 0 on error
 */
uint32_t GczReaderPrivate::getBlockCompressedSize(uint32_t blockNum) const
{
	assert(blockNum < blockPointers.size());
	if (blockNum >= blockPointers.size()) {
		// Out of range.
		return 0;
	}

	const uint64_t bptrStart = le64_to_cpu(blockPointers[blockNum]);
	if (blockNum < blockPointers.size() - 1) {
		// Not the last block.
		const uint64_t bptrEnd = le64_to_cpu(blockPointers[blockNum + 1]);
		return static_cast<uint32_t>(bptrEnd - bptrStart);
	} else /*if (blockNum == blockPointers.size() - 1)*/ {
		// Last block. Read up until the end of the disc.
		return static_cast<uint32_t>(gczHeader.z_data_size - bptrStart);
	}
}

/** GczReader **/

GczReader::GczReader(IRpFile *file)
	: super(new GczReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// GCZ is not supported without zlib.
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
		// zlib isn't in a DLL, but we need to ensure that the
		// CRC table is initialized anyway.
		get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Read the GCZ header.
	RP_D(GczReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->gczHeader, sizeof(d->gczHeader));
	if (sz != sizeof(d->gczHeader)) {
		// Error reading the GCZ header.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	// Check the GCZ magic.
	if (d->gczHeader.magic != cpu_to_le32(GCZ_MAGIC)) {
		// Invalid magic.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswap the header.
	d->gczHeader.magic	= le32_to_cpu(d->gczHeader.magic);
	d->gczHeader.sub_type	= le32_to_cpu(d->gczHeader.sub_type);
	d->gczHeader.z_data_size = le64_to_cpu(d->gczHeader.z_data_size);
	d->gczHeader.data_size	= le64_to_cpu(d->gczHeader.data_size);
	d->gczHeader.block_size = le32_to_cpu(d->gczHeader.block_size);
	d->gczHeader.num_blocks = le32_to_cpu(d->gczHeader.num_blocks);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	// Check if the block size is a supported power of two.
	// - Minimum: GCZ_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: GCZ_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	d->block_size = d->gczHeader.block_size;
	if (!isPow2(d->block_size) ||
	    d->block_size < GCZ_BLOCK_SIZE_MIN || d->block_size > GCZ_BLOCK_SIZE_MAX)
	{
		// Block size is out of range.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	// Verify that if data size is a multiple of the block size, it matches
	// num_blocks, or if not, num_blocks + 1.
	uint64_t expected_data_size;
	if (d->gczHeader.data_size % d->block_size == 0) {
		// Multiple of the block size.
		expected_data_size = d->gczHeader.data_size;
	} else {
		// Not a multiple of the block size.
		// Round it up, then check.
		expected_data_size = ALIGN_BYTES(d->block_size, d->gczHeader.data_size);
	}
	if (((uint64_t)d->block_size * (uint64_t)d->gczHeader.num_blocks) != expected_data_size) {
		// Not a multiple.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}
	d->disc_size = static_cast<off64_t>(expected_data_size);

	// Make sure the number of blocks is in range.
	// We should have at least one block, and at most 16 GB of data.
	if (d->gczHeader.num_blocks == 0) {
		// Zero blocks...
		d->disc_size = 0;
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}
	const uint64_t dataSizeCalc =
		static_cast<uint64_t>(d->gczHeader.num_blocks) *
		static_cast<uint64_t>(d->gczHeader.block_size);
	if (dataSizeCalc > 16ULL*1024ULL*1024ULL*1024ULL) {
		// More than 16 GB...
		d->disc_size = 0;
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	// Read the block pointers and hashes.
	// NOTE: These are byteswapped on demand, not ahead of time.
	d->blockPointers.resize(d->gczHeader.num_blocks);
	size_t expected_size = d->blockPointers.size() * sizeof(uint64_t);
	size_t size = m_file->read(d->blockPointers.data(), expected_size);
	if (size != expected_size) {
		// Read error.
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		d->disc_size = 0;
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}
	d->hashes.resize(d->gczHeader.num_blocks);
	expected_size = d->hashes.size() * sizeof(uint32_t);
	size = m_file->read(d->hashes.data(), expected_size);
	if (size != expected_size) {
		// Read error.
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		d->disc_size = 0;
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}

	// Data offset is the current read position.
	off64_t pos = m_file->tell();
	if (pos <= 0 || pos >= 1LL*1024*1024*1024) {
		// tell() failed...
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		d->blockPointers.clear();
		d->hashes.clear();
		d->disc_size = 0;
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}
	d->dataOffset = static_cast<uint32_t>(pos);

	// Initialize the block cache and decompression buffer.
	// NOTE: Extra 64 bytes is for zlib, in case it needs it.
	d->blockCache.resize(d->block_size + 64);
	d->z_buffer.resize(d->block_size + 64);
	d->blockCacheIdx = ~0U;

	// Reset the disc position.
	d->pos = 0;
}

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int GczReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < 8) {
		// Not enough data to check.
		return -1;
	}

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// GCZ is not supported without zlib.
		return -1;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Check the GCZ magic.
	const GczHeader *const gczHeader = reinterpret_cast<const GczHeader*>(pHeader);
	if (gczHeader->magic != cpu_to_le32(GCZ_MAGIC)) {
		// Invalid magic.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: GCZ_BLOCK_SIZE_MIN (32 KB, 1 << 15)
	// - Maximum: GCZ_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	const uint32_t block_size = le32_to_cpu(gczHeader->block_size);
	if (!isPow2(block_size) ||
	    block_size < GCZ_BLOCK_SIZE_MIN || block_size > GCZ_BLOCK_SIZE_MAX)
	{
		// Block size is out of range.
		return -1;
	}

	// Verify that if data size is a multiple of the block size, it matches
	// num_blocks, or if not, num_blocks + 1.
	const uint64_t data_size = le64_to_cpu(gczHeader->data_size);
	const uint32_t num_blocks = le32_to_cpu(gczHeader->num_blocks);
	uint64_t expected_data_size;
	if (data_size % block_size == 0) {
		// Multiple of the block size.
		expected_data_size = data_size;
	} else {
		// Not a multiple of the block size.
		// Round it up, then check.
		expected_data_size = ALIGN_BYTES(block_size, data_size);
	}
	if (((uint64_t)block_size * (uint64_t)num_blocks) != expected_data_size) { 
		// Incorrect size.
		return -1;
	}

	// This is a valid GCZ image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GczReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	return isDiscSupported_static(pHeader, szHeader);
}

/** SparseDiscReader functions. **/

/**
 * Get the physical address of the specified logical block index.
 *
 * @param blockIdx	[in] Block index.
 * @return Physical address. (0 == empty block; -1 == invalid block index)
 */
off64_t GczReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	// TODO: Check against maxLogicalBlockUsed?
	RP_D(const GczReader);
	assert(blockIdx < d->blockPointers.size());
	if (blockIdx >= d->blockPointers.size()) {
		// Out of range.
		return -1;
	}

	// Get the physical block address.
	// NOTE: The caller has to decompress the block.
	return (le64_to_cpu(d->blockPointers[blockIdx]) & ~GCZ_FLAG_BLOCK_NOT_COMPRESSED) + d->dataOffset;
}

/**
 * Read the specified block.
 *
 * This can read either a full block or a partial block.
 * For a full block, set pos = 0 and size = block_size.
 *
 * @param blockIdx	[in] Block index.
 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
 * @param ptr		[out] Output data buffer.
 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
 * @return Number of bytes read, or -1 if the block index is invalid.
 */
int GczReader::readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(GczReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert(static_cast<off64_t>(pos + size) <= static_cast<off64_t>(d->block_size));
	if (pos < 0 || static_cast<off64_t>(pos + size) > static_cast<off64_t>(d->block_size)) {
		// pos+size is out of range.
		return -1;
	}

	if (unlikely(size == 0)) {
		// Nothing to read.
		return 0;
	}

	if (blockIdx == d->blockCacheIdx) {
		// Block is cached.
		memcpy(ptr, &d->blockCache[pos], size);
		return static_cast<int>(size);
	}

	// NOTE: If this is the last block, then we might have
	// a short read. We'll allow it.
	const bool isLastBlock = (blockIdx + 1 == d->blockPointers.size());

	// Get the physical address first.
	const uint64_t blockPointer = le64_to_cpu(d->blockPointers[blockIdx]);
	const off64_t physBlockAddr = static_cast<off64_t>(blockPointer & ~GCZ_FLAG_BLOCK_NOT_COMPRESSED) + d->dataOffset;
	const uint32_t z_block_size = d->getBlockCompressedSize(blockIdx);
	if (z_block_size == 0) {
		// Unable to get the block's compressed size...
		m_lastError = EIO;
		return 0;
	}

	bool compressed = (!(blockPointer & GCZ_FLAG_BLOCK_NOT_COMPRESSED));
	if (!compressed) {
		// (Un)compressed block size must match the actual block size.
		if (z_block_size != d->block_size) {
			// Error...
			m_lastError = EIO;
			return 0;
		}
	}

	if (!compressed) {
		// Reading uncompressed data directly into the cache.
		if (isLastBlock) {
			memset(d->blockCache.data(), 0, d->blockCache.size());
		}

		size_t sz_read = m_file->seekAndRead(physBlockAddr, d->blockCache.data(), z_block_size);
		if (sz_read != z_block_size && !isLastBlock) {
			// Seek and/or read error.
			d->blockCacheIdx = ~0U;
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}
		d->blockCacheIdx = blockIdx;
	} else {
		// Read compressed data into a temporary buffer,
		// then decompress it.
		if (z_block_size > d->block_size) {
			// Compressed data is larger than the uncompressed block size...
			m_lastError = EIO;
			return 0;
		}

		size_t sz_read = m_file->seekAndRead(physBlockAddr, d->z_buffer.data(), z_block_size);
		if (sz_read != z_block_size) {
			// Seek and/or read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			return 0;
		}

		// Verify the hash of the *compressed* data.
		uint32_t hash_calc = adler32(0L, Z_NULL, 0);
		hash_calc = adler32(hash_calc, d->z_buffer.data(), z_block_size);
		if (hash_calc != le32_to_cpu(d->hashes[blockIdx])) {
			// Hash error.
			// TODO: Print warnings and/or more comprehensive error codes.
			m_lastError = EIO;
			return 0;
		}

		// Decompress the data.
		z_stream z = { };
		z.next_in = d->z_buffer.data();
		z.avail_in = z_block_size;
		z.next_out = d->blockCache.data();
		z.avail_out = d->block_size;
		inflateInit(&z);

		int status = inflate(&z, Z_FULL_FLUSH);
		const uint32_t uncomp_size = d->block_size - z.avail_out;
		inflateEnd(&z);

		if (status != Z_STREAM_END || uncomp_size != d->block_size) {
			// Decompression error.
			// TODO: Print warnings and/or more comprehensive error codes.
			d->blockCacheIdx = ~0U;
			m_lastError = EIO;
			return 0;
		}
	}

	// Block has been loaded into the cache.
	memcpy(ptr, &d->blockCache[pos], size);
	d->blockCacheIdx = blockIdx;
	return static_cast<int>(size);
}

}

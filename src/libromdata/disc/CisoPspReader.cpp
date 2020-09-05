/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspReader.cpp: PlayStation Portable CISO disc image reader.         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/unknownbrackets/maxcso/blob/master/README_CSO.md

#include "stdafx.h"
#include "config.librpbase.h"

#include "CisoPspReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "ciso_psp_structs.h"

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
DELAYLOAD_TEST_FUNCTION_IMPL0(zlibVersion);
#endif /* _MSC_VER */

class CisoPspReaderPrivate : public SparseDiscReaderPrivate {
	public:
		CisoPspReaderPrivate(CisoPspReader *q);

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(CisoPspReaderPrivate)

	public:
		// CISO PSP header.
		CisoPspHeader cisoPspHeader;

		// Index entries. These are *absolute* offsets.
		// High bit interpretation depends on CISO version.
		// - v0/v1: If set, block is not compressed.
		// - v2: If set, block is compressed using LZ4; otherwise, deflate.
		ao::uvector<uint32_t> indexEntries;

		// Block cache.
		ao::uvector<uint8_t> blockCache;
		uint32_t blockCacheIdx;

		// Decompression buffer.
		// (Same size as blockCache.)
		ao::uvector<uint8_t> z_buffer;

		/**
		 * Get the compressed size of a block.
		 * @param blockNum Block number.
		 * @return Block's compressed size, or 0 on error.
		 */
		uint32_t getBlockCompressedSize(uint32_t blockNum) const;
};

/** CisoPspReaderPrivate **/

CisoPspReaderPrivate::CisoPspReaderPrivate(CisoPspReader *q)
	: super(q)
	, blockCacheIdx(~0U)
{
	// Clear the GCZ header struct.
	memset(&cisoPspHeader, 0, sizeof(cisoPspHeader));
}

/**
 * Get the compressed size of a block.
 * @param blockNum Block number.
 * @return Block's compressed size, or 0 on error.
 */
uint32_t CisoPspReaderPrivate::getBlockCompressedSize(uint32_t blockNum) const
{
	assert(!indexEntries.empty());
	assert(blockNum < indexEntries.size()-1);
	if (indexEntries.empty() || blockNum >= indexEntries.size()-1) {
		// Out of range.
		return 0;
	}

	// NOTE: Index entry table has an extra entry for the final block.
	// Hence, we don't need the same workaround as GCZ.

	// High bit is reserved as a flag for all CISO versions.
	const uint32_t idxStart = le32_to_cpu(indexEntries[blockNum]) & ~CISO_PSP_V0_NOT_COMPRESSED;
	const uint32_t idxEnd = le32_to_cpu(indexEntries[blockNum + 1]) & ~CISO_PSP_V0_NOT_COMPRESSED;
	return static_cast<uint32_t>(idxEnd - idxStart);
}

/** CisoPspReader **/

CisoPspReader::CisoPspReader(IRpFile *file)
	: super(new CisoPspReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlibVersion() != 0) {
		// Delay load failed.
		// GCZ is not supported without zlib.
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Read the CISO header.
	RP_D(CisoPspReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->cisoPspHeader, sizeof(d->cisoPspHeader));
	if (sz != sizeof(d->cisoPspHeader)) {
		// Error reading the GCZ header.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	// Reuse isDiscSupported_static() for validation.
	if (isDiscSupported_static(reinterpret_cast<const uint8_t*>(&d->cisoPspHeader), sizeof(d->cisoPspHeader)) < 0) {
		// Not valid.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswap the header.
	d->cisoPspHeader.magic			= le32_to_cpu(d->cisoPspHeader.magic);
	d->cisoPspHeader.header_size		= le32_to_cpu(d->cisoPspHeader.header_size);
	d->cisoPspHeader.uncompressed_size	= le64_to_cpu(d->cisoPspHeader.uncompressed_size);
	d->cisoPspHeader.block_size		= le32_to_cpu(d->cisoPspHeader.block_size);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	// Calculate the number of blocks.
	d->block_size = d->cisoPspHeader.block_size;
	const uint32_t num_blocks = d->cisoPspHeader.uncompressed_size / d->block_size;
	assert(num_blocks > 0);
	if (num_blocks == 0) {
		// No blocks...
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}

	// Read the index entries.
	// NOTE: These are byteswapped on demand, not ahead of time.
	d->indexEntries.resize(num_blocks);
	size_t expected_size = (num_blocks + 1) * sizeof(uint32_t);
	size_t size = m_file->read(d->indexEntries.data(), expected_size);
	if (size != expected_size) {
		// Read error.
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}

	// Use the disc size directly from the header.
	// TODO: Verify it?
	d->disc_size = d->cisoPspHeader.uncompressed_size;

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
int CisoPspReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < sizeof(CisoPspHeader)) {
		// Not enough data to check.
		return -1;
	}

#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlibVersion() != 0) {
		// Delay load failed.
		// GCZ is not supported without zlib.
		return -1;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Check the CISO magic.
	const CisoPspHeader *const cisoPspHeader = reinterpret_cast<const CisoPspHeader*>(pHeader);
	if (cisoPspHeader->magic != be32_to_cpu(CISO_MAGIC)) {
		// Invalid magic.
		return -1;
	}

	// Header size should be either 0x18 or 0.
	// If it's v2, it *must* be 0x18.
	if (cisoPspHeader->header_size == 0) {
		if (cisoPspHeader->version >= 2) {
			// Invalid header size.
			return -1;
		}
	} else if (cisoPspHeader->header_size != cpu_to_le32(sizeof(*cisoPspHeader))) {
		// Invalid header size.
		return -1;
	}

	// Check if the block size is a supported power of two.
	// - Minimum: CISO_PSP_BLOCK_SIZE_MIN ( 2 KB, 1 << 11)
	// - Maximum: CISO_PSP_BLOCK_SIZE_MAX (16 MB, 1 << 24)
	const uint32_t block_size = le32_to_cpu(cisoPspHeader->block_size);
	if (!isPow2(block_size) ||
	    block_size < CISO_PSP_BLOCK_SIZE_MIN || block_size > CISO_PSP_BLOCK_SIZE_MAX)
	{
		// Block size is out of range.
		return -1;
	}

	// Must have at least one block and less than 16 GB of uncompressed data.
	const uint64_t uncompressed_size = le64_to_cpu(cisoPspHeader->uncompressed_size);
	if (uncompressed_size < cisoPspHeader->block_size ||
	    uncompressed_size > 16ULL*1024ULL*1024ULL*1024ULL)
	{
		// Less than one block, or more than 16 GB...
		return -1;
	}

	// Uncompressed data size must be a multiple of the block size.
	if (uncompressed_size % block_size != 0) {
		// Not a multiple.
		return -1;
	}

	// This is a valid CISO PSP image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int CisoPspReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
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
off64_t CisoPspReader::getPhysBlockAddr(uint32_t blockIdx) const
{
	// Make sure the block index is in range.
	// TODO: Check against maxLogicalBlockUsed?
	RP_D(CisoPspReader);
	assert(!d->indexEntries.empty());
	assert(blockIdx < d->indexEntries.size()-1);
	if (d->indexEntries.empty() || blockIdx >= d->indexEntries.size()-1) {
		// Out of range.
		return -1;
	}

	// Get the physical block address.
	// NOTE: The caller has to decompress the block.
	return (d->indexEntries[blockIdx] & ~CISO_PSP_V0_NOT_COMPRESSED);
}

/**
 * Read the specified block.
 *
 * This can read either a full block or a partial block.
 * For a full block, set pos = 0 and size = block_size.
 *
 * @param blockIdx	[in] Block index.
 * @param ptr		[out] Output data buffer.
 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
 * @return Number of bytes read, or -1 if the block index is invalid.
 */
int CisoPspReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(CisoPspReader);
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
		return size;
	}

	// Get the physical address first.
	const uint32_t indexEntry = d->indexEntries[blockIdx];
	const off64_t physBlockAddr = static_cast<off64_t>(indexEntry & ~CISO_PSP_V0_NOT_COMPRESSED);
	const uint32_t z_block_size = d->getBlockCompressedSize(blockIdx);
	if (z_block_size == 0) {
		// Unable to get the block's compressed size...
		m_lastError = EIO;
		return 0;
	}

	enum class CompressionMode {
		None = 0,
		Deflate = 1,
		LZ4 = 2,
	};
	CompressionMode z_mode;
	if (d->cisoPspHeader.version < 2) {
		// CISO v0/v1: Check if compressed.
		z_mode = (physBlockAddr & CISO_PSP_V0_NOT_COMPRESSED)
			? CompressionMode::None
			: CompressionMode::Deflate;

		if (z_mode == CompressionMode::None) {
			// (Un)compressed block size must match the actual block size.
			if (z_block_size != d->block_size) {
				// Error...
				m_lastError = EIO;
				return 0;
			}
		}
	} else {
		// CISO v2: Check if compressed, and if so, which algorithm.
		if (z_block_size == d->block_size) {
			z_mode = CompressionMode::None;
		} else {
			z_mode = (physBlockAddr & CISO_PSP_V2_LZ4_COMPRESSED)
				? CompressionMode::LZ4
				: CompressionMode::Deflate;
		}
	}

	switch (z_mode) {
		default:
			assert(!"Compression mode not supported...");
			m_lastError = ENOTSUP;
			return 0;

		case CompressionMode::None: {
			// Reading uncompressed data directly into the cache.
			size_t sz_read = m_file->seekAndRead(physBlockAddr, d->blockCache.data(), z_block_size);
			if (sz_read != z_block_size) {
				// Seek and/or read error.
				d->blockCacheIdx = ~0U;
				m_lastError = m_file->lastError();
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				return 0;
			}
			d->blockCacheIdx = blockIdx;
			break;
		}

		case CompressionMode::Deflate: {
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

			// Decompress the data.
			z_stream z = { };
			z.next_in = d->z_buffer.data();
			z.avail_in = z_block_size;
			z.next_out = d->blockCache.data();
			z.avail_out = d->block_size;
			// CISO uses raw deflate. (-15)
			// DAX uses regular deflate. (15)
			inflateInit2(&z, -15);

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
			break;
		}
	}

	// Block has been loaded into the cache.
	memcpy(ptr, &d->blockCache[pos], size);
	d->blockCacheIdx = blockIdx;
	return size;
}

}

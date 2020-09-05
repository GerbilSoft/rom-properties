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
		enum class CisoType {
			Unknown	= -1,

			CISO	= 0,	// CISO
			DAX	= 1,	// DAX

			Max
		};
		CisoType cisoType;

		// Header.
		union {
			CisoPspHeader cisoPsp;
			DaxHeader dax;
		} header;

		// Index entries. These are *absolute* offsets.
		// High bit interpretation depends on CISO version.
		// - v0/v1: If set, block is not compressed.
		// - v2: If set, block is compressed using LZ4; otherwise, deflate.
		ao::uvector<uint32_t> indexEntries;

		// DAX: Size and NC area tables.
		ao::uvector<uint16_t> daxSizeTable;
		ao::uvector<uint8_t> daxNCTable;	// 0 = compressed; 1 = not compressed
		bool isDaxWithoutNCTable;

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
	, cisoType(CisoType::Unknown)
	, isDaxWithoutNCTable(false)
	, blockCacheIdx(~0U)
{
	// Clear the header structs.
	memset(&header, 0, sizeof(header));
}

/**
 * Get the compressed size of a block.
 * @param blockNum Block number.
 * @return Block's compressed size, or 0 on error.
 */
uint32_t CisoPspReaderPrivate::getBlockCompressedSize(uint32_t blockNum) const
{
	assert(!indexEntries.empty());
	assert(blockNum < indexEntries.size());
	if (indexEntries.empty() || blockNum >= indexEntries.size()) {
		// Out of range.
		return 0;
	}

	uint32_t size = 0;
	switch (cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			return 0;

		case CisoPspReaderPrivate::CisoType::CISO:
			// Index entry table has an extra entry for the final block.
			// Hence, we don't need the same workaround as GCZ.
			assert(blockNum != indexEntries.size()-1);
			if (blockNum < indexEntries.size()-1) {
				// High bit is reserved as a flag for all CISO versions.
				const uint32_t idxStart = le32_to_cpu(indexEntries[blockNum]) & ~CISO_PSP_V0_NOT_COMPRESSED;
				const uint32_t idxEnd = le32_to_cpu(indexEntries[blockNum + 1]) & ~CISO_PSP_V0_NOT_COMPRESSED;
				size = static_cast<uint32_t>(idxEnd - idxStart);
			}
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
			// DAX uses a separate size table.
			size = static_cast<uint32_t>(daxSizeTable[blockNum]);
			break;
	}
	return size;
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

	// Read the header.
	RP_D(CisoPspReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->header, sizeof(d->header));
	if (sz != sizeof(d->header)) {
		// Error reading the header.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	// Reuse isDiscSupported_static() for validation.
	d->cisoType = static_cast<CisoPspReaderPrivate::CisoType>(
		isDiscSupported_static(reinterpret_cast<const uint8_t*>(&d->header), sizeof(d->header)));
	if ((int)d->cisoType < 0) {
		// Not valid.
		UNREF_AND_NULL_NOCHK(m_file);
		m_lastError = EIO;
		return;
	}

	unsigned int indexEntryTblPos;
	switch (d->cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			UNREF_AND_NULL_NOCHK(m_file);
			m_lastError = ENOTSUP;
			return;

		case CisoPspReaderPrivate::CisoType::CISO:
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the header.
			d->header.cisoPsp.magic			= le32_to_cpu(d->header.cisoPsp.magic);
			d->header.cisoPsp.header_size		= le32_to_cpu(d->header.cisoPsp.header_size);
			d->header.cisoPsp.uncompressed_size	= le64_to_cpu(d->header.cisoPsp.uncompressed_size);
			d->header.cisoPsp.block_size		= le32_to_cpu(d->header.cisoPsp.block_size);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

			d->block_size = d->header.cisoPsp.block_size;
			d->disc_size = d->header.cisoPsp.uncompressed_size;
			indexEntryTblPos = static_cast<off64_t>(sizeof(d->header.cisoPsp));
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the header.
			d->header.dax.magic		= le32_to_cpu(d->header.dax.magic);
			d->header.dax.uncompressed_size	= le32_to_cpu(d->header.dax.uncompressed_size);
			d->header.dax.version		= le32_to_cpu(d->header.dax.version);
			d->header.dax.nc_areas		= le32_to_cpu(d->header.dax.nc_areas);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

			d->block_size = DAX_BLOCK_SIZE;
			d->disc_size = d->header.dax.uncompressed_size;
			// FIXME: Handle NC areas.
			indexEntryTblPos = static_cast<off64_t>(sizeof(d->header.dax));
			break;
	}

	// Calculate the number of blocks.
	const uint32_t num_blocks = static_cast<uint32_t>(d->disc_size / d->block_size);
	assert(num_blocks > 0);
	if (num_blocks == 0) {
		// No blocks...
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}

	// Read the index entries.
	// NOTE: These are byteswapped on demand, not ahead of time.
	d->indexEntries.resize(num_blocks);
	size_t expected_size = num_blocks * sizeof(uint32_t);
	if (d->cisoType == CisoPspReaderPrivate::CisoType::CISO) {
		// CISO has an extra entry for proper size handling.
		expected_size += sizeof(uint32_t);
	}
	size_t size = m_file->seekAndRead(indexEntryTblPos, d->indexEntries.data(), expected_size);
	if (size != expected_size) {
		// Read error.
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		UNREF_AND_NULL_NOCHK(m_file);
		return;
	}

	if (d->cisoType == CisoPspReaderPrivate::CisoType::DAX) {
		// Read the DAX size table.
		d->daxSizeTable.resize(num_blocks);
		expected_size = num_blocks * sizeof(uint16_t);
		size = m_file->read(d->daxSizeTable.data(), expected_size);
		if (size != expected_size) {
			// Read error.
			m_lastError = m_file->lastError();
			if (m_lastError == 0) {
				m_lastError = EIO;
			}
			UNREF_AND_NULL_NOCHK(m_file);
			return;
		}

		if (d->header.dax.nc_areas > 0) {
			// Handle the NC (non-compressed) areas.
			// This table is stored immediately after the index entry table.
			ao::uvector<DaxNCArea> nc_areas;
			nc_areas.resize(d->header.dax.nc_areas);
			expected_size = d->header.dax.nc_areas * sizeof(DaxNCArea);
			size = m_file->read(nc_areas.data(), expected_size);
			if (size != expected_size) {
				// Read error.
				m_lastError = m_file->lastError();
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				UNREF_AND_NULL_NOCHK(m_file);
				return;
			}

			// Process the NC areas.
			d->daxNCTable.resize(num_blocks);
			const auto nc_areas_end = nc_areas.cend();
			for (auto iter = nc_areas.cbegin(); iter != nc_areas_end; ++iter) {
				uint32_t i = le32_to_cpu(iter->start);
				const uint32_t end = i + le32_to_cpu(iter->count);
				if (end > num_blocks) {
					// Out of range...
					m_lastError = EIO;
					UNREF_AND_NULL_NOCHK(m_file);
					return;
				}

				// TODO: Assert on overlapping NC areas?
				for (; i < end; i++) {
					d->daxNCTable[i] = true;
				}
			}
		} else {
			// No NC areas.
			d->isDaxWithoutNCTable = true;
		}
	}

	// Initialize the block cache and decompression buffer.
	// NOTE: Extra 64 bytes is for zlib, in case it needs it.
	size_t cache_size = d->block_size + 64;
	if (d->isDaxWithoutNCTable) {
		// DAX with no NC table. Use double the block size,
		// since zlib-compressed data can end up taking up
		// more space than uncompressed.
		cache_size *= 2;
	}
	d->blockCache.resize(cache_size);
	d->z_buffer.resize(cache_size);
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

	// Check the magic number.
	const uint32_t *const pu32 = reinterpret_cast<const uint32_t*>(pHeader);
	if (*pu32 == be32_to_cpu(CISO_MAGIC) && szHeader >= sizeof(CisoPspHeader)) {
		// CISO magic.
		const CisoPspHeader *const cisoPspHeader = reinterpret_cast<const CisoPspHeader*>(pHeader);

		// Header size should be either 0x18 or 0.
		// If it's v2, it *must* be 0x18.
		if (cisoPspHeader->header_size == 0) {
			if (cisoPspHeader->version >= 2) {
				// Invalid header size.
				return -1;
			}
		} else if (cisoPspHeader->header_size != cpu_to_le32(sizeof(*cisoPspHeader))) {
			// Invalid header size.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Check if the block size is a supported power of two.
		// - Minimum: CISO_PSP_BLOCK_SIZE_MIN ( 2 KB, 1 << 11)
		// - Maximum: CISO_PSP_BLOCK_SIZE_MAX (16 MB, 1 << 24)
		const uint32_t block_size = le32_to_cpu(cisoPspHeader->block_size);
		if (!isPow2(block_size) ||
		    block_size < CISO_PSP_BLOCK_SIZE_MIN || block_size > CISO_PSP_BLOCK_SIZE_MAX)
		{
			// Block size is out of range.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Must have at least one block and less than 16 GB of uncompressed data.
		const uint64_t uncompressed_size = le64_to_cpu(cisoPspHeader->uncompressed_size);
		if (uncompressed_size < cisoPspHeader->block_size ||
		    uncompressed_size > 16ULL*1024ULL*1024ULL*1024ULL)
		{
			// Less than one block, or more than 16 GB...
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Uncompressed data size must be a multiple of the block size.
		if (uncompressed_size % block_size != 0) {
			// Not a multiple.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// This is a valid CISO PSP image.
		return (int)CisoPspReaderPrivate::CisoType::CISO;
	} else if (*pu32 == be32_to_cpu(DAX_MAGIC) && szHeader >= sizeof(DaxHeader)) {
		// DAX magic.
		const DaxHeader *const daxHeader = reinterpret_cast<const DaxHeader*>(pHeader);

		// Version must be 0 or 1.
		if (le32_to_cpu(daxHeader->version) > 1) {
			// Not supported.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Must have at least one block and less than 4 GB of uncompressed data.
		// NOTE: 4 GB is the implied maximum size due to use of uint32_t.
		const uint32_t uncompressed_size = le32_to_cpu(daxHeader->uncompressed_size);
		if (uncompressed_size < DAX_BLOCK_SIZE) {
			// Less than one block...
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Uncompressed data size must be a multiple of the block size.
		if (uncompressed_size % DAX_BLOCK_SIZE != 0) {
			// Not a multiple.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// This is a valid DAX image.
		return (int)CisoPspReaderPrivate::CisoType::DAX;
	}

	// Not supported.
	return (int)CisoPspReaderPrivate::CisoType::Unknown;
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
	const uint32_t z_block_size = d->getBlockCompressedSize(blockIdx);
	if (z_block_size == 0) {
		// Unable to get the block's compressed size...
		m_lastError = EIO;
		return 0;
	}

	// Mask the high bit for CISO format.
	uint32_t physBlockAddr = indexEntry;
	if (likely(d->cisoType == CisoPspReaderPrivate::CisoType::CISO)) {
		physBlockAddr &= ~CISO_PSP_V0_NOT_COMPRESSED;
	}

	enum class CompressionMode {
		None = 0,
		Deflate = 1,
		LZ4 = 2,
	};
	CompressionMode z_mode;
	int windowBits = -15;	// raw deflate (CISO)

	switch (d->cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			UNREF_AND_NULL_NOCHK(m_file);
			m_lastError = ENOTSUP;
			return 0;

		case CisoPspReaderPrivate::CisoType::CISO:
			// CISO uses raw deflate.
			//windowBits = -15;

			if (d->header.cisoPsp.version < 2) {
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
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
			if (d->header.dax.nc_areas > 0 && d->daxNCTable[blockIdx]) {
				// Uncompressed block.
				z_mode = CompressionMode::None;
			} else {
				// Compressed block.
				// DAX uses zlib deflate.
				windowBits = 15;
				z_mode = CompressionMode::Deflate;
			}
			break;
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
			uint32_t z_max_size = d->block_size;
			if (unlikely(d->isDaxWithoutNCTable)) {
				// DAX without NC table can end up compressing to larger
				// than the uncompressed size.
				z_max_size *= 2;
			}
			if (z_block_size > z_max_size) {
				// Compressed data is larger than the uncompressed block size.
				// This is only allowed for DAX without NC table.
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
			inflateInit2(&z, windowBits);

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

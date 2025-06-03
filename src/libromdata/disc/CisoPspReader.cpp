/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * CisoPspReader.cpp: PlayStation Portable CISO disc image reader.         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - https://github.com/unknownbrackets/maxcso/blob/master/README_CSO.md

#include "stdafx.h"
#include "config.librpbase.h"
#include "config.libromdata.h"

#include "CisoPspReader.hpp"
#include "librpbase/disc/SparseDiscReader_p.hpp"
#include "ciso_psp_structs.h"

// zlib
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  define dlsym(handle, symbol)	GetProcAddress((handle), (symbol))
#  define dlclose(handle)	FreeLibrary(handle)
#else /* !_WIN32 */
#  include <dlfcn.h>	// for dlopen()
typedef void *HMODULE;
#endif /* !_WIN32 */

#ifdef HAVE_LZ4
#  if defined(USE_INTERNAL_LZ4) && !defined(USE_INTERNAL_LZ4_DLL)
     // Using a statically linked copy of LZ4.
#    define LZ4_DIRECT_LINKAGE 1
#  else /* !(USE_INTERNAL_LZ4 && USE_INTERNAL_LZ4_DLL) */
     // Using a shared library copy of LZ4.
#    define LZ4_SHARED_LINKAGE 1
#  endif /* USE_INTERNAL_LZ4 && USE_INTERNAL_LZ4_DLL */
#  include <lz4.h>
#endif /* HAVE_LZ4 */

// LZO (JISO)
// NOTE: The bundled version is MiniLZO.
#ifdef HAVE_LZO
#  if defined(USE_INTERNAL_LZO) && !defined(USE_INTERNAL_LZO_DLL)
     // Using a statically linked copy of LZO.
#    define LZO_DIRECT_LINKAGE 1
#  else /* !(USE_INTERNAL_LZO && USE_INTERNAL_LZO_DLL) */
     // Using a shared library copy of LZO.
#    define LZO_SHARED_LINKAGE 1
#  endif /* USE_INTERNAL_LZO && USE_INTERNAL_LZO_DLL */
#  ifdef USE_INTERNAL_LZO
#    include "minilzo.h"
#  else
#    include <lzo/lzo1x.h>
#  endif
#endif /* HAVE_LZO */

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class CisoPspReaderPrivate : public SparseDiscReaderPrivate
{
public:
	explicit CisoPspReaderPrivate(CisoPspReader *q);
	~CisoPspReaderPrivate();

private:
	typedef SparseDiscReaderPrivate super;
	RP_DISABLE_COPY(CisoPspReaderPrivate)

public:
	enum class CisoType {
		Unknown	= -1,

		CISO	= 0,
		ZISO	= 1,
		JISO	= 2,
		DAX	= 3,

		Max
	};
	CisoType cisoType;

	// Header
	union {
		CisoPspHeader cisoPsp;
		JisoHeader jiso;
		DaxHeader dax;
	} header;

	// Index entries. These are *absolute* offsets.
	// High bit interpretation depends on CISO version.
	// - v0/v1: If set, block is not compressed.
	// - v2: If set, block is compressed using LZ4; otherwise, deflate.
	rp::uvector<uint32_t> indexEntries;

	// Block cache
	rp::uvector<uint8_t> blockCache;
	uint32_t blockCacheIdx;

	// Decompression buffer
	// (Same size as blockCache.)
	rp::uvector<uint8_t> z_buffer;

	// DAX: Size and NC area tables
	rp::uvector<uint16_t> daxSizeTable;
	std::vector<uint8_t> daxNCTable;	// 0 = compressed; 1 = not compressed

	uint8_t index_shift;		// Index shift value (CISO/ZISO only)
	bool isDaxWithoutNCTable;	// Convenience variable

	/**
	 * Get the compressed size of a block.
	 * @param blockNum Block number.
	 * @return Block's compressed size, or 0 on error.
	 */
	uint32_t getBlockCompressedSize(uint32_t blockNum) const;

public:
	/** Function pointers (for shared linkage via dlopen()) **/
	// TODO: Shared object with reference counting?

#ifdef LZ4_SHARED_LINKAGE
	// LZ4
	HMODULE liblz4;

	typedef __typeof__(LZ4_decompress_safe) *pfn_LZ4_decompress_safe_t;
	pfn_LZ4_decompress_safe_t pfn_LZ4_decompress_safe;

	/**
	 * Initialize the LZ4 function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init_pfn_LZ4(void);
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
	// LZO
	HMODULE liblzo2;

	typedef __typeof__(lzo1x_decompress_safe) *pfn_lzo1x_decompress_safe_t;
	pfn_lzo1x_decompress_safe_t pfn_lzo1x_decompress_safe;

	/**
	 * Initialize the LZO function pointers.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int init_pfn_LZO(void);
#endif /* LZO_SHARED_LINKAGE */
};

/** CisoPspReaderPrivate **/

CisoPspReaderPrivate::CisoPspReaderPrivate(CisoPspReader *q)
	: super(q)
	, cisoType(CisoType::Unknown)
	, blockCacheIdx(~0U)
	, index_shift(0)
	, isDaxWithoutNCTable(false)
#ifdef LZ4_SHARED_LINKAGE
	, liblz4(nullptr)
	, pfn_LZ4_decompress_safe(nullptr)
#endif /* LZ4_SHARED_LINKAGE */
#ifdef LZO_SHARED_LINKAGE
	, liblzo2(nullptr)
	, pfn_lzo1x_decompress_safe(nullptr)
#endif /* LZO_SHARED_LINKAGE */
{
	// Clear the header structs.
	memset(&header, 0, sizeof(header));
}

CisoPspReaderPrivate::~CisoPspReaderPrivate()
{
	// Close dlopen()'d libraries, if necessary.
#ifdef LZ4_SHARED_LINKAGE
	if (liblz4) {
		dlclose(liblz4);
	}
#endif /* LZ4_SHARED_LINKAGE */
#ifdef LZO_SHARED_LINKAGE
	if (liblzo2) {
		dlclose(liblzo2);
	}
#endif /* LZO_SHARED_LINKAGE */
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
		case CisoPspReaderPrivate::CisoType::ZISO:
			// Index entry table has an extra entry for the final block.
			// Hence, we don't need the same workaround as GCZ.
			assert(blockNum != indexEntries.size()-1);
			if (blockNum < indexEntries.size()-1) {
				// High bit is reserved as a flag for all CISO versions.
				off64_t idxStart = le32_to_cpu(indexEntries[blockNum]) & ~CISO_PSP_V0_NOT_COMPRESSED;
				off64_t idxEnd = le32_to_cpu(indexEntries[blockNum + 1]) & ~CISO_PSP_V0_NOT_COMPRESSED;
				idxStart <<= index_shift;
				idxEnd <<= index_shift;
				size = static_cast<uint32_t>(idxEnd - idxStart);
			}
			break;

		case CisoPspReaderPrivate::CisoType::JISO:
			// Index entry table has an extra entry for the final block.
			// Hence, we don't need the same workaround as GCZ.
			// NOTE: JISO doesn't use the high bit for NC.
			assert(blockNum != indexEntries.size()-1);
			if (blockNum < indexEntries.size()-1) {
				off64_t idxStart = le32_to_cpu(indexEntries[blockNum]);
				off64_t idxEnd = le32_to_cpu(indexEntries[blockNum + 1]);
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

#ifdef LZ4_SHARED_LINKAGE
/**
 * Initialize the LZ4 function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspReaderPrivate::init_pfn_LZ4(void)
{
	if (liblz4) {
		// Already loaded.
		return 0;
	}

#ifdef _WIN32
#  ifndef NDEBUG
#    define LZ4_DLL_FILENAME "lz4d.dll"
#  else /* NDEBUG */
#    define LZ4_DLL_FILENAME "lz4.dll"
#  endif /* NDEBUG */
	HMODULE lib = rp_LoadLibrary(LZ4_DLL_FILENAME);
#else /* !_WIN32 */
	HMODULE lib = dlopen("liblz4.so.1", RTLD_LOCAL|RTLD_NOW);
#endif /* _WIN32 */
	if (!lib) {
		// NOTE: dlopen() does not set errno, but it does have dlerror().
		return -EIO;	// TODO: Better error code.
	}

	// Attempt to load the function pointers.
	pfn_LZ4_decompress_safe = reinterpret_cast<pfn_LZ4_decompress_safe_t>(dlsym(lib, "LZ4_decompress_safe"));
	if (!pfn_LZ4_decompress_safe) {
		// Failed to load the function pointers.
		dlclose(lib);
		return -EIO;	// TODO: Better error code.
	}

	// Function pointers loaded.
	liblz4 = lib;
	return 0;
}
#endif /* LZ4_SHARED_LINKAGE */

#ifdef LZO_SHARED_LINKAGE
/**
 * Initialize the LZO function pointers.
 * @return 0 on success; negative POSIX error code on error.
 */
int CisoPspReaderPrivate::init_pfn_LZO(void)
{
	if (liblzo2) {
		// Already loaded.
		return 0;
	}

#ifdef _WIN32
#  ifndef NDEBUG
#    define LZ4_DLL_FILENAME "minilzod.dll"
#  else /* NDEBUG */
#    define LZ4_DLL_FILENAME "minilzo.dll"
#  endif /* NDEBUG */
	HMODULE lib = rp_LoadLibrary(LZ4_DLL_FILENAME);
#else /* !_WIN32 */
	HMODULE lib = dlopen("liblzo2.so.2", RTLD_LOCAL|RTLD_NOW);
#endif /* _WIN32 */
	if (!lib) {
		// NOTE: dlopen() does not set errno, but it does have dlerror().
		return -EIO;	// TODO: Better error code.
	}

	// Load the __lzo_init_v2 function pointer first and initialize LZO.
	typedef __typeof__(__lzo_init_v2) *pfn___lzo_init_v2_t;
	pfn___lzo_init_v2_t pfn___lzo_init_v2 = reinterpret_cast<pfn___lzo_init_v2_t>(dlsym(lib, "__lzo_init_v2"));
	if (!pfn___lzo_init_v2) {
		// Failed to load the LZO initialization function pointer.
		dlclose(lib);
		return -EIO;	// TODO: Better error code.
	}

	// Initialize the LZO library.
	// Based on the lzo_init() macro from lzoconf.h.
	int ret = pfn___lzo_init_v2(LZO_VERSION, (int)sizeof(short), (int)sizeof(int),
		(int)sizeof(long), (int)sizeof(lzo_uint32_t), (int)sizeof(lzo_uint),
		(int)lzo_sizeof_dict_t, (int)sizeof(char *),(int)sizeof(lzo_voidp),
		(int)sizeof(lzo_callback_t));
	if (ret != LZO_E_OK) {
		// Failed to initialize LZO.
		dlclose(lib);
		return -EIO;	// TODO: Better error code.
	}

	// Attempt to load the remaining function pointers.
	pfn_lzo1x_decompress_safe = reinterpret_cast<pfn_lzo1x_decompress_safe_t>(dlsym(lib, "lzo1x_decompress_safe"));
	if (!pfn_lzo1x_decompress_safe) {
		// Failed to load the function pointers.
		dlclose(lib);
		return -EIO;	// TODO: Better error code.
	}

	// Function pointers loaded.
	liblzo2 = lib;
	return 0;
}
#endif /* LZO_SHARED_LINKAGE */

/** CisoPspReader **/

CisoPspReader::CisoPspReader(const IRpFilePtr &file)
	: super(new CisoPspReaderPrivate(this), file)
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the header.
	RP_D(CisoPspReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->header, sizeof(d->header));
	if (sz != sizeof(d->header)) {
		// Error reading the header.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Reuse isDiscSupported_static() for validation.
	d->cisoType = static_cast<CisoPspReaderPrivate::CisoType>(
		isDiscSupported_static(reinterpret_cast<const uint8_t*>(&d->header), sizeof(d->header)));
	if ((int)d->cisoType < 0) {
		// Not valid.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	unsigned int indexEntryTblPos;
	bool isZlib = false;	// If zlib, we have to call get_crc_table().
#ifdef LZ4_SHARED_LINKAGE
	bool isLZ4 = false;
#endif /* LZ4_SHARED_LINKAGE */
#ifdef LZO_SHARED_LINKAGE
	bool isLZO = false;
#endif /* LZO_SHARED_LINKAGE */
	switch (d->cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			m_file.reset();
			m_lastError = ENOTSUP;
			return;

		case CisoPspReaderPrivate::CisoType::CISO:
			isZlib = true;
			// fall-through
		case CisoPspReaderPrivate::CisoType::ZISO:
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the header.
			d->header.cisoPsp.magic			= le32_to_cpu(d->header.cisoPsp.magic);
			d->header.cisoPsp.header_size		= le32_to_cpu(d->header.cisoPsp.header_size);
			d->header.cisoPsp.uncompressed_size	= le64_to_cpu(d->header.cisoPsp.uncompressed_size);
			d->header.cisoPsp.block_size		= le32_to_cpu(d->header.cisoPsp.block_size);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

			d->block_size = d->header.cisoPsp.block_size;
			d->disc_size = d->header.cisoPsp.uncompressed_size;
			d->index_shift = d->header.cisoPsp.index_shift;
			indexEntryTblPos = static_cast<off64_t>(sizeof(d->header.cisoPsp));

#ifdef LZ4_SHARED_LINKAGE
			// If CISOv2, we might also have LZ4.
			// If ZISO, we *definitely* have LZ4.
			if (d->header.cisoPsp.version >= 2 ||
			    d->cisoType == CisoPspReaderPrivate::CisoType::ZISO)
			{
				isLZ4 = true;
			}
#endif /* !LZ4_SHARED_LINKAGE */
			break;

		case CisoPspReaderPrivate::CisoType::JISO:
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the header.
			d->header.jiso.magic			= le32_to_cpu(d->header.jiso.magic);
			d->header.jiso.block_size		= le16_to_cpu(d->header.jiso.block_size);
			d->header.jiso.uncompressed_size	= le32_to_cpu(d->header.jiso.uncompressed_size);
			d->header.jiso.header_size		= le32_to_cpu(d->header.jiso.block_size);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

			// Determine which library should be checked
			// by the Delay Load helper.
			switch (d->header.jiso.method) {
				default:
					break;

#ifdef LZO_SHARED_LINKAGE
				case JISO_METHOD_LZO:
					isLZO = true;
					break;
#endif /* LZO_SHARED_LINKAGE */

				case JISO_METHOD_ZLIB:
					isZlib = true;
					break;
			}

			d->block_size = d->header.jiso.block_size;
			d->disc_size = d->header.jiso.uncompressed_size;
			// TODO: Does JISO have an index_shift field? Assuming it doesn't for now...
			d->index_shift = 0;
			indexEntryTblPos = static_cast<off64_t>(sizeof(d->header.jiso));
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
			isZlib = true;
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
			// Byteswap the header.
			d->header.dax.magic		= le32_to_cpu(d->header.dax.magic);
			d->header.dax.uncompressed_size	= le32_to_cpu(d->header.dax.uncompressed_size);
			d->header.dax.version		= le32_to_cpu(d->header.dax.version);
			d->header.dax.nc_areas		= le32_to_cpu(d->header.dax.nc_areas);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

			d->block_size = DAX_BLOCK_SIZE;
			d->disc_size = d->header.dax.uncompressed_size;
			// TODO: Does DAX have an index_shift field? Assuming it doesn't for now...
			d->index_shift = 0;
			indexEntryTblPos = static_cast<off64_t>(sizeof(d->header.dax));
			break;
	}

#ifdef _MSC_VER
	// Delay load verification.
#  ifdef ZLIB_IS_DLL
	if (isZlib) {
		if (DelayLoad_test_get_crc_table() != 0) {
			// Delay load for zlib failed.
			m_file.reset();
			return;
		}
	}
#  endif /* ZLIB_IS_DLL */
#endif

#ifdef LZ4_SHARED_LINKAGE
	if (isLZ4) {
		// Attempt to load the LZ4 function pointers.
		if (d->init_pfn_LZ4() != 0) {
			// Failed to load the LZ4 function pointers.
			m_file.reset();
			return;
		}
	}
#endif /* LZ4_SHARED_LINKAGE */

	if (isLZO) {
#ifdef LZO_SHARED_LINKAGE
		// Attempt to load the LZO function pointers.
		if (d->init_pfn_LZO() != LZO_E_OK) {
			// Failed to load the LZO function pointers.
			m_file.reset();
			return;
		}
#else /* !LZO_SHARED_LINKAGE */
		// Need to call init_lzo(), even in static library builds.
		if (init_lzo() != LZO_E_OK) {
			// Failed to initialize LZO.
			m_file.reset();
			return;
		}
#endif /* LZO_SHARED_LINKAGE */
	}


#if !defined(_MSC_VER) || !defined(ZLIB_IS_DLL)
	if (isZlib) {
		// zlib isn't in a DLL, but we need to ensure that the
		// CRC table is initialized anyway.
		get_crc_table();
	}
#endif /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */

	// Calculate the number of blocks.
	const uint32_t num_blocks = static_cast<uint32_t>(d->disc_size / d->block_size);
	assert(num_blocks > 0);
	if (num_blocks == 0) {
		// No blocks...
		m_file.reset();
		return;
	}

	// Read the index entries.
	// NOTE: These are byteswapped on demand, not ahead of time.
	uint32_t num_blocks_alloc = num_blocks;
	switch (d->cisoType) {
		case CisoPspReaderPrivate::CisoType::CISO:
		case CisoPspReaderPrivate::CisoType::ZISO:
		case CisoPspReaderPrivate::CisoType::JISO:
			num_blocks_alloc++;
			break;

		default:
			break;
	}
	d->indexEntries.resize(num_blocks_alloc);
	size_t expected_size = num_blocks_alloc * sizeof(uint32_t);
	size_t size = m_file->seekAndRead(indexEntryTblPos, d->indexEntries.data(), expected_size);
	if (size != expected_size) {
		// Read error.
		m_lastError = m_file->lastError();
		if (m_lastError == 0) {
			m_lastError = EIO;
		}
		m_file.reset();
		return;
	}

	// TODO: NC areas for JISO.
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
			m_file.reset();
			return;
		}

		if (d->header.dax.nc_areas > 0) {
			// Handle the NC (non-compressed) areas.
			// This table is stored immediately after the index entry table.
			rp::uvector<DaxNCArea> nc_areas;
			nc_areas.resize(d->header.dax.nc_areas);
			expected_size = d->header.dax.nc_areas * sizeof(DaxNCArea);
			size = m_file->read(nc_areas.data(), expected_size);
			if (size != expected_size) {
				// Read error.
				m_lastError = m_file->lastError();
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				m_file.reset();
				return;
			}

			// Process the NC areas.
			d->daxNCTable.resize(num_blocks);
			for (const DaxNCArea &p : nc_areas) {
				uint32_t i = le32_to_cpu(p.start);
				const uint32_t end = i + le32_to_cpu(p.count);
				if (end > num_blocks) {
					// Out of range...
					m_lastError = EIO;
					m_file.reset();
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

	// PSP disc images are always ISO-9660 Mode 1.
	d->hasCdromInfo = true;
	d->cdromSectorInfo.mode = 1;
	d->cdromSectorInfo.sector_size = 2048;
	d->cdromSectorInfo.subchannel_size = 0;

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
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// GCZ is not supported without zlib.
		return -1;
	}
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// Check the magic number.
	const uint32_t *const pu32 = reinterpret_cast<const uint32_t*>(pHeader);
	if ((*pu32 == be32_to_cpu(CISO_MAGIC) ||
	     *pu32 == be32_to_cpu(ZISO_MAGIC)) &&
	    szHeader >= sizeof(CisoPspHeader))
	{
		// CISO/ZISO magic.
		const CisoPspHeader *const cisoPspHeader = reinterpret_cast<const CisoPspHeader*>(pHeader);

		// Header size:
		// - CISO v0/v1: 0 or 0x18
		// - CISO v2: 0x18
		// - ZISO: 0x18
		if (cisoPspHeader->header_size == 0) {
			if (pHeader[0] != 'C' || cisoPspHeader->version >= 2) {
				// Invalid header size.
				return -1;
			}
		} else if (cisoPspHeader->header_size != cpu_to_le32(sizeof(*cisoPspHeader))) {
			// Invalid header size.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Version checks.
		if (pHeader[0] == 'Z' && cisoPspHeader->version != 1) {
			// ZISO: Only v1 is known to exist.
			return -1;
		} else if (cisoPspHeader->version > 2) {
			// CISO: v2 is max.
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
		return (pHeader[0] == 'Z')
			? (int)CisoPspReaderPrivate::CisoType::ZISO
			: (int)CisoPspReaderPrivate::CisoType::CISO;
	} else if (*pu32 == be32_to_cpu(JISO_MAGIC) && szHeader >= sizeof(JisoHeader)) {
		// JISO magic.
		const JisoHeader *const jisoHeader = reinterpret_cast<const JisoHeader*>(pHeader);

		// TODO: Version number field?

		// Header size.
		// TODO: Is this accurate?
		if (jisoHeader->header_size != cpu_to_le32(sizeof(*jisoHeader))) {
			// Header size is incorrect.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Check if the block size is a supported power of two.
		// - Minimum: JISO_BLOCK_SIZE_MIN ( 2 KB, 1 << 11)
		// - Maximum: JISO_BLOCK_SIZE_MAX (64 KB, 1 << 16)
		const uint32_t block_size = le32_to_cpu(jisoHeader->block_size);
		if (!isPow2(block_size) ||
		    block_size < JISO_BLOCK_SIZE_MIN || block_size > JISO_BLOCK_SIZE_MAX)
		{
			// Block size is out of range.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Must have at least one block and less than 4 GB of uncompressed data.
		// NOTE: 4 GB is the implied maximum size due to use of uint32_t.
		const uint32_t uncompressed_size = le32_to_cpu(jisoHeader->uncompressed_size);
		if (uncompressed_size < DAX_BLOCK_SIZE) {
			// Less than one block...
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// Uncompressed data size must be a multiple of the block size.
		if (uncompressed_size % block_size != 0) {
			// Not a multiple.
			return (int)CisoPspReaderPrivate::CisoType::Unknown;
		}

		// This is a valid JISO image.
		return (int)CisoPspReaderPrivate::CisoType::JISO;
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

/** SparseDiscReader functions **/

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
	RP_D(const CisoPspReader);
	assert(!d->indexEntries.empty());
	assert(blockIdx < d->indexEntries.size()-1);
	if (d->indexEntries.empty() || blockIdx >= d->indexEntries.size()-1) {
		// Out of range.
		return -1;
	}

	// Get the physical block address.
	// NOTE: The caller has to decompress the block.
	off64_t addr = -1;
	switch (d->cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			return 0;

		case CisoPspReaderPrivate::CisoType::CISO:
		case CisoPspReaderPrivate::CisoType::ZISO:
			addr = static_cast<off64_t>(d->indexEntries[blockIdx] & ~CISO_PSP_V0_NOT_COMPRESSED);
			addr <<= d->index_shift;
			break;

		case CisoPspReaderPrivate::CisoType::JISO:
			// TODO: Does JISO have an index_shift field? Assuming it doesn't for now...
			addr = static_cast<off64_t>(d->indexEntries[blockIdx]);
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
			// TODO: Does DAX have an index_shift field? Assuming it doesn't for now...
			addr = static_cast<off64_t>(d->indexEntries[blockIdx]);
			break;
	}

	return addr;
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
int CisoPspReader::readBlock(uint32_t blockIdx, int pos, void *ptr, size_t size)
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
		return static_cast<int>(size);
	}

	// Get the physical address first.
	const uint32_t indexEntry = d->indexEntries[blockIdx];
	uint32_t z_block_size = d->getBlockCompressedSize(blockIdx);
	if (z_block_size == 0) {
		// Unable to get the block's compressed size...
		m_lastError = EIO;
		return 0;
	}

	enum class CompressionMode {
		None = 0,
		Deflate = 1,
		LZ4 = 2,
		LZO = 3,
	};
	CompressionMode z_mode;
	int windowBits = 0;

	off64_t physBlockAddr;
	switch (d->cisoType) {
		default:
		case CisoPspReaderPrivate::CisoType::Unknown:
			assert(!"Unsupported CisoType.");
			m_file.reset();
			m_lastError = ENOTSUP;
			return 0;

		case CisoPspReaderPrivate::CisoType::CISO:
			// CISO uses raw deflate.
			windowBits = -15;

			// Mask off the compression bit, and shift the address
			// based on the index shift.
			physBlockAddr = static_cast<off64_t>(indexEntry & ~CISO_PSP_V0_NOT_COMPRESSED);
			physBlockAddr <<= d->index_shift;

			if (d->header.cisoPsp.version < 2) {
				// CISO v0/v1: Check if compressed.
				z_mode = (indexEntry & CISO_PSP_V0_NOT_COMPRESSED)
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
					z_mode = (indexEntry & CISO_PSP_V2_LZ4_COMPRESSED)
						? CompressionMode::LZ4
						: CompressionMode::Deflate;
				}
			}
			break;

		case CisoPspReaderPrivate::CisoType::ZISO:
			// ZISO uses LZ4.

			// Mask off the compression bit, and shift the address
			// based on the index shift.
			physBlockAddr = static_cast<off64_t>(indexEntry & ~CISO_PSP_V0_NOT_COMPRESSED);
			physBlockAddr <<= d->index_shift;

			z_mode = (indexEntry & CISO_PSP_V0_NOT_COMPRESSED)
				? CompressionMode::None
				: CompressionMode::LZ4;
			break;

		case CisoPspReaderPrivate::CisoType::JISO:
			// JISO uses LZO or zlib.
			// TODO: Verify the rest of this.
			// TODO: Does JISO have an index_shift field? Assuming it doesn't for now...

			// JISO does *not* indicate compression using the high bit.
			// Instead, the compressed block size will match the uncompressed
			// block size, similar to CISOv2.
			physBlockAddr = static_cast<off64_t>(indexEntry);
			physBlockAddr <<= d->index_shift;

			if (d->header.jiso.block_headers) {
				// Block headers are present.
				// TODO: jiso.exe says this can provide for "faster decompression".
				if (z_block_size <= 4) {
					// Incorrect block size.
					m_lastError = EIO;
					return 0;
				}
				physBlockAddr += 4;
				z_block_size -= 4;
			}

			if (z_block_size == d->block_size) {
				z_mode = CompressionMode::None;
			} else {
				switch (d->header.jiso.method) {
					case JISO_METHOD_LZO:
						z_mode = CompressionMode::LZO;
						break;
					case JISO_METHOD_ZLIB:
						// JISO zlib uses raw deflate.
						windowBits = -15;
						z_mode = CompressionMode::Deflate;
						break;
					default:
						assert(!"Unsupported JISO compression method.");
						m_lastError = ENOTSUP;
						return 0;
				}
			}
			break;

		case CisoPspReaderPrivate::CisoType::DAX:
			// TODO: Does DAX have an index_shift field? Assuming it doesn't for now...
			physBlockAddr = static_cast<off64_t>(indexEntry);
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
			assert(windowBits != 0);
			if (windowBits == 0) {
				d->blockCacheIdx = ~0U;
				m_lastError = EINVAL;
				return 0;
			}
			uint32_t z_max_size = d->block_size;
			if (unlikely(d->isDaxWithoutNCTable)) {
				// DAX without NC table can end up compressing to larger
				// than the uncompressed size.
				z_max_size *= 2;
			}
			if (z_block_size > z_max_size) {
				// Compressed data is larger than the uncompressed block size.
				// This is only allowed for DAX without NC table.
				d->blockCacheIdx = ~0U;
				m_lastError = EIO;
				return 0;
			}

			size_t sz_read = m_file->seekAndRead(physBlockAddr, d->z_buffer.data(), z_block_size);
			if (sz_read != z_block_size) {
				// Seek and/or read error.
				d->blockCacheIdx = ~0U;
				m_lastError = m_file->lastError();
				if (m_lastError == 0) {
					m_lastError = EIO;
				}
				return 0;
			}

			// Decompress the data.
			z_stream strm = { };
			strm.next_in = d->z_buffer.data();
			strm.avail_in = z_block_size;
			strm.next_out = d->blockCache.data();
			strm.avail_out = d->block_size;
			int ret = inflateInit2(&strm, windowBits);
			if (ret != Z_OK) {
				// Error initializing zlib.
				d->blockCacheIdx = ~0U;
				m_lastError = EIO;
				return 0;
			}

			int status = inflate(&strm, Z_FULL_FLUSH);
			const uint32_t uncomp_size = d->block_size - strm.avail_out;
			inflateEnd(&strm);

			if (status != Z_STREAM_END || uncomp_size != d->block_size) {
				// Decompression error.
				// TODO: Print warnings and/or more comprehensive error codes.
				d->blockCacheIdx = ~0U;
				m_lastError = EIO;
				return 0;
			}
			break;
		}

		case CompressionMode::LZ4: {
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
#ifdef LZ4_SHARED_LINKAGE
			int sz_rd = d->pfn_LZ4_decompress_safe(
				reinterpret_cast<const char*>(d->z_buffer.data()),
				reinterpret_cast<char*>(d->blockCache.data()),
				z_block_size, d->block_size);
#else /* !LZ4_SHARED_LINKAGE */
			int sz_rd = LZ4_decompress_safe(
				reinterpret_cast<const char*>(d->z_buffer.data()),
				reinterpret_cast<char*>(d->blockCache.data()),
				z_block_size, d->block_size);
#endif /* LZ4_SHARED_LINKAGE */
			if (sz_rd != (int)d->block_size) {
				// Decompression error.
				// TODO: Print warnings and/or more comprehensive error codes.
				d->blockCacheIdx = ~0U;
				m_lastError = EIO;
				return 0;
			}
			break;
		}

		case CompressionMode::LZO: {
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
			// TODO: LZO in-place decompression?
			lzo_uint dst_len = d->block_size;
#ifdef LZO_SHARED_LINKAGE
			int ret = d->pfn_lzo1x_decompress_safe(
				d->z_buffer.data(), z_block_size,
				d->blockCache.data(), &dst_len,
				nullptr);
#else /* !LZO_SHARED_LINKAGE */
			int ret = lzo1x_decompress_safe(
				d->z_buffer.data(), z_block_size,
				d->blockCache.data(), &dst_len,
				nullptr);
#endif /* LZO_SHARED_LINKAGE */
			if (ret != LZO_E_OK || dst_len != d->block_size) {
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
	return static_cast<int>(size);
}

}

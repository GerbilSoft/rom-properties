/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartition_p.cpp: GameCube partition private class.                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "GcnPartition_p.hpp"

#include "GcnFst.hpp"
#include "GcnPartition.hpp"

// Other rom-properties libraries
#include "librpbyteswap/byteswap_rp.h"
using LibRpFile::IRpFile;

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
using std::unique_ptr;

namespace LibRomData {

GcnPartitionPrivate::GcnPartitionPrivate(GcnPartition *q,
	off64_t partition_offset, off64_t data_size, uint8_t offsetShift)
	: q_ptr(q)
	, partition_offset(partition_offset)
	, data_offset(-1)	// -1 == invalid
	, partition_size(-1)
	, data_size(-1)
	, bootLoaded(false)
	, offsetShift(offsetShift)
{
	// NOTE: The discReader parameter is needed because
	// WiiPartitionPrivate is created *before* the
	// WiiPartition superclass's discReader field is set.

	// Clear the various structs.
	memset(&bootBlock, 0, sizeof(bootBlock));
	memset(&bootInfo, 0, sizeof(bootInfo));

	// Save important data.
	this->data_offset    = partition_offset;
	this->data_size      = data_size;
	this->partition_size = data_size;
}

/**
 * Load the boot block and boot info.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnPartitionPrivate::loadBootBlockAndInfo(void)
{
	if (bootLoaded) {
		// Already loaded.
		return 0;
	}

	// Load the boot block and boot info.
	// TODO: Consolidate into a single read?
	RP_Q(GcnPartition);
	q->m_lastError = 0;
	size_t size = q->seekAndRead(GCN_Boot_Block_ADDRESS, &bootBlock, sizeof(bootBlock));
	if (size != sizeof(bootBlock)) {
		// Seek and/or read failed.
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return -q->m_lastError;
	}

	q->m_lastError = 0;
	size = q->read(&bootInfo, sizeof(bootInfo));
	if (size != sizeof(bootInfo)) {
		// bootInfo read failed.
		if (q->m_lastError == 0) {
			q->m_lastError = EIO;
		}
		return -q->m_lastError;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Byteswap bootBlock and bootInfo.
	rp_byte_swap_32_array(bootBlock.u32, sizeof(bootBlock.u32));
	rp_byte_swap_32_array(bootInfo.u32, sizeof(bootInfo.u32));
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

	// bootBlock and bootInfo have been loaded.
	bootLoaded = true;
	return 0;
}

/**
 * Load the FST.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnPartitionPrivate::loadFst(void)
{
	RP_Q(GcnPartition);
	if (fst) {
		// FST is already loaded.
		return 0;
	} else if (data_offset < 0) {
		// Partition is invalid.
		q->m_lastError = EINVAL;
		return -q->m_lastError;
	}

	// Load the boot block and boot info.
	int ret = loadBootBlockAndInfo();
	if (ret != 0) {
		// Error loading boot block and/or boot info.
		return ret;
	}

	if (bootBlock.fst_size > (1048576U >> offsetShift) ||
	    bootBlock.fst_max_size > (1048576U >> offsetShift))
	{
		// Sanity check: FST larger than 1 MB is invalid.
		// TODO: What is the actual largest FST?
		q->m_lastError = EIO;
		return -EIO;
	} else if (bootBlock.fst_size > bootBlock.fst_max_size) {
		// FST is invalid.
		q->m_lastError = EIO;
		return -EIO;
	}

	// Seek to the beginning of the FST.
	// FIXME: g++ can't resolve IRpFile::seek(off64_t) here, so explicitly specify SeekWhence::Set.
	ret = q->seek(static_cast<off64_t>(bootBlock.fst_offset) << offsetShift, IRpFile::SeekWhence::Set);
	if (ret != 0) {
		// Seek failed.
		return -q->m_lastError;
	}

	// Read the FST.
	// TODO: Eliminate the extra copy?
	const uint32_t fstData_len = bootBlock.fst_size << offsetShift;
	uint8_t *const fstData = new uint8_t[fstData_len];
	size_t size = q->read(fstData, fstData_len);
	if (size != fstData_len) {
		// Short read.
		delete[] fstData;
		q->m_lastError = EIO;
		return -EIO;
	}

	// Create the GcnFst.
	unique_ptr<GcnFst> gcnFst(new GcnFst(fstData, fstData_len, offsetShift));
	delete[] fstData;	// TODO: Eliminate the extra copy?
	if (gcnFst->hasErrors()) {
		// FST has errors.
		q->m_lastError = EIO;
		return -EIO;
	}
	this->fst = std::move(gcnFst);
	return 0;
}

} // namespace LibRomData

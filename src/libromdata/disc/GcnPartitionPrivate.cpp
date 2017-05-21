/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartitionPrivate.cpp: GameCube partition private class.              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "GcnPartitionPrivate.hpp"
#include "librpbase/byteswap.h"

#include "GcnFst.hpp"
#include "GcnPartition.hpp"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"
using LibRpBase::IDiscReader;
using LibRpBase::KeyManager;

// C includes. (C++ namespace)
#include <cstring>

namespace LibRomData {

GcnPartitionPrivate::GcnPartitionPrivate(GcnPartition *q, IDiscReader *discReader,
	int64_t partition_offset, uint8_t offsetShift)
	: q_ptr(q)
	, offsetShift(offsetShift)
	, discReader(discReader)
	, partition_offset(partition_offset)
	, data_offset(-1)	// -1 == invalid
	, partition_size(-1)
	, data_size(-1)
	, bootLoaded(false)
	, fst(nullptr)
{
	// Clear the various structs.
	memset(&bootBlock, 0, sizeof(bootBlock));
	memset(&bootInfo, 0, sizeof(bootInfo));

	if (!discReader->isOpen()) {
		q->m_lastError = discReader->lastError();
		return;
	}

	// Save important data.
	data_offset     = partition_offset;
	data_size       = discReader->size();
	partition_size  = data_size;
}

GcnPartitionPrivate::~GcnPartitionPrivate()
{
	delete fst;
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
	RP_Q(GcnPartition);
	int ret = q->seek(GCN_Boot_Block_ADDRESS);
	if (ret != 0) {
		// Seek failed.
		return -q->m_lastError;
	}

	// TODO: Consolidate into a single read?
	size_t size = q->read(&bootBlock, sizeof(bootBlock));
	if (size != sizeof(bootBlock)) {
		// bootBlock read failed.
		q->m_lastError = EIO;
		return -q->m_lastError;
	}
	size = q->read(&bootInfo, sizeof(bootInfo));
	if (size != sizeof(bootInfo)) {
		// bootInfo read failed.
		q->m_lastError = EIO;
		return -q->m_lastError;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Byteswap bootBlock.
	bootBlock.dol_offset	= be32_to_cpu(bootBlock.dol_offset);
	bootBlock.fst_offset	= be32_to_cpu(bootBlock.fst_offset);
	bootBlock.fst_size	= be32_to_cpu(bootBlock.fst_size);
	bootBlock.fst_max_size	= be32_to_cpu(bootBlock.fst_max_size);
	bootBlock.fst_mem_addr	= be32_to_cpu(bootBlock.fst_mem_addr);
	bootBlock.user_pos	= be32_to_cpu(bootBlock.user_pos);
	bootBlock.user_len	= be32_to_cpu(bootBlock.user_len);

	// Byteswap bootInfo.
	bootInfo.debug_mon_size	= be32_to_cpu(bootInfo.debug_mon_size);
	bootInfo.sim_mem_size	= be32_to_cpu(bootInfo.sim_mem_size);
	bootInfo.arg_offset	= be32_to_cpu(bootInfo.arg_offset);
	bootInfo.debug_flag	= be32_to_cpu(bootInfo.debug_flag);
	bootInfo.trk_location	= be32_to_cpu(bootInfo.trk_location);
	bootInfo.trk_size	= be32_to_cpu(bootInfo.trk_size);
	bootInfo.region_code	= be32_to_cpu(bootInfo.region_code);
	bootInfo.dol_limit	= be32_to_cpu(bootInfo.dol_limit);
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
		return -q->m_lastError;
	} else if (bootBlock.fst_size > bootBlock.fst_max_size) {
		// FST is invalid.
		q->m_lastError = EIO;
		return -q->m_lastError;
	}

	// Seek to the beginning of the FST.
	ret = q->seek((int64_t)bootBlock.fst_offset << offsetShift);
	if (ret != 0) {
		// Seek failed.
		return -q->m_lastError;
	}

	// Read the FST.
	// TODO: Eliminate the extra copy?
	uint32_t fstData_len = bootBlock.fst_size << offsetShift;
	uint8_t *fstData = static_cast<uint8_t*>(malloc(fstData_len));
	if (!fstData) {
		// malloc() failed.
		q->m_lastError = ENOMEM;
		return -q->m_lastError;
	}
	size_t size = q->read(fstData, fstData_len);
	if (size != fstData_len) {
		// Short read.
		free(fstData);
		q->m_lastError = EIO;
		return -q->m_lastError;
	}

	// Create the GcnFst.
	fst = new GcnFst(fstData, fstData_len, offsetShift);
	free(fstData);	// TODO: Eliminate the extra copy?
	return 0;
}

}

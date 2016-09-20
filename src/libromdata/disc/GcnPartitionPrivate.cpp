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
#include "../byteswap.h"

#include "IDiscReader.hpp"
#include "GcnFst.hpp"
#include "GcnPartition.hpp"

namespace LibRomData {

GcnPartitionPrivate::GcnPartitionPrivate(GcnPartition *q, IDiscReader *discReader,
	int64_t partition_offset, uint8_t offsetShift)
	: q(q)
	, lastError(0)
	, offsetShift(offsetShift)
	, discReader(discReader)
	, partition_offset(partition_offset)
	, data_offset(-1)	// -1 == invalid
	, partition_size(-1)
	, data_size(-1)
	, fst(nullptr)
{
	static_assert(sizeof(GCN_FST_Info) == GCN_FST_Info_SIZE,
		"sizeof(GCN_FST_Info) is incorrect. (Should be 12)");

	if (!discReader->isOpen()) {
		lastError = discReader->lastError();
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
 * Load the FST.
 * @return 0 on success; negative POSIX error code on error.
 */
int GcnPartitionPrivate::loadFst(void)
{
	if (fst) {
		// FST is already loaded.
		return 0;
	}

	// Load the FST info.
	GCN_FST_Info fstInfo;
	int ret = q->seek(GCN_FST_Info_ADDRESS);
	if (ret != 0) {
		// Seek failed.
		return -lastError;
	}

	size_t size = q->read(&fstInfo, sizeof(fstInfo));
	if (size != sizeof(fstInfo)) {
		// fstInfo read failed.
		lastError = EIO;
		return -lastError;
	}

#if SYS_BYTEORDER != SYS_BIG_ENDIAN
	// Byteswap fstInfo.
	fstInfo.dol_offset	= be32_to_cpu(fstInfo.dol_offset);
	fstInfo.fst_offset	= be32_to_cpu(fstInfo.fst_offset);
	fstInfo.fst_size	= be32_to_cpu(fstInfo.fst_size);
	fstInfo.fst_max_size	= be32_to_cpu(fstInfo.fst_max_size);
#endif /* SYS_BYTEORDER != SYS_BIG_ENDIAN */

	if (fstInfo.fst_size > (1048576U >> offsetShift) ||
	    fstInfo.fst_max_size > (1048576U >> offsetShift))
	{
		// Sanity check: FST larger than 1 MB is invalid.
		// TODO: What is the actual largest FST?
		lastError = EIO;
		return -lastError;
	} else if (fstInfo.fst_size > fstInfo.fst_max_size) {
		// FST is invalid.
		lastError = EIO;
		return -lastError;
	}

	// Seek to the beginning of the FST.
	ret = q->seek((int64_t)fstInfo.fst_offset << offsetShift);
	if (ret != 0) {
		// Seek failed.
		return -lastError;
	}

	// Read the FST.
	// TODO: Eliminate the extra copy?
	uint32_t fstData_len = fstInfo.fst_size << offsetShift;
	uint8_t *fstData = reinterpret_cast<uint8_t*>(malloc(fstData_len));
	if (!fstData) {
		// malloc() failed.
		lastError = ENOMEM;
		return -lastError;
	}
	size = q->read(fstData, fstData_len);
	if (size != fstData_len) {
		// Short read.
		free(fstData);
		lastError = -EIO;
		return -lastError;
	}

	// Create the GcnFst.
	fst = new GcnFst(fstData, fstData_len, 2);
	free(fstData);	// TODO: Eliminate the extra copy?
	return 0;
}

}

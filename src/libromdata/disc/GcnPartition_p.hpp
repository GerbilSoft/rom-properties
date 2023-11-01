/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartition_p.hpp: GCN/Wii partition private class.                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <stdint.h>
#include "../Console/gcn_structs.h"

namespace LibRpBase {
	class IDiscReader;
}

namespace LibRomData {

class GcnPartition;
class GcnFst;

class GcnPartitionPrivate
{
public:
	GcnPartitionPrivate(GcnPartition *q,
		off64_t partition_offset, off64_t data_size,
		uint8_t offsetShift = 0);
	virtual ~GcnPartitionPrivate();

private:
	RP_DISABLE_COPY(GcnPartitionPrivate)
protected:
	GcnPartition *const q_ptr;

public:
	// Offsets (-1 == error)
	// For GCN, these are usually 0.
	// For Wii, partition_offset is the start of the partition,
	// and data_offset is the start of the encrypted data.
	off64_t partition_offset;	// Partition start offset.
	off64_t data_offset;		// Data start offset.

	// Partition size
	// For GCN, these are both the size of the disc images.
	// For Wii, partition_size is the entire partition, including
	// header and hashes, while data size is the data area without
	// any of the hash sections.
	off64_t partition_size;		// Partition size, including header and hashes.
	off64_t data_size;		// Data size, excluding hashes.

	// Boot block and info
	GCN_Boot_Block bootBlock;
	GCN_Boot_Info bootInfo;		// bi2.bin
	bool bootLoaded;

	uint8_t offsetShift;		// GCN == 0, Wii == 2

	/**
	 * Load the boot block and boot info.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadBootBlockAndInfo(void);

	// Filesystem table
	GcnFst *fst;

	/**
	 * Load the FST.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadFst(void);
};

}

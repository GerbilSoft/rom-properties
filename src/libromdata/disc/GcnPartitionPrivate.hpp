/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GcnPartitionPrivate.hpp: GCN/Wii partition private class.               *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITIONPRIVATE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITIONPRIVATE_HPP__

#include <stdint.h>
#include "../gcn_structs.h"

namespace LibRomData {

class GcnPartition;
class IDiscReader;
class GcnFst;

class GcnPartitionPrivate
{
	public:
		GcnPartitionPrivate(GcnPartition *q, IDiscReader *discReader,
			int64_t partition_offset, uint8_t offsetShift = 0);
		virtual ~GcnPartitionPrivate();

	private:
		RP_DISABLE_COPY(GcnPartitionPrivate)
	protected:
		GcnPartition *const q_ptr;

	public:
		uint8_t offsetShift;	// GCN == 0, Wii == 2

		IDiscReader *discReader;

		// Offsets. (-1 == error)
		// For GCN, these are usually 0.
		// For Wii, partition_offset is the start of the partition,
		// and data_offset is the start of the encrypted data.
		int64_t partition_offset;	// Partition start offset.
		int64_t data_offset;		// Data start offset.

		// Partition size.
		// For GCN, these are both the size of the disc images.
		// For Wii, partition_size is the entire partition, including
		// header and hashes, while data size is the data area without
		// any of the hash sections.
		int64_t partition_size;		// Partition size, including header and hashes.
		int64_t data_size;		// Data size, excluding hashes.

		// Boot block and info.
		GCN_Boot_Block bootBlock;
		GCN_Boot_Info bootInfo;		// bi2.bin
		bool bootLoaded;

		/**
		 * Load the boot block and boot info.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadBootBlockAndInfo(void);

		// Filesystem table.
		GcnFst *fst;

		/**
		 * Load the FST.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int loadFst(void);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_GCNPARTITIONPRIVATE_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_WIIPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_WIIPARTITION_HPP__

#include "IDiscReader.hpp"

namespace LibRomData {

class IDiscReader;

class WiiPartitionPrivate;
class WiiPartition
{
	public:
		/**
		 * Construct a WiiPartition with the specified IDiscReader.
		 * NOTE: The IDiscReader *must* remain valid while this
		 * WiiPartition is open.
		 * @param discReader IDiscReader.
		 * @param partition_offset Partition start offset.
		 */
		WiiPartition(IDiscReader *discReader, int64_t partition_offset);
		~WiiPartition();

	private:
		WiiPartition(const WiiPartition &);
		WiiPartition &operator=(const WiiPartition &);
	private:
		friend class WiiPartitionPrivate;
		WiiPartitionPrivate *const d;

	public:
		enum EncInitStatus {
			ENCINIT_OK = 0,
			ENCINIT_UNKNOWN,
			ENCINIT_DISABLED,		// ENABLE_DECRYPTION disabled.
			ENCINIT_INVALID_KEY_IDX,	// Invalid common key index in the disc header.
			ENCINIT_NO_KEYFILE,		// keys.conf not found.
			ENCINIT_MISSING_KEY,		// Required key not found.
			ENCINIT_CIPHER_ERROR,		// Could not initialize the cipher.
		};

		/**
		 * Encryption initialization status.
		 * @return Encryption initialization status.
		 */
		EncInitStatus encInitStatus(void) const;
		
		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size);

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos);

		/**
		 * Seek to the beginning of the partition.
		 */
		void rewind(void);

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		int64_t partition_size(void) const;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		int64_t data_size(void) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_WIIPARTITION_HPP__ */

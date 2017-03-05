/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__
#define __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__

#include "GcnPartition.hpp"
#include "GcnFst.hpp"

namespace LibRomData {

class WiiPartitionPrivate;
class WiiPartition : public GcnPartition
{
	public:
		/**
		 * Construct a WiiPartition with the specified IDiscReader.
		 *
		 * NOTE: The IDiscReader *must* remain valid while this
		 * WiiPartition is open.
		 *
		 * @param discReader IDiscReader.
		 * @param partition_offset Partition start offset.
		 */
		WiiPartition(IDiscReader *discReader, int64_t partition_offset);
		~WiiPartition();

	private:
		typedef GcnPartition super;
		RP_DISABLE_COPY(WiiPartition)

	protected:
		friend class WiiPartitionPrivate;
		// d_ptr is used from the subclass.
		//WiiPartitionPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		virtual size_t read(void *ptr, size_t size) override final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		virtual int seek(int64_t pos) override final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		virtual int64_t tell(void) override final;

		/** WiiPartition **/

		enum EncInitStatus {
			ENCINIT_OK = 0,
			ENCINIT_UNKNOWN,
			ENCINIT_DISABLED,		// ENABLE_DECRYPTION disabled.
			ENCINIT_INVALID_KEY_IDX,	// Invalid common key index in the disc header.
			ENCINIT_NO_KEYFILE,		// keys.conf was not found.
			ENCINIT_MISSING_KEY,		// Required key not found.
			ENCINIT_CIPHER_ERROR,		// Could not initialize the cipher.
			ENCINIT_INCORRECT_KEY,		// Key is incorrect.
		};

		/**
		 * Encryption initialization status.
		 * @return Encryption initialization status.
		 */
		EncInitStatus encInitStatus(void) const;

		// Encryption key in use.
		enum EncKey {
			ENCKEY_UNKNOWN = -1,
			ENCKEY_COMMON = 0,	// Wii common key
			ENCKEY_KOREAN = 1,	// Korean key
			ENCKEY_DEBUG = 2,	// RVT-R debug key
		};

		/**
		 * Get the encryption key in use.
		 * @return Encryption key in use.
		 */
		EncKey encKey(void) const;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_DISC_WIIPARTITION_HPP__ */

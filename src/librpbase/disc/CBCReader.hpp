/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * CBCReader.hpp: AES-128-CBC data reader class.                           *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_DISC_CBCREADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_DISC_CBCREADER_HPP__

// librpbase
#include "IPartition.hpp"

namespace LibRpBase {

class IRpFile;

class CBCReaderPrivate;
class CBCReader : public LibRpBase::IPartition
{
	public:
		/**
		 * Construct a CBCReader with the specified IRpFile.
		 *
		 * NOTE: The IRpFile *must* remain valid while this
		 * CBCReader is open.
		 *
		 * @param file 		[in] IRpFile.
		 * @param offset	[in] Encrypted data start offset, in bytes.
		 * @param length	[in] Encrypted data length, in bytes.
		 * @param key		[in] Encryption key. (Must be 128-bit) [If NULL, acts like no encryption.]
		 * @param iv		[in] Initialization vector. (Must be 128-bit) [If NULL, uses ECB instead of CBC.]
		 */
		CBCReader(LibRpBase::IRpFile *file, int64_t offset, int64_t length,
			const uint8_t *key, const uint8_t *iv);
		virtual ~CBCReader();

	private:
		typedef IPartition super;
		RP_DISABLE_COPY(CBCReader)

	protected:
		friend class CBCReaderPrivate;
		CBCReaderPrivate *const d_ptr;

	public:
		/** IDiscReader **/

		/**
		 * Read data from the partition.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Set the partition position.
		 * @param pos Partition position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Get the partition position.
		 * @return Partition position on success; -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Get the data size.
		 * This size does not include the partition header,
		 * and it's adjusted to exclude hashes.
		 * @return Data size, or -1 on error.
		 */
		int64_t size(void) final;

	public:
		/** IPartition **/

		/**
		 * Get the partition size.
		 * This size includes the partition header and hashes.
		 * @return Partition size, or -1 on error.
		 */
		int64_t partition_size(void) const final;

		/**
		 * Get the used partition size.
		 * This size includes the partition header and hashes,
		 * but does not include "empty" sectors.
		 * @return Used partition size, or -1 on error.
		 */
		int64_t partition_size_used(void) const final;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_DISC_CBCREADER_HPP__ */
